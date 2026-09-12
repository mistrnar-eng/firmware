#include "esp_lcd_sh8601.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_rom_sys.h"

#define SH8601_SWRESET 0x01
#define SH8601_SLPOUT 0x11
#define SH8601_NORON 0x13
#define SH8601_INVOFF 0x20
#define SH8601_PIXFMT 0x3a
#define SH8601_DISPON 0x29
#define SH8601_WDBRIGHTNESS 0x51
#define SH8601_WCTRLD1 0x53

static esp_err_t send_command(esp_lcd_panel_io_handle_t io, uint8_t command,
                              const uint8_t *data, size_t length)
{
    return esp_lcd_panel_io_tx_param(io, command, data, length);
}

esp_err_t bruce_sh8601_init(esp_lcd_panel_io_handle_t io, int reset_gpio)
{
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, "sh8601", "null IO");

    if (reset_gpio >= 0) {
        gpio_config_t reset_config = {
            .pin_bit_mask = 1ULL << reset_gpio,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&reset_config), "sh8601", "reset GPIO");
        gpio_set_level(reset_gpio, 1);
        esp_rom_delay_us(10000);
        gpio_set_level(reset_gpio, 0);
        esp_rom_delay_us(200000);
        gpio_set_level(reset_gpio, 1);
        esp_rom_delay_us(200000);
    }

    ESP_RETURN_ON_ERROR(send_command(io, SH8601_SWRESET, NULL, 0), "sh8601", "reset");
    esp_rom_delay_us(120000);
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_SLPOUT, NULL, 0), "sh8601", "sleep out");
    esp_rom_delay_us(120000);
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_NORON, NULL, 0), "sh8601", "normal mode");
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_INVOFF, NULL, 0), "sh8601", "inversion");

    const uint8_t pixel_format = 0x05;
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_PIXFMT, &pixel_format, 1),
                        "sh8601", "pixel format");

    const uint8_t control_1 = 0x28;
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_WCTRLD1, &control_1, 1),
                        "sh8601", "control");
    const uint8_t brightness = 0xd0;
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_WDBRIGHTNESS, &brightness, 1),
                        "sh8601", "brightness");
    ESP_RETURN_ON_ERROR(send_command(io, SH8601_DISPON, NULL, 0), "sh8601", "display on");
    esp_rom_delay_us(10000);
    return ESP_OK;
}
