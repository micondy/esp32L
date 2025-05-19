#ifndef SMARTCONFIG_H
#define SMARTCONFIG_H

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


/* 事件组和事件位定义 */
extern  EventGroupHandle_t s_wifi_event_group;
extern const int ESPTOUCH_START_BIT ;
extern const int ESPTOUCH_DONE_BIT ;
extern const int WIFI_CONNECTED_BIT ;
extern const int WIFI_FAIL_BIT ;

/* 日志标签 */
#define Wifi_Manager_TAG "WIFI_MANAGER"

/* 函数声明 */
void initialise_wifi(void);

#endif