// filepath: ledc_motor_pwm/main/ledc_motor_pwm.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "ledc_motor_pwm.h"
#include "esp_log.h"
#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE

#define LEDC_TEST_CH_NUM       (4)
//18 19 17 5 这4个io口，数组下标0 1 2 3。
static ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
    {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = 18,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
    {
        .channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = 19,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
    {
        .channel    = LEDC_CHANNEL_2,
        .duty       = 0,
        .gpio_num   = 17,
        .speed_mode = LEDC_HS_MODE, // 建议统一为HS_MODE
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
    {
        .channel    = LEDC_CHANNEL_3,
        .duty       = 0,
        .gpio_num   = 5, // 如果你想用15，请改成15
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
};

#define LEDC_PWM_TAG "LEDC_PWM"

void ledc_motor_pwm_init(void) {
    // 配置时钟
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_HS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }
    ESP_LOGI(LEDC_PWM_TAG, "LEDC电机PWM初始化完成");
}

/*
 * @brief 设置指定通道的PWM占空比
 * @param channel 通道号（0~3）
 * @param duty 占空比（0~1023）
 */
void ledc_set_motor_pwm_duty(int channel, int duty) {
    if (channel < 0 || channel >= LEDC_TEST_CH_NUM) {
        printf("Invalid channel number:%d\n", channel);
        return;
    }
    if (duty < 0) duty = 0;
    if (duty > 1023) duty = 1023;
    ledc_set_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, duty);
    ledc_update_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel);
    ESP_LOGD(LEDC_PWM_TAG, "设置通道%d PWM占空比为%d", channel, duty);
}