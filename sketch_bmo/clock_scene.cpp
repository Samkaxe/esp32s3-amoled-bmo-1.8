#include "clock_scene.h"
#include "config.h"
#include "rtc_driver.h"
#include "imu_driver.h"
#include <stdio.h>
#include <math.h>

void ClockScene::init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    m_cont = lv_obj_create(scr);
    lv_obj_set_size(m_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(m_cont, 0, 0);
    lv_obj_set_style_bg_color(m_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(m_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_cont, 0, 0);
    lv_obj_set_style_radius(m_cont, 0, 0);
    lv_obj_clear_flag(m_cont, LV_OBJ_FLAG_SCROLLABLE);

    // IMU Chart
    m_chart = lv_chart_create(m_cont);
    lv_obj_set_size(m_chart, DISPLAY_WIDTH - 10, 350);
    lv_obj_align(m_chart, LV_ALIGN_TOP_MID, 0, 75);
    lv_chart_set_type(m_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(m_chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000); 
    lv_chart_set_point_count(m_chart, 60);
    lv_obj_set_style_bg_color(m_chart, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(m_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_chart, 1, 0);
    lv_obj_set_style_border_color(m_chart, lv_color_hex(0x004400), 0);
    
    // Green Grid
    lv_obj_set_style_line_color(m_chart, lv_color_hex(0x006600), LV_PART_MAIN);
    lv_chart_set_div_line_count(m_chart, 5, 8);
    
    lv_obj_set_style_line_width(m_chart, 1, LV_PART_ITEMS);

    // Accel Series
    m_ser_x = lv_chart_add_series(m_chart, lv_color_hex(0xFF0000), LV_CHART_AXIS_PRIMARY_Y); // Red: AX
    m_ser_y = lv_chart_add_series(m_chart, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y); // Green: AY
    m_ser_z = lv_chart_add_series(m_chart, lv_color_hex(0x0000FF), LV_CHART_AXIS_PRIMARY_Y); // Blue: AZ
    
    // Gyro Series
    m_ser_gx = lv_chart_add_series(m_chart, lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y); // Yellow: GX
    m_ser_gy = lv_chart_add_series(m_chart, lv_color_hex(0x00FFFF), LV_CHART_AXIS_PRIMARY_Y); // Cyan: GY
    m_ser_gz = lv_chart_add_series(m_chart, lv_color_hex(0xFF00FF), LV_CHART_AXIS_PRIMARY_Y); // Magenta: GZ

    m_ser_wave = lv_chart_add_series(m_chart, lv_color_hex(0xFFFFFF), LV_CHART_AXIS_PRIMARY_Y); // White: Wave

    // Legend
    lv_obj_t *legend = lv_obj_create(m_cont);
    lv_obj_set_size(legend, DISPLAY_WIDTH, 70);
    lv_obj_align(legend, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_opa(legend, 0, 0);
    lv_obj_set_style_border_width(legend, 0, 0);
    lv_obj_clear_flag(legend, LV_OBJ_FLAG_SCROLLABLE);

    auto create_legend_item = [&](const char* text, lv_color_t color, int x, int y) {
        lv_obj_t *rect = lv_obj_create(legend);
        lv_obj_set_size(rect, 8, 8);
        lv_obj_set_pos(rect, x, y + 5);
        lv_obj_set_style_bg_color(rect, color, 0);
        lv_obj_set_style_radius(rect, 2, 0);
        lv_obj_set_style_border_width(rect, 0, 0);

        lv_obj_t *label = lv_label_create(legend);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(label, x + 12, y);
    };

    // Row 1
    create_legend_item("AX", lv_color_hex(0xFF0000), 10, 0);
    create_legend_item("AY", lv_color_hex(0x00FF00), 92, 0);
    create_legend_item("AZ", lv_color_hex(0x0000FF), 174, 0);
    create_legend_item("WAVE", lv_color_hex(0xFFFFFF), 256, 0);

    // Row 2
    create_legend_item("GX", lv_color_hex(0xFFFF00), 10, 30);
    create_legend_item("GY", lv_color_hex(0x00FFFF), 92, 30);
    create_legend_item("GZ", lv_color_hex(0xFF00FF), 174, 30);

    m_last_rtc_update = 0;
    m_last_imu_update = 0;
}

void ClockScene::update(uint32_t now) {
    if (now - m_last_imu_update >= 50) {
        m_last_imu_update = now;
        ImuData data;
        if (update_imu(data)) {
            // Accelerometer: 1G = 1000mG
            lv_chart_set_next_value(m_chart, m_ser_x, (lv_coord_t)(data.ax * 1000));
            lv_chart_set_next_value(m_chart, m_ser_y, (lv_coord_t)(data.ay * 1000));
            lv_chart_set_next_value(m_chart, m_ser_z, (lv_coord_t)(data.az * 1000));

            // Gyroscope: Scale by 25 to make visible (64 dps range * 25 = 1600)
            lv_chart_set_next_value(m_chart, m_ser_gx, (lv_coord_t)(data.gx * 25));
            lv_chart_set_next_value(m_chart, m_ser_gy, (lv_coord_t)(data.gy * 25));
            lv_chart_set_next_value(m_chart, m_ser_gz, (lv_coord_t)(data.gz * 25));

            // Wave logic
            float freq_mod = (abs(data.gx) + abs(data.gy) + abs(data.gz)) * 0.01f;
            m_wave_phase += 0.2f + freq_mod;
            if (m_wave_phase > 2.0f * PI_F) m_wave_phase -= 2.0f * PI_F;
            
            float accel_mag = sqrt(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
            float motion = abs(accel_mag - 1.0f); 
            float amp = 400.0f + motion * 3000.0f;
            if (amp > 1900.0f) amp = 1900.0f;

            lv_coord_t wave_val = (lv_coord_t)(sin(m_wave_phase) * amp);
            lv_chart_set_next_value(m_chart, m_ser_wave, wave_val);
        }
    }
}

void ClockScene::deinit() {
    if (m_cont) {
        lv_obj_del(m_cont);
        m_cont = nullptr;
    }
}
