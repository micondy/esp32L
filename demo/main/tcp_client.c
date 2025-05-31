#include "tcp_client.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "cJSON.h"
#include "ledc_motor_pwm.h"

#define TCP_CLIENT_TAG "TCP_CLIENT"

char rx_buffer[256] = {0};
char tx_buffer[256] = {0};
char json_buffer[256] = {0};

void tcp_client_receive_task(void *pvParameters)
{
    // At task start:
    UBaseType_t stack_high_watermark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGD(TCP_CLIENT_TAG, "tcp_client_receive_task : Initial stack free: %u", stack_high_watermark); // 调试日志，记录初始堆栈剩余空间

    int sockfd = -1;
    struct sockaddr_in dest_addr = {0};
    while (1)
    {
        // 创建socket
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            ESP_LOGE(TCP_CLIENT_TAG, "Socket creation failed"); // 错误日志，Socket 创建失败
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        else
        {
            ESP_LOGI(TCP_CLIENT_TAG, "Socket created successfully"); // 信息日志，Socket 创建成功
        }

        // 配置服务器地址
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &dest_addr.sin_addr.s_addr);

        // 连接服务器
        if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
        {
            ESP_LOGE(TCP_CLIENT_TAG, "Connection failed"); // 错误日志，连接失败
            close(sockfd);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        else
        {
            ESP_LOGI(TCP_CLIENT_TAG, "Connected to server %s:%d", SERVER_IP, SERVER_PORT); // 信息日志，连接成功
        }

        // 发送密钥
        if (send(sockfd, UUID, strlen(UUID), 0) < 0)
        {
            ESP_LOGE(TCP_CLIENT_TAG, "UUID send failed"); // 错误日志，UUID 发送失败
            close(sockfd);
            continue;
        }
        else
        {
            ESP_LOGI(TCP_CLIENT_TAG, "UUID sent: %s", UUID); // 信息日志，UUID 发送成功
        }

        // 接收循环
        while (1)
        {
            int len = recv(sockfd, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE(TCP_CLIENT_TAG, "Receive error"); // 错误日志，接收失败
                break;
            }
            else if (len == 0)
            {
                ESP_LOGW(TCP_CLIENT_TAG, "TCP连接关闭"); // 警告日志，连接关闭
                break;
            }

            rx_buffer[len] = '\0';
            ESP_LOGI(TCP_CLIENT_TAG, "Received data: %s", rx_buffer); // 信息日志，接收到数据
            handle_received_command(rx_buffer);
            memset(rx_buffer, 0, sizeof(rx_buffer)); // 清空接收缓冲区

            // Periodically check:
            stack_high_watermark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGD(TCP_CLIENT_TAG, "tcp_client_receive_task : Current stack free: %u", stack_high_watermark); // 调试日志，记录当前堆栈剩余空间
        }

        close(sockfd);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void handle_received_command(const char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL)
    {
        ESP_LOGE(TCP_CLIENT_TAG, "JSON parsing failed"); // 错误日志，JSON 解析失败
        return;
    }
    //拿到code
     cJSON *code_item = cJSON_GetObjectItem(json, "code");
    if (!cJSON_IsString(code_item)) {
        ESP_LOGE(TCP_CLIENT_TAG, "Missing or invalid 'code' field");
        cJSON_Delete(json);
        return;
    }
    const char *code = code_item->valuestring;
    // 拿到data
    cJSON *command_item = cJSON_GetObjectItem(json, "data");
    if (!cJSON_IsString(command_item))
    {
        ESP_LOGE(TCP_CLIENT_TAG, "Command field missing or invalid in JSON"); // 错误日志，命令字段缺失或无效
        cJSON_Delete(json);
        return;
    }
    int motor_value = atoi(command_item->valuestring);
    int motor_pin = -1;
    ESP_LOGI(TCP_CLIENT_TAG, "Received %s: %d", code, motor_value); // 信息日志，接收到电机状态

    if(strcmp(code, "motor0_status") == 0)
    {
        motor_pin = 0;
    }
    else if(strcmp(code, "motor1_status") == 0)
    {
        motor_pin = 1;
    }
    else if(strcmp(code, "motor2_status") == 0)
    {
        motor_pin = 2;
    }
    else if(strcmp(code, "motor3_status") == 0)
    {
        motor_pin = 3;
    }
    else
    {
        ESP_LOGE(TCP_CLIENT_TAG, "Invalid Motor: %s", code); // 错误日志，无效的代码
        cJSON_Delete(json);
        return;
    }
    // 根据命令执行操作
    if(motor_value <=1024 && motor_value >= 0)
    {
        ledc_set_motor_pwm_duty(motor_pin, motor_value);
        ESP_LOGI(TCP_CLIENT_TAG, "Motor %d PWM duty set to: %d",motor_pin, motor_value); // 信息日志，设置电机 PWM 占空比
    }
    else
    {
        ESP_LOGW(TCP_CLIENT_TAG, "Invalid motor_status value: %d", motor_value); // 警告日志，无效的电机状态值
    }
    cJSON_Delete(json);
    return;
}

void tcp_client_send_task(void *pvParameters)
{
    UBaseType_t stack_high_watermark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGD(TCP_CLIENT_TAG, "tcp_client_send_task : Initial stack free: %u", stack_high_watermark); // 调试日志，记录初始堆栈剩余空间

    int sockfd = -1;
    struct sockaddr_in dest_addr = {0};

    while (1)
    {
        // 创建socket
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            ESP_LOGE(TCP_CLIENT_TAG, "Send_Socket creation failed"); // 错误日志，Socket 创建失败
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        else
        {
            ESP_LOGI(TCP_CLIENT_TAG, "Socket created successfully"); // 信息日志，Socket 创建成功
        }

        // 配置服务器地址
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &dest_addr.sin_addr.s_addr);

        // 连接服务器
        if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
        {
            ESP_LOGE(TCP_CLIENT_TAG, "Send_Connection failed"); // 错误日志，连接失败
            close(sockfd);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // 发送数据循环
        while (1)
        {
            prepare_data_to_send(tx_buffer, sizeof(tx_buffer)); // 准备发送的数据
            if (send(sockfd, tx_buffer, strlen(tx_buffer), 0) < 0)
            {
                ESP_LOGE(TCP_CLIENT_TAG, "Send error"); // 错误日志，发送失败
                break;
            }
            ESP_LOGI(TCP_CLIENT_TAG, "Data sent: %s", tx_buffer); // 信息日志，数据发送成功

            // 模拟发送间隔
            vTaskDelay(pdMS_TO_TICKS(5000));

            // Periodically check:
            stack_high_watermark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGD(TCP_CLIENT_TAG, "tcp_client_send_task : Current stack free: %u", stack_high_watermark); // 调试日志，记录当前堆栈剩余空间
        }

        close(sockfd);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#include "ultrasonic.h"
#include "sht30.h"
void prepare_data_to_send(char *data_to_send, size_t buffer_size)
{
    if (data_to_send == NULL)
    {
        ESP_LOGE(TCP_CLIENT_TAG, "Invalid buffer for data_to_send"); // 错误日志，无效的缓冲区
        return;
    }

    // 发送温湿度
    memset(data_to_send, 0, buffer_size); // 清空缓冲区

    char temp_buffer[128] = {0};
    snprintf(temp_buffer, sizeof(temp_buffer), "~%s|humidity:%.2f,temperature:%.2f,distance:%.2f",
             UUID, sht30_humidity, sht30_temperature, hc_sr04_distance);

    memcpy(data_to_send, temp_buffer, strlen(temp_buffer));
    ESP_LOGD(TCP_CLIENT_TAG, "Prepared data: %s", data_to_send); // 调试日志，准备好的数据
}