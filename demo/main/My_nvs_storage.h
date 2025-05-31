#pragma once
#include "nvs_flash.h"

// 读写空间,然后保存读取，应该不会冲突吧，没有锁现在
// buffer_size: ssid和password缓冲区长度，建议>=64
esp_err_t save_wifi_settings(const char *ssid, const char *password);
esp_err_t load_wifi_settings(char *ssid, char *password, size_t buffer_size);
