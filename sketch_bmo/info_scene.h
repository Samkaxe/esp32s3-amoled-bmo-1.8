#pragma once
#include "scene_manager.h"
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>

class InfoScene : public Scene {
public:
    void init() override;
    void update(uint32_t now) override;
    void deinit() override;

private:
    lv_obj_t *g_main_cont = nullptr;
    lv_obj_t *g_label_info = nullptr;
    
    lv_obj_t *m_chart = nullptr;
    lv_chart_series_t *m_ser_fps = nullptr;
    lv_chart_series_t *m_ser_cpu = nullptr;
    lv_chart_series_t *m_ser_ram = nullptr;
};
