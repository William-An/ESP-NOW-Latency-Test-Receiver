/************************************************************
*
* @file:      main.c
* @author:    Weili An
* @email:     an107@purdue.edu
* @version:   v1.0.0
* @date:      09/10/2021
* @brief:     ESP-NOW latency test receiver, basically just
*             echo the received data
*
************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_now.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_crc.h"

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

void echo_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len);
esp_err_t get_macAddr();
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // Init ESP-NOW
    ESP_LOGI("ESP-NOW", "Init ESP-NOW");
    ESP_ERROR_CHECK(esp_now_init());
    uint32_t esp_now_version;
    ESP_ERROR_CHECK(esp_now_get_version(&esp_now_version));
    ESP_LOGI("ESP-NOW", "ESP-NOW Version: %d", esp_now_version);

    get_macAddr();

    // Register ESP-NOW recv cb
    ESP_LOGI("ESP-NOW", "Register ESP-NOW recv callback func");
    ESP_ERROR_CHECK(esp_now_register_recv_cb(echo_cb));

    // Dead loop
    for(;;) {
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

/**
 * @brief ECHO whatever send to this ESP
 * 
 * @param mac_addr : Sender mac addr
 * @param data     : payload ptr
 * @param data_len : payload length
 */
void echo_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    // ESP_LOGI("ESP-NOW", "Received message");
     /* If MAC address does not exist in peer list, add it to peer list. */
    if (esp_now_is_peer_exist(mac_addr) == false) {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            ESP_LOGE("Callback", "Malloc peer information fail");
            vTaskDelete(NULL);
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = 0;
        peer->ifidx = ESPNOW_WIFI_IF;
        peer->encrypt = false;
        memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(peer) );
        free(peer);
    }
    ESP_ERROR_CHECK(esp_now_send(mac_addr, data, data_len));
    ESP_LOGI("ESP-NOW", "Sent message back with %d bytes", data_len);
}

esp_err_t get_macAddr() {
    uint8_t mac[6] = {0};
    esp_err_t err = ESP_OK;

    err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Station MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    err = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "AP MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    err = esp_read_mac(mac, ESP_MAC_BT);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Bluetooth MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    err = esp_read_mac(mac, ESP_MAC_ETH);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Ethernet MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return err;
}