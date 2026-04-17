#include "clock_scene.h"
#include "config.h"
#include "rtc_driver.h"
#include "imu_driver.h"
#include <stdio.h>
#include <math.h>

void ClockScene::init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    m_cont = lv_obj_create(scr);
    lv_obj_set_size(m_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(m_cont, 0, 0);
    lv_obj_set_style_bg_color(m_cont, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(m_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_cont, 0, 0);
    lv_obj_set_style_radius(m_cont, 0, 0);
    lv_obj_clear_flag(m_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Time Label
    m_time_label = lv_label_create(m_cont);
    lv_label_set_text(m_time_label, "00:00:00");
    lv_obj_set_style_text_color(m_time_label, lv_color_hex(COLOR_FACE), 0);
    lv_obj_set_style_text_font(m_time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(m_time_label, LV_ALIGN_CENTER, 0, -20);

    // Date Label
    m_date_label = lv_label_create(m_cont);
    lv_label_set_text(m_date_label, "Loading...");
    lv_obj_set_style_text_color(m_date_label, lv_color_hex(COLOR_FACE), 0);
    lv_obj_set_style_text_font(m_date_label, &lv_font_montserrat_14, 0);
    lv_obj_align(m_date_label, LV_ALIGN_CENTER, 0, 20);

    // BMO expression
    m_bmo_label = lv_label_create(m_cont);
    lv_label_set_text(m_bmo_label, "(^_^) B.M.O CLOCK");
    lv_obj_set_style_text_color(m_bmo_label, lv_color_hex(COLOR_FACE), 0);
    lv_obj_set_style_text_font(m_bmo_label, &lv_font_montserrat_14, 0);
    lv_obj_align(m_bmo_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    // IMU Chart
    m_chart = lv_chart_create(m_cont);
    lv_obj_set_size(m_chart, DISPLAY_WIDTH - 40, 150);
    lv_obj_align(m_chart, LV_ALIGN_TOP_MID, 0, 40);
    lv_chart_set_type(m_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(m_chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000); // For Accelerometer in mG
    lv_chart_set_point_count(m_chart, 50);
    lv_obj_set_style_bg_opa(m_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m_chart, 0, 0);
    lv_obj_set_style_line_width(m_chart, 2, LV_PART_ITEMS);

    m_ser_x = lv_chart_add_series(m_chart, lv_color_hex(0xFF0000), LV_CHART_AXIS_PRIMARY_Y); // Red: X
    m_ser_y = lv_chart_add_series(m_chart, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y); // Green: Y
    m_ser_z = lv_chart_add_series(m_chart, lv_color_hex(0x0000FF), LV_CHART_AXIS_PRIMARY_Y); // Blue: Z
    m_ser_wave = lv_chart_add_series(m_chart, lv_color_hex(0xFFFFFF), LV_CHART_AXIS_PRIMARY_Y); // White: Modulated Wave

    m_last_rtc_update = 0;
    m_last_imu_update = 0;
}

void ClockScene::update(uint32_t now) {
    if (now - m_last_rtc_update >= 1000) {
        m_last_rtc_update = now;
        BmoTime tm;
        if (get_rtc_time(tm)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.hour, tm.minute, tm.second);
            lv_label_set_text(m_time_label, buf);

            static const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            static const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
            
            snprintf(buf, sizeof(buf), "%s, %s %d %d", days[tm.week % 7], months[(tm.month - 1) % 12], tm.day, tm.year);
            lv_label_set_text(m_date_label, buf);
        }
    }

    if (now - m_last_imu_update >= 20) {
        m_last_imu_update = now;
        ImuData data;
        if (update_imu(data)) {
            // Accelerometer values are in G's, convert to mG for the chart range (-2000 to 2000)
            lv_chart_set_next_value(m_chart, m_ser_x, (lv_coord_t)(data.ax * 1000));
            lv_chart_set_next_value(m_chart, m_ser_y, (lv_coord_t)(data.ay * 1000));
            lv_chart_set_next_value(m_chart, m_ser_z, (lv_coord_t)(data.az * 1000));

            // Generate a modulated sine wave
            // Base phase increment + Gyro influence
            float freq_mod = (abs(data.gx) + abs(data.gy) + abs(data.gz)) * 0.01f;
            m_wave_phase += 0.2f + freq_mod;
            if (m_wave_phase > 2.0f * PI_F) m_wave_phase -= 2.0f * PI_F;
            
            // Amplitude modulated by motion (deviation from 1G gravity)
            float accel_mag = sqrt(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
            float motion = abs(accel_mag - 1.0f); 
            float amp = 400.0f + motion * 3000.0f;
            if (amp > 1900.0f) amp = 1900.0f;

            lv_coord_t wave_val = (lv_coord_t)(sin(m_wave_phase) * amp);
            lv_chart_set_next_value(m_chart, m_ser_wave, wave_val);
            
            // "Awesome" Tilt effect: rotate labels based on X-axis acceleration
            // Angle is in 0.1 degree units (900 = 90 degrees)
            int16_t tilt_angle = (int16_t)(data.ax * -150.0f); // Up to ~15 degrees
            lv_obj_set_style_transform_angle(m_time_label, tilt_angle, 0);
            lv_obj_set_style_transform_angle(m_date_label, tilt_angle, 0);
            lv_obj_set_style_transform_angle(m_bmo_label, tilt_angle, 0);

            // Change BMO expression based on motion
            if (motion > 0.5f) {
                lv_label_set_text(m_bmo_label, "(>_<) WHOA!");
            } else if (abs(data.ax) > 0.3f) {
                lv_label_set_text(m_bmo_label, "(o_o) TILTING...");
            } else {
                lv_label_set_text(m_bmo_label, "(^_^) B.M.O CLOCK");
            }
        }
    }
}

void ClockScene::deinit() {
    if (m_cont) {
        lv_obj_del(m_cont);
        m_cont = nullptr;
    }
}
