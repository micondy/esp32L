/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"

#include "wifi_manager.h"
#include "tcp_client.h"
#include "My_nvs_storage.h"
#include "ledc_motor_pwm.h"

#include "ultrasonic.h"
#include "sht30.h"

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化WiFi
    initialise_wifi();
    // 初始化电机
    ledc_motor_pwm_init();
    // 初始化超声波 25000us,4.25m了.
    ultrasonic_init(&hc_sr04_sensor, hc_sr04_trig_pin, hc_sr04_echo_pin, 25000);
    // 初始化sht30
    sht30_init(SHT30_I2C_SCL_IO, SHT30_I2C_SDA_IO, SHT30_I2C_PORT, SHT30_I2C_FREQ_HZ);

    xTaskCreate(tcp_client_receive_task, "tcp_client_receive", 4096, NULL, 5, NULL);
    xTaskCreate(tcp_client_send_task, "tcp_client_send", 4096, NULL, 5, NULL);

    while (1)
    {
        // ultrasonic_measure_once(&hc_sr04_sensor);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        sht30_read_data();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // sht30_deinit();
}
