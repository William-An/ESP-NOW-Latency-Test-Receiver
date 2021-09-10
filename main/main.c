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

void echo_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len);

void app_main(void)
{
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

    // Register ESP-NOW recv cb
    ESP_LOGI("ESP-NOW", "Register ESP-NOW recv callback func");
    ESP_ERROR_CHECK(esp_now_register_recv_cb(echo_cb));

    // Dead loop
    for(;;);
}

/**
 * @brief ECHO whatever send to this ESP
 * 
 * @param mac_addr : Sender mac addr
 * @param data     : payload ptr
 * @param data_len : payload length
 */
void echo_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    ESP_ERROR_CHECK(esp_now_send(mac_addr, data, data_len));
}
