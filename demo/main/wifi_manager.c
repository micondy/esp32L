#include "wifi_manager.h"
#include "My_nvs_storage.h"
/* 事件组和事件位定义 */
EventGroupHandle_t s_wifi_event_group;

#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID "demo"
#endif

#ifndef DEFAULT_WIFI_PASSWORD
#define DEFAULT_WIFI_PASSWORD "812345678"
#endif
#ifndef MAX_RETRY_COUNT
#define MAX_RETRY_COUNT 5
#endif

#ifndef SMARTCONFIG_TIMEOUT_MS
#define SMARTCONFIG_TIMEOUT_MS 30000 // 默认超时时间 30 秒
#endif

const int ESPTOUCH_START_BIT = BIT3;
const int ESPTOUCH_DONE_BIT = BIT2;
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT = BIT1;

static void handle_wifi_event(void *arg, int32_t event_id, void *event_data);
static void handle_ip_event(void *arg, int32_t event_id, void *event_data);
static void handle_smartconfig_event(void *arg, int32_t event_id, void *event_data);

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        handle_wifi_event(arg, event_id, event_data);
    }
    else if (event_base == IP_EVENT)
    {
        handle_ip_event(arg, event_id, event_data);
    }
    else if (event_base == SC_EVENT)
    {
        handle_smartconfig_event(arg, event_id, event_data);
    }
}

static int s_retry_num = 0;  // 连接重试次数
static int connected_already=0; // =1已经连接过了，然后如果disconnect,就触发重连，而不是smartconfig
wifi_config_t wifi_config;

/**
 * @brief 初始化 Wi-Fi 并尝试连接到存储的网络
 * 
 * 如果存储的网络不可用，将启动 SmartConfig 配网模式。
 */
void initialise_wifi(void)
{
    ESP_LOGI(Wifi_Manager_TAG, "Start  initialise_wifi"); // 初始化开始

    // 初始化底层的TCP/IP协议栈。
    ESP_ERROR_CHECK(esp_netif_init());

    s_wifi_event_group = xEventGroupCreate();

    // 创建事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建默认的 Wi-Fi STA 接口
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 初始化Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_t instance_any_id1;

    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SC_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id1));

    uint8_t ssid[33] = {0};
    uint8_t password[65] = {0};
    if (load_wifi_settings((char *)ssid, (char *)password, sizeof(ssid)) == ESP_OK)
    {
        memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        ESP_LOGI(Wifi_Manager_TAG, "Loaded Wi-Fi settings: SSID:%s,password:%s", ssid, password); // 加载存储的 Wi-Fi 配置
    }
    else
    {
        ESP_LOGW(Wifi_Manager_TAG, "No wifi settings found, using default values."); // 警告日志，未找到存储的 Wi-Fi 配置
        memcpy(wifi_config.sta.ssid, DEFAULT_WIFI_SSID, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, DEFAULT_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
        ESP_LOGI(Wifi_Manager_TAG, "Loaded default Wi-Fi settings: SSID:%s,password:%s", ssid, password); // 加载默认 Wi-Fi 配置
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(Wifi_Manager_TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password); // 连接成功
        return;
    }

    bits = xEventGroupWaitBits(s_wifi_event_group,
                               ESPTOUCH_START_BIT,
                               pdFALSE,
                               pdFALSE,
                               portMAX_DELAY);
    if (bits & ESPTOUCH_START_BIT)
    {
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        ESP_LOGI(Wifi_Manager_TAG, "smartconfig start"); // SmartConfig 开始
    }
    bits = xEventGroupWaitBits(s_wifi_event_group,
                               ESPTOUCH_DONE_BIT,
                               pdFALSE,
                               pdFALSE,
                               SMARTCONFIG_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (bits & ESPTOUCH_DONE_BIT)
    {
        ESP_LOGI(Wifi_Manager_TAG, "SmartConfig completed.");
        esp_smartconfig_stop();
        vEventGroupDelete(s_wifi_event_group);
    }
    else
    {
        ESP_LOGE(Wifi_Manager_TAG, "SmartConfig timeout, restarting...");
        esp_smartconfig_stop();
        vEventGroupDelete(s_wifi_event_group); // 确保事件组被删除
        esp_restart();
    }
}

static void handle_wifi_event(void *arg, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        // 在每次尝试连接之前清除相关事件位
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
        esp_wifi_connect();
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAX_RETRY_COUNT)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(Wifi_Manager_TAG, "Retry to connect to the AP: nums:%d", s_retry_num);
        }
        else if (connected_already == 1)
        {
            ESP_LOGE(Wifi_Manager_TAG, "Restarting device due to repeated disconnections.");
            s_retry_num = 0;
            esp_restart();
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_START_BIT);
            ESP_LOGI(Wifi_Manager_TAG, "Failed to connect to the AP after retries.");
        }
    }
}

static void handle_ip_event(void *arg, int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(Wifi_Manager_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        connected_already = 1;
        // 在连接成功时设置事件位
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void handle_smartconfig_event(void *arg, int32_t event_id, void *event_data)
{
    if (event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(Wifi_Manager_TAG, "SmartConfig scan done.");
    }
    else if (event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(Wifi_Manager_TAG, "SmartConfig found channel.");
    }
    else if (event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(Wifi_Manager_TAG, "SmartConfig got SSID and password.");
        // 处理 SSID 和密码的逻辑保持不变
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        uint8_t rvd_data[33] = {0};

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        if (save_wifi_settings((const char *)evt->ssid, (const char *)evt->password) != ESP_OK)
        {
            ESP_LOGE(Wifi_Manager_TAG, "Failed to save Wi-Fi settings to NVS."); // 错误日志，保存 Wi-Fi 配置失败
        }
        else
        {
            ESP_LOGI(Wifi_Manager_TAG, "Saved Wi-Fi settings to NVS: SSID:%s, password:%s", evt->ssid, evt->password); // 保存 Wi-Fi 配置成功
        }
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(Wifi_Manager_TAG, "SSID:%s", ssid); // 打印 SSID
        ESP_LOGI(Wifi_Manager_TAG, "PASSWORD:%s", password); // 打印密码
        if (evt->type == SC_TYPE_ESPTOUCH_V2)
        {
            ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
            ESP_LOGI(Wifi_Manager_TAG, "RVD_DATA:"); // 打印 RVD 数据
            for (int i = 0; i < 33; i++)
            {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    else if (event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}
