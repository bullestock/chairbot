#pragma once

#include "protocol.h"

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   (wifi_interface_t) ESP_IF_WIFI_STA

#define ESPNOW_QUEUE_SIZE           6

enum espnow_event_id_t
{
    ESPNOW_SEND_CB,
    ESPNOW_RECV_CB,
};

struct espnow_event_recv_cb_t
{
    uint8_t* data;
    int data_len;
};

union espnow_event_info_t
{
    esp_now_send_status_t send_status;
    espnow_event_recv_cb_t recv_cb;
};

// Event posted to task
struct espnow_event_t
{
    espnow_event_id_t id;
    espnow_event_info_t info;
};

bool init_radio();

bool send_frame(ReturnAirFrame& frame);

/// Get last received frame
ForwardAirFrame get_received_frame();

/// Is most recent frame too old?
bool is_halted();
