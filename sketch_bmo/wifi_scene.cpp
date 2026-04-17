#include "wifi_scene.h"
#include "config.h"
#include "globals.h"
#include "esp_heap_caps.h"
#include <WiFi.h>

// WiFi scan constants
#define RSSI_CEILING -40
#define RSSI_FLOOR -100

static const uint32_t channel_hex_colors[] = {
    0xFF0000, // RED
    0xFF8000, // ORANGE
    0xFFFF00, // YELLOW
    0x00FF00, // GREEN
    0x00FFFF, // CYAN
    0x0000FF, // BLUE
    0xFF00FF, // MAGENTA
    0xFF0000, // RED
    0xFF8000, // ORANGE
    0xFFFF00, // YELLOW
    0x00FF00, // GREEN
    0x00FFFF, // CYAN
    0x0000FF, // BLUE
    0xFF00FF  // MAGENTA
};

void WifiScene::init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    m_cont = lv_obj_create(scr);
    lv_obj_set_size(m_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(m_cont, 0, 0);
    lv_obj_set_style_bg_color(m_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(m_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_cont, 0, 0);
    lv_obj_set_style_pad_all(m_cont, 0, 0);
    lv_obj_clear_flag(m_cont, LV_OBJ_FLAG_SCROLLABLE);

    m_graph_obj = lv_obj_create(m_cont);
    lv_obj_set_size(m_graph_obj, DISPLAY_WIDTH, 260);
    lv_obj_align(m_graph_obj, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(m_graph_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(m_graph_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_graph_obj, 0, 0);
    lv_obj_add_event_cb(m_graph_obj, draw_event_cb, LV_EVENT_DRAW_MAIN, this);

    m_label_status = lv_label_create(m_cont);
    lv_label_set_text(m_label_status, "B.M.O WIFI ANALYZER - INITIALIZING...");
    lv_obj_set_style_text_color(m_label_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(m_label_status, &lv_font_montserrat_14, 0);
    lv_obj_set_width(m_label_status, DISPLAY_WIDTH);
    lv_obj_set_style_text_align(m_label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(m_label_status, LV_ALIGN_TOP_MID, 0, 15);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    start_scan();
}

void WifiScene::deinit() {
    WiFi.mode(WIFI_OFF);
    if (m_cont) {
        lv_obj_del(m_cont);
        m_cont = nullptr;
    }
}

void WifiScene::start_scan() {
    WiFi.scanNetworks(true, true, false, 300); // Async ACTIVE scan
    m_scan_active = true;
    lv_label_set_text(m_label_status, "SCANNING...");
}

void WifiScene::update(uint32_t now) {
    if (m_scan_active) {
        int16_t n = WiFi.scanComplete();
        if (n >= 0) {
            m_scan_active = false;
            m_last_scan_ms = now;
            
            if (m_graph_obj) lv_obj_invalidate(m_graph_obj);
            
            char buf[64];
            snprintf(buf, sizeof(buf), "B.M.O WIFI ANALYZER - %d NETWORKS", n);
            lv_label_set_text(m_label_status, buf);
            Serial.printf("WiFi Scan Complete: %d networks found\n", n);
        }
    } else if (now - m_last_scan_ms > 5000) { // Scan every 5s to avoid bus congestion
        start_scan();
    }
}

void WifiScene::draw_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    
    int n = WiFi.scanComplete();
    if (n <= 0) return;

    static uint32_t last_debug_ms = 0;
    bool show_debug = (millis() - last_debug_ms > 5000);
    if (show_debug) {
        last_debug_ms = millis();
        Serial.printf("Drawing WiFi Spectrum: %d networks\n", n);
    }

    int w = lv_obj_get_width(obj);
    int h = lv_obj_get_height(obj);
    
    lv_area_t obj_coords;
    lv_obj_get_coords(obj, &obj_coords);

    if (show_debug) {
        Serial.printf("Graph Obj Coords: x1=%d, y1=%d, w=%d, h=%d\n", obj_coords.x1, obj_coords.y1, w, h);
    }

    int graph_baseline = h - 30;
    int graph_height = graph_baseline - 60;
    int channel_width = w / 17;
    int signal_width = channel_width * 2;

    // Draw baseline
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(0xFFFFFF);
    line_dsc.width = 1;
    
    lv_point_t p1 = {obj_coords.x1, (lv_coord_t)(obj_coords.y1 + graph_baseline)};
    lv_point_t p2 = {obj_coords.x2, (lv_coord_t)(obj_coords.y1 + graph_baseline)};
    lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);

    // Track peaks
    int peak_rssi[14];
    int peak_id[14];
    for(int i=0; i<14; i++) { peak_rssi[i] = RSSI_FLOOR; peak_id[i] = -1; }

    for (int i = 0; i < n; i++) {
        int ch = WiFi.channel(i);
        if (ch < 1 || ch > 14) continue;
        int32_t rssi = WiFi.RSSI(i);
        if (rssi > peak_rssi[ch-1]) {
            peak_rssi[ch-1] = rssi;
            peak_id[ch-1] = i;
        }
    }

    // Draw signals
    for (int i = 0; i < n; i++) {
        int ch = WiFi.channel(i);
        if (ch < 1 || ch > 14) continue;
        int idx = ch - 1;
        int32_t rssi = WiFi.RSSI(i);
        int height = map(constrain(rssi, RSSI_FLOOR, RSSI_CEILING), RSSI_FLOOR, RSSI_CEILING, 5, graph_height);
        int offset = (ch + 1) * channel_width;

        line_dsc.color = lv_color_hex(channel_hex_colors[idx]);
        line_dsc.width = 2;
        
        lv_point_t pa = {(lv_coord_t)(obj_coords.x1 + offset - signal_width), (lv_coord_t)(obj_coords.y1 + graph_baseline)};
        lv_point_t pb = {(lv_coord_t)(obj_coords.x1 + offset), (lv_coord_t)(obj_coords.y1 + graph_baseline - height)};
        lv_point_t pc = {(lv_coord_t)(obj_coords.x1 + offset + signal_width), (lv_coord_t)(obj_coords.y1 + graph_baseline)};
        
        lv_draw_line(draw_ctx, &line_dsc, &pa, &pb);
        lv_draw_line(draw_ctx, &line_dsc, &pb, &pc);

        if (show_debug && i == 0) {
            Serial.printf("Sample Signal: Ch=%d, RSSI=%d, x=%d, y=%d\n", ch, (int)rssi, (int)pb.x, (int)pb.y);
        }

        if (i == peak_id[idx]) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) ssid = WiFi.BSSIDstr(i);
            
            lv_draw_label_dsc_t lbl_dsc;
            lv_draw_label_dsc_init(&lbl_dsc);
            lbl_dsc.color = lv_color_hex(channel_hex_colors[idx]);
            lbl_dsc.font = &lv_font_montserrat_14;
            
            lv_area_t label_area;
            label_area.x1 = obj_coords.x1 + offset - 40;
            label_area.y1 = obj_coords.y1 + graph_baseline - height - 20;
            label_area.x2 = obj_coords.x1 + offset + 60;
            label_area.y2 = obj_coords.y1 + graph_baseline - height;
            
            lv_draw_label(draw_ctx, &lbl_dsc, &label_area, ssid.c_str(), NULL);
        }
    }

    // Draw channel numbers
    for (int ch = 1; ch <= 14; ch++) {
        int offset = (ch + 1) * channel_width;
        lv_draw_label_dsc_t ch_lbl;
        lv_draw_label_dsc_init(&ch_lbl);
        ch_lbl.color = lv_color_hex(channel_hex_colors[ch-1]);
        ch_lbl.font = &lv_font_montserrat_14;
        
        char ch_buf[4];
        snprintf(ch_buf, sizeof(ch_buf), "%d", ch);
        
        lv_area_t ch_area;
        ch_area.x1 = obj_coords.x1 + offset - 5;
        ch_area.y1 = obj_coords.y1 + graph_baseline + 5;
        ch_area.x2 = obj_coords.x1 + offset + 25;
        ch_area.y2 = obj_coords.y1 + graph_baseline + 25;
        
        lv_draw_label(draw_ctx, &ch_lbl, &ch_area, ch_buf, NULL);
    }
}
