// sht30_test.h
#ifndef _SHT30_TEST_H_
#define _SHT30_TEST_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "sht3x.h"       // 使用 sht3x 库，但传感器名为 SHT30

// I2C 配置宏（根据实际硬件修改引脚）
extern uint16_t SHT30_I2C_SCL_IO;      
extern uint16_t SHT30_I2C_SDA_IO;      
extern uint16_t SHT30_I2C_PORT;         
extern uint16_t SHT30_I2C_FREQ_HZ;      

extern float sht30_temperature, sht30_humidity;
// 函数声明
void sht30_init(uint16_t i2c_sda_io, uint16_t i2c_scl_io, uint16_t i2c_port, uint16_t i2c_freq_hz);
void sht30_deinit(void);
void sht30_read_data(void);

#endif // _SHT30_TEST_H_