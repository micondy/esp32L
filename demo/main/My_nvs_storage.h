#pragma once
#include "nvs_flash.h"
#define MY_NVS_TAG "my_nvs_storage"
//读写空间,然后保存读取，应该不会冲突吧，没有锁现在
esp_err_t save_wifi_settings(const char *ssid, const char *password);
esp_err_t load_wifi_settings(char *ssid, char *password, size_t buffer_size);
