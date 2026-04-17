#include "input_handler.h"
#include "config.h"

ButtonState g_btn_boot = {PIN_BUTTON_BOOT, false, false, 0, false, false, false, false};
ButtonState g_btn_bat  = {PIN_BUTTON_BAT,  false, false, 0, false, false, false, false};

void update_button(ButtonState &btn, uint32_t now) {
    if (btn.pin == GPIO_NUM_MAX) return;
    bool raw = (digitalRead((int)btn.pin) == LOW);
    if (raw != btn.last_raw) {
        btn.last_change_ms = now;
        btn.last_raw = raw;
    }

    if ((now - btn.last_change_ms) > 30) {
        if (raw != btn.state) {
            btn.state = raw;
            if (btn.state) {
                btn.pressed_event = true;
                btn.long_press_handled = false;
            } else {
                btn.released_event = true;
            }
        }
    }

    if (btn.state && !btn.long_press_handled && (now - btn.last_change_ms) > 1000) {
        btn.long_press_event = true;
        btn.long_press_handled = true;
    }
}
