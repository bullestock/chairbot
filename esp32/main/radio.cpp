#include "config.h"
#include "nvs.h"
#include "protocol.h"
#include "radio.h"

#include "esp_crc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"

#include <string.h>
#include <stdint.h>
#include <mutex>

// Ticks to wait for queue to become available
#define ESPNOW_MAXDELAY 512

static SemaphoreHandle_t send_mutex;
static bool send_pending = false; // protected by send_mutex

static std::mutex receive_mutex;
static ForwardAirFrame received_frame; // protected by receive_mutex
static bool halted = true;

static QueueHandle_t s_espnow_queue;

static uint8_t s_other_mac[ESP_NOW_ETH_ALEN];

static void espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void espnow_task(void *pvParameter);

ForwardAirFrame get_received_frame()
{
    std::lock_guard<std::mutex> g(receive_mutex);
    return received_frame;
}

bool is_halted()
{
    std::lock_guard<std::mutex> g(receive_mutex);
    return halted;
}

void fatal_error(const char* why)
{
    printf("FATAL ERROR: %s\n", why);
    esp_restart();
}

static unsigned int to_int(char c)
{
  if (c >= '0' && c <= '9')
      return c - '0';
  if (c >= 'A' && c <= 'F')
      return 10 + c - 'A';
  if (c >= 'a' && c <= 'f')
      return 10 + c - 'a';
  return -1;
}

bool init_radio()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    const auto my_mac_str = get_my_mac();
    if (my_mac_str[0])
    {
        uint8_t my_mac[ESP_NOW_ETH_ALEN];
        for (size_t i = 0; i < ESP_NOW_ETH_ALEN; ++i)
            my_mac[i] = 16 * to_int(my_mac_str[2*i]) + to_int(my_mac_str[2*i+1]);
        ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_STA, my_mac));
        ESP_LOGI(TAG, "Set MAC successfully");
    }

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF,
                                          WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                                          WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif

    uint8_t mac[ESP_NOW_ETH_ALEN];
    esp_wifi_get_mac((wifi_interface_t) ESP_IF_WIFI_STA, mac);
    ESP_LOGI(TAG, "My MAC: " MACSTR, MAC2STR(mac));

    const auto peer_mac = get_peer_mac();
    for (size_t i = 0; i < ESP_NOW_ETH_ALEN; ++i)
        s_other_mac[i] = 16 * to_int(peer_mac[2*i]) + to_int(peer_mac[2*i+1]);
    ESP_LOGI(TAG, "Other MAC: " MACSTR, MAC2STR(s_other_mac));

    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return false;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL));
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t*) CONFIG_ESPNOW_PMK));

    // Add peer to peer list
    auto peer = reinterpret_cast<esp_now_peer_info_t*>(malloc(sizeof(esp_now_peer_info_t)));
    if (!peer)
        fatal_error("Malloc peer information fail");
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_other_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    send_mutex = xSemaphoreCreateMutex();
    
    xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 4, NULL);
    
    return true;
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void espnow_send_cb(const uint8_t*, esp_now_send_status_t status)
{
    espnow_event_t evt;
    evt.id = ESPNOW_SEND_CB;
    evt.info.send_status = status;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE)
        ESP_LOGW(TAG, "Send send queue fail");
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    espnow_event_t evt;
    espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = ESPNOW_RECV_CB;
    recv_cb->data = reinterpret_cast<uint8_t*>(malloc(len));
    if (!recv_cb->data)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

static void espnow_task(void*)
{
    vTaskDelay(500 / portTICK_PERIOD_MS);

    auto last_receive_tick = xTaskGetTickCount();

    while (1)
    {
        const auto now_tick = xTaskGetTickCount();
        espnow_event_t evt;
        if (!xQueueReceive(s_espnow_queue, &evt, 10 / portTICK_PERIOD_MS))
        {
            if (now_tick - last_receive_tick > max_radio_idle_time)
            {
                bool was_halted = false;
                {
                    std::lock_guard<std::mutex> g(receive_mutex);
                    was_halted = halted;
                    halted = true;
                }
                if (!was_halted && halted)
                    printf("%lu: HALT: Last packet was seen at %lu\n", now_tick, last_receive_tick);
            }
            continue;
        }

        switch (evt.id)
        {
        case ESPNOW_SEND_CB:
            {
                const auto send_status = evt.info.send_status;

                ESP_LOGD(TAG, "Sent data, status: %d", send_status);

                if (xSemaphoreTake(send_mutex, portMAX_DELAY) == pdTRUE)
                {
                    send_pending = false;
                    xSemaphoreGive(send_mutex);
                }
                break;
            }
        case ESPNOW_RECV_CB:
            {
                const auto recv_cb = &evt.info.recv_cb;

                if (recv_cb->data_len < sizeof(ForwardAirFrame))
                    ESP_LOGE(TAG, "Received ESPNOW data too short, len:%d", recv_cb->data_len);
                else
                {
                    last_receive_tick = now_tick;
                    auto frame = reinterpret_cast<const ForwardAirFrame*>(recv_cb->data);
                    bool was_halted = false;
                    {
                        std::lock_guard<std::mutex> g(receive_mutex);
                        was_halted = halted;
                        halted = false;
                        memcpy(&received_frame, frame, sizeof(ForwardAirFrame));
                    }
                    if (was_halted)
                        printf("%lu: RESUME\n", now_tick);
                }
                free(recv_cb->data);
                break;
            }
        default:
            ESP_LOGE(TAG, "Callback type error: %d", evt.id);
            break;
        }
    }
}

bool send_frame(ReturnAirFrame& frame)
{
    if (xSemaphoreTake(send_mutex, portMAX_DELAY) == pdTRUE)
    {
        if (send_pending)
        {
            // A frame is already pending.
            xSemaphoreGive(send_mutex);
            printf("PENDING\n");
            return true; //false;
        }
        send_pending = true;
        xSemaphoreGive(send_mutex);
    }
    return esp_now_send(s_other_mac, (uint8_t*) &frame, sizeof(ReturnAirFrame)) == ESP_OK;
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
