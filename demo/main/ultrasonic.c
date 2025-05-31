#include "ultrasonic.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#define ULTRASONIC_TAG "ULTRASONIC"

static const char* TAG = "Ultrasonic";


ultrasonic_sensor_t hc_sr04_sensor;
uint32_t hc_sr04_trig_pin = 2;
uint32_t hc_sr04_echo_pin = 15;
float hc_sr04_distance;

// 中断处理函数
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    ultrasonic_sensor_t* sensor = (ultrasonic_sensor_t*)arg;
    uint64_t timestamp = esp_timer_get_time();
    xQueueSendFromISR(sensor->queue, &timestamp, NULL);
}

// 测量任务
static void measurement_task(void* arg) {
    ultrasonic_sensor_t* sensor = (ultrasonic_sensor_t*)arg;
    static uint64_t rise_time = 0;
    static bool expecting_rising_edge = true;

    while (1) {
        uint64_t timestamp;
        if (xQueueReceive(sensor->queue, &timestamp, portMAX_DELAY)) {
            if (expecting_rising_edge) {
                // 记录上升沿时间
                rise_time = timestamp;
                expecting_rising_edge = false;
            } else {
                // 计算下降沿时间差
                uint64_t pulse_duration = timestamp - rise_time;
                expecting_rising_edge = true;

                // 有效性检查
                if (pulse_duration > sensor->timeout_us) {
                    ESP_LOGW(TAG, "Measurement timeout");
                    continue;
                }
                // 计算距离
                hc_sr04_distance = pulse_duration * 0.0343f / 2.0f;
                ESP_LOGI(TAG, "Distance: %.2f cm", hc_sr04_distance);
            }
        }
    }
}

esp_err_t ultrasonic_init(ultrasonic_sensor_t* sensor, 
                         gpio_num_t trig_pin,
                         gpio_num_t echo_pin,
                         uint32_t timeout_us) {
    // 参数校验
    if (!GPIO_IS_VALID_OUTPUT_GPIO(trig_pin) || 
        !GPIO_IS_VALID_GPIO(echo_pin)) {
        ESP_LOGE(TAG, "Invalid GPIO pins");
        return ESP_ERR_INVALID_ARG;
    }

    // 初始化结构体
    *sensor = (ultrasonic_sensor_t){
        .trig_pin = trig_pin,
        .echo_pin = echo_pin,
        .timeout_us = timeout_us,
        .queue = NULL,
        .task_handle = NULL
    };

    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << trig_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = (1ULL << echo_pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // 创建队列
    sensor->queue = xQueueCreate(10, sizeof(uint64_t));
    if (!sensor->queue) {
        ESP_LOGE(TAG, "Queue creation failed");
        return ESP_ERR_NO_MEM;
    }

    // 创建任务
    BaseType_t ret = xTaskCreate(measurement_task, 
                                "ultrasonic_task",
                                2048,
                                sensor,
                                6,
                                &sensor->task_handle);
    if (ret != pdPASS) {
        vQueueDelete(sensor->queue);
        ESP_LOGE(TAG, "Task creation failed");
        return ESP_FAIL;
    }

    // 注册中断
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(echo_pin, gpio_isr_handler, sensor));

    ESP_LOGI(ULTRASONIC_TAG, "超声波模块初始化完成");

    return ESP_OK;
}

esp_err_t ultrasonic_measure_once(ultrasonic_sensor_t* sensor) {
    // 发送触发脉冲
    ESP_ERROR_CHECK(gpio_set_level(sensor->trig_pin, 0));
    esp_rom_delay_us(2);
    ESP_ERROR_CHECK(gpio_set_level(sensor->trig_pin, 1));
    esp_rom_delay_us(10);
    ESP_ERROR_CHECK(gpio_set_level(sensor->trig_pin, 0));

    return ESP_OK;
}

void ultrasonic_cleanup(ultrasonic_sensor_t* sensor) {
    if (!sensor) return;

    if (sensor->task_handle) {
        vTaskDelete(sensor->task_handle);
        sensor->task_handle = NULL;
        ESP_LOGI(TAG, "测距任务已删除");
    }
    if (sensor->queue) {
        vQueueDelete(sensor->queue);
        sensor->queue = NULL;
        ESP_LOGI(TAG, "队列已删除");
    }
    gpio_isr_handler_remove(sensor->echo_pin);
    gpio_uninstall_isr_service();
    ESP_LOGI(TAG, "中断服务已卸载");
    ESP_LOGI(ULTRASONIC_TAG, "超声波模块资源已释放");
}