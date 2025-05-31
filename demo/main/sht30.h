// sht30_test.h
#ifndef _SHT30_TEST_H_
#define _SHT30_TEST_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "sht3x.h"       // 使用 sht3x 库，但传感器名为 SHT30

// I2C 配置参数（可根据实际硬件修改）
extern uint16_t SHT30_I2C_SCL_IO;
extern uint16_t SHT30_I2C_SDA_IO;
extern uint16_t SHT30_I2C_PORT;
extern uint32_t SHT30_I2C_FREQ_HZ;

// 当前温湿度值
extern float sht30_temperature, sht30_humidity;

// SHT30初始化，返回ESP_OK/ESP_FAIL
esp_err_t sht30_init(uint16_t i2c_sda_io, uint16_t i2c_scl_io, uint16_t i2c_port, uint32_t i2c_freq_hz);

// 释放SHT30和I2C资源
void sht30_deinit(void);

// 读取一次温湿度数据（需先初始化）
void sht30_read_data(void);

#endif // _SHT30_TEST_H_