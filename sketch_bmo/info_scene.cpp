#include "info_scene.h"
#include "config.h"
#include "globals.h"
#include "perf_monitor.h"
#include "esp_heap_caps.h"

void InfoScene::init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    g_main_cont = lv_obj_create(scr);
    lv_obj_set_size(g_main_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(g_main_cont, 0, 0);
    lv_obj_set_style_bg_color(g_main_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_main_cont, 0, 0);
    lv_obj_set_style_pad_all(g_main_cont, 0, 0);
    lv_obj_clear_flag(g_main_cont, LV_OBJ_FLAG_SCROLLABLE);

    g_label_info = lv_label_create(g_main_cont);
    lv_label_set_text(g_label_info, "INIT...");
    lv_obj_set_style_text_color(g_label_info, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_label_info, &lv_font_montserrat_14, 0);
    lv_obj_set_width(g_label_info, DISPLAY_WIDTH);
    lv_obj_set_style_text_align(g_label_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_label_info, LV_ALIGN_TOP_MID, 0, 15);

    // Create Chart
    m_chart = lv_chart_create(g_main_cont);
    lv_obj_set_size(m_chart, DISPLAY_WIDTH, DISPLAY_HEIGHT - 60);
    lv_obj_align(m_chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(m_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(m_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(m_chart, 300); // Higher density
    lv_chart_set_range(m_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    
    // Grid and Div lines
    lv_obj_set_style_bg_color(m_chart, lv_color_hex(0x000000), 0);
    lv_obj_set_style_line_color(m_chart, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_chart_set_div_line_count(m_chart, 5, 10);
    
    // Aesthetic: Hide points, show only lines
    lv_obj_set_style_size(m_chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(m_chart, 0, 0);
    lv_obj_set_style_pad_all(m_chart, 0, 0);

    // Add series
    m_ser_fps = lv_chart_add_series(m_chart, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y); // FPS - GREEN
    m_ser_cpu = lv_chart_add_series(m_chart, lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y); // CPU - YELLOW
    m_ser_ram = lv_chart_add_series(m_chart, lv_color_hex(0x00FFFF), LV_CHART_AXIS_PRIMARY_Y); // RAM - CYAN

    // Clear initial points
    for(int i=0; i<300; i++) {
        lv_chart_set_next_value(m_chart, m_ser_fps, 0);
        lv_chart_set_next_value(m_chart, m_ser_cpu, 0);
        lv_chart_set_next_value(m_chart, m_ser_ram, 0);
    }
}

void InfoScene::update(uint32_t now) {
    static uint32_t last_chart_upd = 0;
    if (now - last_chart_upd >= 200) { // Update chart every 200ms
        last_chart_upd = now;

        // Scale values to 0-100 range for the chart
        int fps_val = (int)g_last_fps;
        if (fps_val > 100) fps_val = 100;
        
        int cpu_val = (int)g_last_cpu_load;
        if (cpu_val > 100) cpu_val = 100;

        // Internal DRAM is ~320KB, but total heap includes PSRAM (8MB)
        // For the chart, we'll monitor the *Internal* heap usage since it's most critical
        size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t total_int = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        int ram_val = 100 - (int)((free_int * 100) / total_int);
        if (ram_val < 0) ram_val = 0;
        if (ram_val > 100) ram_val = 100;

        lv_chart_set_next_value(m_chart, m_ser_fps, fps_val);
        lv_chart_set_next_value(m_chart, m_ser_cpu, cpu_val);
        lv_chart_set_next_value(m_chart, m_ser_ram, ram_val);

        uint32_t uptime_s = now / 1000;
        uint32_t hh = uptime_s / 3600;
        uint32_t mm = (uptime_s % 3600) / 60;
        uint32_t ss = uptime_s % 60;

        char buf[256];
        snprintf(buf, sizeof(buf), 
            "#00FF00 FPS: %.0f#     #FFFF00 CPU: %.1f%%#     #00FFFF RAM: %.1f%%#",
            g_last_fps, g_last_cpu_load, (float)(total_int - free_int) / (total_int / 100.0f)
        );
        lv_label_set_recolor(g_label_info, true);
        lv_label_set_text(g_label_info, buf);
    }
}

void InfoScene::deinit() {
    if (g_main_cont) {
        lv_obj_del(g_main_cont);
        g_main_cont = nullptr;
    }
}
