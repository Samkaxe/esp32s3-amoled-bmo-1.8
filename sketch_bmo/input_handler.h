#pragma once
#include <Arduino.h>
#include "driver/gpio.h"

struct ButtonState {
    gpio_num_t pin;
    bool last_raw;
    bool state;
    uint32_t last_change_ms;
    bool pressed_event;
    bool released_event;
    bool long_press_event;
    bool long_press_handled;
};

extern ButtonState g_btn_boot;
extern ButtonState g_btn_bat;

void update_button(ButtonState &btn, uint32_t now);
