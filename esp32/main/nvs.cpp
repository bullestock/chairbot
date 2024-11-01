#include "nvs.h"

#include "config.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_now.h"
#include "nvs_flash.h"

static char peer_mac[2*ESP_NOW_ETH_ALEN + 1];

bool set_peer_mac(const char* mac)
{
    if (strlen(mac) != 2*ESP_NOW_ETH_ALEN)
        return false;
    strcpy(peer_mac, mac);
    nvs_handle my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, MAC_KEY, peer_mac));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    return true;
}

const char* get_peer_mac()
{
    return peer_mac;
}

void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
    size_t sz = 0;
    if (nvs_get_str(my_handle, MAC_KEY, nullptr, &sz) != ESP_OK ||
        sz != 2*ESP_NOW_ETH_ALEN + 1)
    {
        printf("No peer MAC (%d)\n", (int) sz);
        peer_mac[0] = 0;
    }
    else
    {
        if (nvs_get_str(my_handle, MAC_KEY, peer_mac, &sz) != ESP_OK)
        {
            printf("Error reading peer MAC\n");
            peer_mac[0] = 0;
        }
        else
            printf("Peer MAC %s\n", peer_mac);
    }
    nvs_close(my_handle);
}
