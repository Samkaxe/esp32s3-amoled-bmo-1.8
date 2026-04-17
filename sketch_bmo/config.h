#pragma once
#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

// LCD Pins
static constexpr spi_host_device_t LCD_HOST = SPI2_HOST;
static constexpr gpio_num_t PIN_LCD_CS    = GPIO_NUM_12;
static constexpr gpio_num_t PIN_LCD_PCLK  = GPIO_NUM_11;
static constexpr gpio_num_t PIN_LCD_D0    = GPIO_NUM_4;
static constexpr gpio_num_t PIN_LCD_D1    = GPIO_NUM_5;
static constexpr gpio_num_t PIN_LCD_D2    = GPIO_NUM_6;
static constexpr gpio_num_t PIN_LCD_D3    = GPIO_NUM_7;
static constexpr gpio_num_t PIN_LCD_RESET = GPIO_NUM_MAX; // Handled by XCA9554

// Buttons
static constexpr gpio_num_t PIN_BUTTON_BOOT = GPIO_NUM_0;
static constexpr gpio_num_t PIN_BUTTON_BAT  = GPIO_NUM_MAX; // The 1.8" board might not have this exact button

// I2C Pins (RTC, Touch, PMU, IO Expander)
static constexpr gpio_num_t PIN_I2C_SDA = GPIO_NUM_15;
static constexpr gpio_num_t PIN_I2C_SCL = GPIO_NUM_14;

// Display Dimensions
static constexpr int DISPLAY_WIDTH   = 368;
static constexpr int DISPLAY_HEIGHT  = 448;
static constexpr int DISPLAY_YOFFSET = 0; // SH8601 usually doesn't need offset
static constexpr int LVGL_BUF_LINES  = 40;

// Colors
static constexpr uint32_t COLOR_BG   = 0x72cba8;
static constexpr uint32_t COLOR_FACE = 0x000000;

// Face Proportions (Scaled for 368x448)
static constexpr int LEFT_EYE_X  = 113;
static constexpr int RIGHT_EYE_X = 254;
static constexpr int EYE_Y       = 190;
static constexpr int EYE_R       = 18;
static constexpr int EYE_MOVE_X  = 10;
static constexpr int EYE_MOVE_Y  = 6;
static constexpr int EYE_OBJ_W   = 80;
static constexpr int EYE_OBJ_H   = 80;

static constexpr int SMILE_CX    = 184;
static constexpr int SMILE_CY    = 300;
static constexpr int SMILE_RX    = 35;
static constexpr int SMILE_RY    = 18;
static constexpr int SMILE_START = 20;
static constexpr int SMILE_END   = 160;
static constexpr int SMILE_WIDTH = 5;
static constexpr int SMILE_STEPS = 28;
static constexpr int MOUTH_OBJ_W = 150;
static constexpr int MOUTH_OBJ_H = 100;

// Timing
static constexpr uint32_t FRAME_MS = 16;

// Math Constants
static constexpr float PI_F = 3.14159265f;
static constexpr float DEG_TO_RAD_F = 0.0174532925f;
