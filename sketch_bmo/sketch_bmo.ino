#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include "config.h"
#include "globals.h"
#include "display_driver.h"
#include "input_handler.h"
#include "perf_monitor.h"
#include "scene_manager.h"
#include "face_scene.h"
#include "info_scene.h"
#include "clock_scene.h"
#include "wifi_scene.h"
#include "rtc_driver.h"
#include "imu_driver.h"

uint32_t g_last_tick = 0;
uint32_t g_last_frame = 0;
float g_cur_brightness = 255.0f;
static int g_current_scene_idx = 0;

void setup() {
    Serial.begin(115200);
    delay(500); // Give serial monitor time to connect

    Serial.println("--- B.M.O BOOTING ---");
    Serial.printf("Free heap: %u KB\n", (uint32_t)esp_get_free_heap_size() / 1024);
    Serial.printf("Free PSRAM: %u KB\n", (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

    init_i2c();
    init_imu();
    init_pmu();
    init_spi_bus();
    reset_panel();
    init_panel();
    init_lvgl_display();
    init_touch();

    pinMode((int)PIN_BUTTON_BOOT, INPUT_PULLUP);
    if (PIN_BUTTON_BAT != GPIO_NUM_MAX) {
        pinMode((int)PIN_BUTTON_BAT, INPUT_PULLUP);
    }

    randomSeed(esp_random());

    const uint32_t now = millis();
    g_last_tick = now;
    g_last_frame = now;
    
    // Start with the Face Scene
    Serial.println("Loading Face Scene...");
    SceneManager::switchScene(new FaceScene());

    if (g_display) {
        lv_refr_now(g_display);
    }
    Serial.println("B.M.O READY.");
}

void loop() {
    const uint32_t loop_start_us = (uint32_t)esp_timer_get_time();
    const uint32_t now = millis();
    const uint32_t tick_dt = now - g_last_tick;

    if (tick_dt > 0) {
        lv_tick_inc(tick_dt);
        g_last_tick = now;
    }

    update_button(g_btn_boot, now);

    // Scene switching logic (BOOT button release)
    if (g_btn_boot.released_event) {
        g_btn_boot.released_event = false;
        
        // Only switch scenes if this wasn't a long press
        if (!g_btn_boot.long_press_handled) {
            Serial.println("Switching scenes...");
            g_current_scene_idx = (g_current_scene_idx + 1) % 4;
            
            if (g_current_scene_idx == 0) {
                SceneManager::switchScene(new FaceScene());
            } else if (g_current_scene_idx == 1) {
                SceneManager::switchScene(new InfoScene());
            } else if (g_current_scene_idx == 2) {
                SceneManager::switchScene(new ClockScene());
            } else {
                SceneManager::switchScene(new WifiScene());
            }
        }
    }

    SceneManager::update(now);

    if ((now - g_last_frame) >= FRAME_MS) {
        g_last_frame = now;
        g_perf_frames++;
    }

    lv_timer_handler();

    const uint32_t loop_end_us = (uint32_t)esp_timer_get_time();
    g_perf_busy_us += (loop_end_us - loop_start_us);

    update_perf_monitor(now);

    delay(1);
}
