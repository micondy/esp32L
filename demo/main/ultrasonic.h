#pragma once
#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t trig_pin;       // 触发引脚
    gpio_num_t echo_pin;       // 回波引脚
    uint32_t timeout_us;       // 超时时间（微秒）
    QueueHandle_t queue;       // 事件队列
    TaskHandle_t task_handle;  // 任务句柄
} ultrasonic_sensor_t;

extern ultrasonic_sensor_t  hc_sr04_sensor;
extern uint32_t hc_sr04_trig_pin ;
extern uint32_t hc_sr04_echo_pin;
extern float hc_sr04_distance;
/**
 * @brief 初始化超声波传感器
 * @param sensor 传感器配置结构体指针
 * @param trig_pin 触发引脚
 * @param echo_pin 回波引脚
 * @param timeout_us 测量超时时间（微秒）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t ultrasonic_init(ultrasonic_sensor_t* sensor, 
                         gpio_num_t trig_pin,
                         gpio_num_t echo_pin,
                         uint32_t timeout_us);

/**
 * @brief 获取距离测量结果（厘米）
 * @param sensor 传感器配置结构体指针
 * @param distance_cm 输出距离值
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t ultrasonic_measure_once(ultrasonic_sensor_t* sensor);

/**
 * @brief 清理传感器资源
 * @param sensor 传感器配置结构体指针
 */
void ultrasonic_cleanup(ultrasonic_sensor_t* sensor);

#ifdef __cplusplus
}
#endif