#include "My_nvs_storage.h"

#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"


#include "nvs_flash.h"
#include "esp_log.h"

#define MY_NVS_TAG   "WIFI_NVS"
#define NVS_STORAGE_TAG "NVS_STORAGE"
#define NVS_WIFI_SPACE "wifi_config"
#define MAX_SSID_LEN  32
#define MAX_PASS_LEN  64

/* 保存WiFi凭证到NVS */
esp_err_t save_wifi_settings(const char *ssid, const char *password) {
    // 参数有效性检查
    if (!ssid || !password || strlen(ssid) == 0 || strlen(password) == 0) {
        ESP_LOGE(MY_NVS_TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // 检查长度限制
    if (strlen(ssid) >= MAX_SSID_LEN || strlen(password) >= MAX_PASS_LEN) {
        ESP_LOGE(MY_NVS_TAG, "SSID/PASS too long");
        return ESP_ERR_INVALID_SIZE;
    }

    nvs_handle_t handle;
    esp_err_t err;

    // 打开NVS命名空间
    if ((err = nvs_open(NVS_WIFI_SPACE, NVS_READWRITE, &handle)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    // 存储SSID
    if ((err = nvs_set_str(handle, "wifi_ssid", ssid)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "Save SSID failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // 存储密码
    if ((err = nvs_set_str(handle, "wifi_pass", password)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "Save PASS failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // 提交更改
    if ((err = nvs_commit(handle)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "Commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(NVS_STORAGE_TAG, "WiFi配置已保存: SSID=%s", ssid);
    }

cleanup:
    nvs_close(handle);
    return err;
}

/* 从NVS加载WiFi凭证 */
esp_err_t load_wifi_settings(char *ssid, char *password, size_t buffer_size) {
    nvs_handle_t handle;
    esp_err_t err;

    // 初始化缓冲区
    if (ssid) memset(ssid, 0, buffer_size);
    if (password) memset(password, 0, buffer_size);

    // 打开NVS命名空间
    if ((err = nvs_open(NVS_WIFI_SPACE, NVS_READONLY, &handle)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    // 读取SSID
    size_t required_size = buffer_size;
    if ((err = nvs_get_str(handle, "wifi_ssid", ssid, &required_size)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "Read SSID failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // 读取密码
    required_size = buffer_size;
    if ((err = nvs_get_str(handle, "wifi_pass", password, &required_size)) != ESP_OK) {
        ESP_LOGE(MY_NVS_TAG, "Read PASS failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(MY_NVS_TAG, "Loaded: SSID=%s", ssid);
    }

cleanup:
    nvs_close(handle);
    return err;
}