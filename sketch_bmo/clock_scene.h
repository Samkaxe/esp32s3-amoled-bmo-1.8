#pragma once
#include "scene_manager.h"
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>

class ClockScene : public Scene {
public:
    void init() override;
    void update(uint32_t now) override;
    void deinit() override;

private:
    lv_obj_t *m_cont = nullptr;
    lv_obj_t *m_time_label = nullptr;
    lv_obj_t *m_date_label = nullptr;
    lv_obj_t *m_bmo_label = nullptr; // A small text-based BMO
    lv_obj_t *m_chart = nullptr;
    lv_chart_series_t *m_ser_x = nullptr;
    lv_chart_series_t *m_ser_y = nullptr;
    lv_chart_series_t *m_ser_z = nullptr;
    lv_chart_series_t *m_ser_wave = nullptr;
    
    uint32_t m_last_rtc_update = 0;
    uint32_t m_last_imu_update = 0;
    float m_wave_phase = 0;
};
