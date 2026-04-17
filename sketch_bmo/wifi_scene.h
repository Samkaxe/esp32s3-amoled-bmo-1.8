#pragma once
#include "scene_manager.h"
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include <WiFi.h>

class WifiScene : public Scene {
public:
    void init() override;
    void update(uint32_t now) override;
    void deinit() override;

private:
    lv_obj_t *m_cont = nullptr;
    lv_obj_t *m_label_status = nullptr;
    lv_obj_t *m_graph_obj = nullptr;
    
    uint32_t m_last_scan_ms = 0;
    bool m_scan_active = false;
    
    void draw_analyzer();
    void start_scan();
    
    static void draw_event_cb(lv_event_t *e);
};
