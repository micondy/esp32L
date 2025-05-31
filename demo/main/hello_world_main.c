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

void sht30_task(void *pvParameters)
{
    bool sht30_ready = false;

    while (1)
    {
        if (!sht30_ready)
        {
            if (sht30_init(SHT30_I2C_SCL_IO, SHT30_I2C_SDA_IO, SHT30_I2C_PORT, SHT30_I2C_FREQ_HZ) == ESP_OK)
            {
                sht30_ready = true;
                ESP_LOGI("SHT30", "初始化成功");
            }
            else
            {
                ESP_LOGE("SHT30", "初始化失败， 10秒后重试");
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
        }
        sht30_read_data();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// 新版超声波测距任务，自动初始化和重试
void ultrasonic_task(void *pvParameters)
{
    bool ultrasonic_ready = false;
    while (1)
    {
        if (!ultrasonic_ready)
        {
            if (ultrasonic_init(&hc_sr04_sensor, hc_sr04_trig_pin, hc_sr04_echo_pin, 25000) == ESP_OK)
            {
                ultrasonic_ready = true;
                ESP_LOGI("ULTRASONIC", "初始化成功");
            }
            else
            {
                ESP_LOGE("ULTRASONIC", "初始化失败，10秒后重试");
                vTaskDelay(pdMS_TO_TICKS(10000)); // 10秒重试
                continue;
            }
        }
        ultrasonic_measure_once(&hc_sr04_sensor);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

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

    // 创建网络通信任务
    xTaskCreate(tcp_client_receive_task, "tcp_client_receive", 4096, NULL, 5, NULL);
    xTaskCreate(tcp_client_send_task, "tcp_client_send", 4096, NULL, 5, NULL);

    // 直接创建传感器采集任务（任务内部自动初始化和重试）
    xTaskCreate(sht30_task, "sht30_task", 4096, NULL, 4, NULL);
    xTaskCreate(ultrasonic_task, "ultrasonic_task", 2048, NULL, 4, NULL);

    // 主线程可空转或做监控
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
