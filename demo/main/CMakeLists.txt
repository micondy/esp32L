idf_component_register(SRCS "ultrasonic.c" "ledc_motor_pwm.c" "tcp_client.c" "wifi_manager.c" "My_nvs_storage.c" "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_wifi wpa_supplicant nvs_flash json esp_driver_ledc esp_driver_gpio esp_timer
                    REQUIRES 
                    INCLUDE_DIRS "")
