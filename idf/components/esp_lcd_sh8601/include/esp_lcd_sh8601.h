#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_err.h"

esp_err_t bruce_sh8601_init(
    esp_lcd_panel_io_handle_t io,
    int reset_gpio
);
