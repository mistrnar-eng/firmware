idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        esp_lcd_sh8601
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_spi
        fatfs
        sdmmc
        esp_timer
)
