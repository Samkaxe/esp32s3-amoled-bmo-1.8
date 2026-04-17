#pragma once
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include "esp_lcd_panel_io.h"

void init_pmu();
void init_spi_bus();
void reset_panel();
void init_panel();
void init_lvgl_display();
void init_touch();
void set_brightness(uint8_t level);
esp_err_t write_cmd(uint8_t command, const void *data = nullptr, size_t len = 0);
