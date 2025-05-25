// sht30_test.c
#include "sht30.h"
#include "esp_log.h"

static const char *TAG = "SHT30";

static i2c_bus_handle_t i2c_bus = NULL; // I2C 总线句柄
static sht3x_handle_t sht30 = NULL;     // 传感器句柄（库名为 sht3x，实际传感器为 SHT30）

uint16_t SHT30_I2C_SCL_IO = 15;      // SCL 引脚
uint16_t SHT30_I2C_SDA_IO = 2;       // SDA 引脚
uint16_t SHT30_I2C_PORT = I2C_NUM_0; // I2C_NUM_0
uint16_t SHT30_I2C_FREQ_HZ = 100000; // 100 kHz
float sht30_temperature, sht30_humidity;
// 初始化 I2C 和传感器
void sht30_init(uint16_t i2c_sda_io, uint16_t i2c_scl_io, uint16_t i2c_port, uint16_t i2c_freq_hz)
{
    // 配置 I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_sda_io,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = i2c_scl_io,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2c_freq_hz,
    };

    // 创建 I2C 总线
    i2c_bus = i2c_bus_create(i2c_port, &conf);
    if (i2c_bus == NULL)
    {
        ESP_LOGE(TAG, "I2C 总线创建失败！");
        return;
    }

    // 创建 SHT30 传感器对象（使用 sht3x 库）
    sht30 = sht3x_create(i2c_bus, SHT3x_ADDR_PIN_SELECT_VSS);
    if (sht30 == NULL)
    {
        ESP_LOGE(TAG, "SHT30 传感器初始化失败！");
        return;
    }

    // 配置测量模式
    sht3x_set_measure_mode(sht30, SHT3x_PER_2_MEDIUM);
    ESP_LOGI(TAG, "SHT30 初始化完成");
}

// 释放资源
void sht30_deinit(void)
{
    if (sht30)
    {
        sht3x_delete(&sht30); // 删除传感器对象
        ESP_LOGI(TAG, "SHT30 资源已释放");
    }
    if (i2c_bus)
    {
        i2c_bus_delete(&i2c_bus); // 删除 I2C 总线
        ESP_LOGI(TAG, "I2C 总线已释放");
    }
}

// 读取并打印数据
void sht30_read_data(void)
{

    int retry_count = 10;

    while (retry_count--)
    {
        if (sht3x_get_humiture(sht30, &sht30_temperature, &sht30_humidity) == ESP_OK)
        {
            ESP_LOGI(TAG, "温度: %.2f°C, 湿度: %.2f%%", sht30_temperature, sht30_humidity);
        }
        else
        {
            ESP_LOGE(TAG, "数据读取失败");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1秒间隔
    }
}