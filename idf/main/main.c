#include "board_waveshare_amoled_164.h"
#include "esp_lcd_sh8601.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include <stdio.h>

static const char *TAG = "bruce-board";

static esp_err_t init_touch(void)
{
    i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&config, &bus), TAG, "I2C init failed");

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t device;
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &device_config, &device), TAG,
                        "FT3168 device setup failed");

    uint8_t register_address = 0x02;
    uint8_t touch_count = 0;
    esp_err_t result = i2c_master_transmit_receive(device, &register_address, 1,
                                                   &touch_count, 1, 100);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "FT3168 detected at 0x%02x, touches=%u", TOUCH_I2C_ADDR,
                 touch_count & 0x0f);
    }
    return result;
}

static esp_err_t init_sd(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * 1024,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO),
                        TAG, "SD SPI init failed");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = SPI3_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card = NULL;
    esp_err_t result = esp_vfs_fat_sdspi_mount("/sd", &host, &slot_config,
                                               &mount_config, &card);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "SD mounted on SPI3_HOST");
        sdmmc_card_print_info(stdout, card);
    }
    return result;
}

static esp_err_t init_lcd(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_D0,
        .miso_io_num = LCD_D1,
        .sclk_io_num = LCD_SCK,
        .quadwp_io_num = LCD_D2,
        .quadhd_io_num = LCD_D3,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO),
                        TAG, "LCD QSPI init failed");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = -1,
        .spi_mode = 0,
        .pclk_hz = 40 * 1000 * 1000,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags.quad_mode = 1,
    };
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(
                            (esp_lcd_spi_bus_handle_t)SPI2_HOST,
                            &io_config,
                            &io),
                        TAG, "LCD panel IO init failed");

    ESP_RETURN_ON_ERROR(bruce_sh8601_init(io, LCD_RST), TAG,
                        "SH8601 init failed");
    ESP_LOGI(TAG, "SH8601 init sequence sent over SPI2_HOST");
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Waveshare ESP32-S3-Touch-AMOLED-1.64 probe");
    ESP_LOGI(TAG, "LCD SH8601/CO5300 QSPI: CS=%d SCK=%d D0..D3=%d,%d,%d,%d RST=%d",
             LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_RST);

    esp_err_t touch_result = init_touch();
    if (touch_result != ESP_OK) {
        ESP_LOGE(TAG, "FT3168 probe failed: %s", esp_err_to_name(touch_result));
    }

    esp_err_t lcd_result = init_lcd();
    if (lcd_result != ESP_OK) {
        ESP_LOGE(TAG, "LCD probe failed: %s", esp_err_to_name(lcd_result));
    }

    esp_err_t sd_result = init_sd();
    if (sd_result != ESP_OK) {
        ESP_LOGE(TAG, "SD probe failed: %s", esp_err_to_name(sd_result));
    }
}
