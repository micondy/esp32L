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
    ledc_motor_pwm_init();
     // 初始化传感器 25000us,4.25m了.
    ESP_ERROR_CHECK(ultrasonic_init(&hc_sr04_sensor, 22, 23, 25000));
    
    xTaskCreate(tcp_client_receive_task, "tcp_client_receive", 4096, NULL, 5, NULL);
    xTaskCreate(tcp_client_send_task, "tcp_client_send", 4096, NULL, 5, NULL);
   
    while (1)
    {  //ledc_set_motor_pwm_duty(0, 512);
       // ledc_set_motor_pwm_duty(1, 512);
        //ledc_set_motor_pwm_duty(2, 512);
        //ledc_set_motor_pwm_duty(3, 512);
       // ledc_set_motor_pwm_duty(0, 1024);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // ledc_set_motor_pwm_duty(1, 1024);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // ledc_set_motor_pwm_duty(2, 1024);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // ledc_set_motor_pwm_duty(3, 1024);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // //ledc_set_motor_pwm_duty(0, 1023);
        ultrasonic_measure_once(&hc_sr04_sensor);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        //ESP_LOGE("MOTOR", "ledc_set_motor_pwm_duty 512");
    }
    
}

