#pragma once
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include "esp_lcd_panel_io.h"

extern esp_lcd_panel_io_handle_t g_panel_io;
extern lv_disp_t *g_display;

extern uint32_t g_last_tick;
extern uint32_t g_last_frame;

extern float g_cur_brightness;
