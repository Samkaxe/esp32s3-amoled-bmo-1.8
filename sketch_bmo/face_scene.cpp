#include "face_scene.h"
#include "config.h"
#include "utils.h"
#include "globals.h"
#include "display_driver.h"
#include "input_handler.h"
#include <math.h>

// Internal drawing helpers
static void draw_mouth(lv_draw_ctx_t *dc, int origin_x, int origin_y, float progress) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_hex(COLOR_FACE);
    dsc.opa = LV_OPA_COVER;
    dsc.width = SMILE_WIDTH;
    dsc.round_start = 1;
    dsc.round_end = 1;

    if (progress <= 0.01f) {
        // Simple normal smile
        for (int i = 0; i < SMILE_STEPS; i++) {
            const float t1 = (float)i / (float)SMILE_STEPS;
            const float t2 = (float)(i + 1) / (float)SMILE_STEPS;
            const float a1 = (float)(SMILE_START + ((SMILE_END - SMILE_START) * t1)) * DEG_TO_RAD_F;
            const float a2 = (float)(SMILE_START + ((SMILE_END - SMILE_START) * t2)) * DEG_TO_RAD_F;
            lv_point_t p1 = {(lv_coord_t)lroundf((float)origin_x + (float)(MOUTH_OBJ_W / 2) + cosf(a1) * (float)SMILE_RX),
                             (lv_coord_t)lroundf((float)origin_y + (float)(MOUTH_OBJ_H / 2) + sinf(a1) * (float)SMILE_RY)};
            lv_point_t p2 = {(lv_coord_t)lroundf((float)origin_x + (float)(MOUTH_OBJ_W / 2) + cosf(a2) * (float)SMILE_RX),
                             (lv_coord_t)lroundf((float)origin_y + (float)(MOUTH_OBJ_H / 2) + sinf(a2) * (float)SMILE_RY)};
            lv_draw_line(dc, &dsc, &p1, &p2);
        }
    } else {
        // Transition to cat mouth 'w'
        const float w_rx = lerpf((float)SMILE_RX, (float)SMILE_RX * 0.45f, progress);
        const float w_ry = lerpf((float)SMILE_RY, (float)SMILE_RY * 1.20f, progress);
        const float offset_x = lerpf(0.0f, w_rx * 0.9f, progress);
        const float start_a = lerpf((float)SMILE_START, 10.0f, progress);
        const float end_a = lerpf((float)SMILE_END, 170.0f, progress);

        // Two arcs
        for (int side = 0; side < 2; side++) {
            const float cx = (float)origin_x + (float)(MOUTH_OBJ_W / 2) + (side == 0 ? -offset_x : offset_x);
            for (int i = 0; i < SMILE_STEPS / 2; i++) {
                const float t1 = (float)i / (float)(SMILE_STEPS / 2);
                const float t2 = (float)(i + 1) / (float)(SMILE_STEPS / 2);
                const float a1 = (float)(start_a + ((end_a - start_a) * t1)) * DEG_TO_RAD_F;
                const float a2 = (float)(start_a + ((end_a - start_a) * t2)) * DEG_TO_RAD_F;
                lv_point_t p1 = {(lv_coord_t)lroundf(cx + cosf(a1) * w_rx), (lv_coord_t)lroundf((float)origin_y + (float)(MOUTH_OBJ_H / 2) + sinf(a1) * w_ry)};
                lv_point_t p2 = {(lv_coord_t)lroundf(cx + cosf(a2) * w_rx), (lv_coord_t)lroundf((float)origin_y + (float)(MOUTH_OBJ_H / 2) + sinf(a2) * w_ry)};
                lv_draw_line(dc, &dsc, &p1, &p2);
            }
        }
    }
}

static void mouth_draw_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *dc = lv_event_get_draw_ctx(e);
    FaceScene *scene = (FaceScene *)lv_event_get_user_data(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    draw_mouth(dc, coords.x1, coords.y1, scene ? scene->get_pose_progress() : 0.0f);
}

static void face_touch_cb(lv_event_t *e) {
    FaceScene *scene = (FaceScene *)lv_event_get_user_data(e);
    if (scene) {
        scene->toggle_love_pose();
    }
}

static void style_overlay_obj(lv_obj_t *obj) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

// Scene methods
void FaceScene::init() {
    const size_t buf_size = EYE_OBJ_W * EYE_OBJ_H * sizeof(lv_color_t);
    m_left_eye_canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!m_left_eye_canvas_buf) {
        m_left_eye_canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

    m_right_eye_canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!m_right_eye_canvas_buf) {
        m_right_eye_canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

    create_ui();
    schedule_idle_events(millis());
}

void FaceScene::deinit() {
    if (m_face) {
        lv_obj_del(m_face);
        m_face = nullptr;
    }
    if (m_left_eye_canvas_buf) {
        heap_caps_free(m_left_eye_canvas_buf);
        m_left_eye_canvas_buf = nullptr;
    }
    if (m_right_eye_canvas_buf) {
        heap_caps_free(m_right_eye_canvas_buf);
        m_right_eye_canvas_buf = nullptr;
    }
}

void FaceScene::update(uint32_t now) {
    // Smoothly animate pose transition
    float target_progress = (m_pose == FacePose::Love) ? 1.0f : 0.0f;
    if (m_pose_progress != target_progress) {
        m_pose_progress = lerpf(m_pose_progress, target_progress, 0.08f);
        if (fabsf(m_pose_progress - target_progress) < 0.01f) m_pose_progress = target_progress;
        lv_obj_invalidate(m_mouth_obj); // Force mouth redraw during transition
    }

    animate_face(now);
    
    if (now - g_last_frame >= FRAME_MS) {
        if (m_left_eye_canvas_buf) redraw_eye_canvas(m_left_eye_obj);
        if (m_right_eye_canvas_buf) redraw_eye_canvas(m_right_eye_obj);
        lv_obj_invalidate(m_left_eye_obj);
        lv_obj_invalidate(m_right_eye_obj);
    }
}

void FaceScene::create_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    m_face = lv_obj_create(scr);
    lv_obj_set_size(m_face, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(m_face, 0, 0);
    lv_obj_add_flag(m_face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(m_face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(m_face, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(m_face, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_face, 0, 0);
    lv_obj_set_style_outline_width(m_face, 0, 0);
    lv_obj_set_style_pad_all(m_face, 0, 0);
    lv_obj_set_style_radius(m_face, 0, 0);
    lv_obj_add_event_cb(m_face, face_touch_cb, LV_EVENT_CLICKED, this);

    m_left_eye_obj = lv_canvas_create(m_face);
    lv_obj_set_size(m_left_eye_obj, EYE_OBJ_W, EYE_OBJ_H);
    lv_obj_set_pos(m_left_eye_obj, LEFT_EYE_X - (EYE_OBJ_W / 2), EYE_Y - (EYE_OBJ_H / 2));
    style_overlay_obj(m_left_eye_obj);
    if (m_left_eye_canvas_buf) {
        lv_canvas_set_buffer(m_left_eye_obj, m_left_eye_canvas_buf, EYE_OBJ_W, EYE_OBJ_H, LV_IMG_CF_TRUE_COLOR);
    }

    m_right_eye_obj = lv_canvas_create(m_face);
    lv_obj_set_size(m_right_eye_obj, EYE_OBJ_W, EYE_OBJ_H);
    lv_obj_set_pos(m_right_eye_obj, RIGHT_EYE_X - (EYE_OBJ_W / 2), EYE_Y - (EYE_OBJ_H / 2));
    style_overlay_obj(m_right_eye_obj);
    if (m_right_eye_canvas_buf) {
        lv_canvas_set_buffer(m_right_eye_obj, m_right_eye_canvas_buf, EYE_OBJ_W, EYE_OBJ_H, LV_IMG_CF_TRUE_COLOR);
    }

    m_mouth_obj = lv_obj_create(m_face);
    lv_obj_set_size(m_mouth_obj, MOUTH_OBJ_W, MOUTH_OBJ_H);
    lv_obj_set_pos(m_mouth_obj, SMILE_CX - (MOUTH_OBJ_W / 2), SMILE_CY - (MOUTH_OBJ_H / 2));
    style_overlay_obj(m_mouth_obj);
    lv_obj_add_event_cb(m_mouth_obj, mouth_draw_cb, LV_EVENT_DRAW_MAIN, this);
}

void FaceScene::animate_face(uint32_t now) {
    if (g_btn_boot.pressed_event) {
        g_btn_boot.pressed_event = false;
        Serial.println("BOOT button pressed!");
        if (m_state == FaceState::Idle) enter_blink(now);
        else if (m_state == FaceState::Sleeping) trigger_wake(now);
    }

    if (g_btn_boot.long_press_event) {
        g_btn_boot.long_press_event = false;
        Serial.println("BOOT button long press!");
        if (m_state == FaceState::Idle) {
            m_state = FaceState::Sleeping;
            m_state_start_ms = now;
            m_wake_at_ms = 0xFFFFFFFF;
            start_eye_move(0.0f, 0.0f, 900, now);
            Serial.println("B.M.O is going to sleep...");
        } else if (m_state == FaceState::Sleeping) trigger_wake(now);
    }


    if (m_state == FaceState::Idle) update_idle(now);
    else if (m_state == FaceState::Blinking) update_blinking(now);
    else if (m_state == FaceState::Sleeping) update_sleeping(now);
    else update_waking(now);

    float target_br = (m_state == FaceState::Sleeping) ? 40.0f : 255.0f;
    if (fabsf(g_cur_brightness - target_br) > 0.5f) {
        const uint8_t old_br = (uint8_t)lroundf(g_cur_brightness);
        g_cur_brightness = lerpf(g_cur_brightness, target_br, 0.04f);
        const uint8_t new_br = (uint8_t)lroundf(g_cur_brightness);
        if (new_br != old_br) set_brightness(new_br);
    }
}

void FaceScene::update_idle(uint32_t now) {
    m_eye_open = 1.0f;
    update_eye_move(now);
    if (!m_eye_move_active && now >= m_next_look_ms) {
        start_eye_move(rand_range(-EYE_MOVE_X, EYE_MOVE_X), rand_range(-EYE_MOVE_Y, EYE_MOVE_Y), rand_range_u32(500, 1200), now);
        m_next_look_ms = now + rand_range_u32(1400, 2800);
    }
    if (now >= m_next_blink_ms) enter_blink(now);
    else if (now >= m_sleep_at_ms) {
        m_state = FaceState::Sleeping;
        m_state_start_ms = now;
        m_wake_at_ms = now + rand_range_u32(6500, 11000);
        start_eye_move(0.0f, 0.0f, 900, now);
    }
}

void FaceScene::update_blinking(uint32_t now) {
    update_eye_move(now);
    const uint32_t dt = now - m_blink_phase_start_ms;
    if (m_blink_phase == BlinkPhase::Closing) {
        m_eye_open = lerpf(1.0f, 0.0f, smoothstep((float)dt / (float)m_blink_close_ms));
        if (dt >= m_blink_close_ms) {
            m_blink_phase = BlinkPhase::Hold;
            m_blink_phase_start_ms = now;
            m_eye_open = 0.0f;
        }
    } else if (m_blink_phase == BlinkPhase::Hold) {
        m_eye_open = 0.0f;
        if (dt >= m_blink_hold_ms) {
            m_blink_phase = BlinkPhase::Opening;
            m_blink_phase_start_ms = now;
        }
    } else {
        m_eye_open = lerpf(0.0f, 1.0f, smoothstep((float)dt / (float)m_blink_open_ms));
        if (dt >= m_blink_open_ms) {
            m_eye_open = 1.0f;
            m_state = FaceState::Idle;
            m_state_start_ms = now;
            m_next_blink_ms = now + rand_range_u32(3200, 6200);
        }
    }
}

void FaceScene::update_sleeping(uint32_t now) {
    update_eye_move(now);
    const uint32_t dt = now - m_state_start_ms;
    if (dt < 1000) m_eye_open = lerpf(1.0f, 0.0f, smoothstep((float)dt / 1000.0f));
    else {
        m_eye_open = 0.0f;
        if (now >= m_wake_at_ms) trigger_wake(now);
    }
}

void FaceScene::update_waking(uint32_t now) {
    update_eye_move(now);
    const uint32_t dt = now - m_wake_phase_start_ms;
    if (m_wake_phase == WakePhase::ToHalf) {
        m_eye_open = lerpf(0.0f, 0.45f, smoothstep((float)dt / 420.0f));
        if (dt >= 420) {
            m_eye_open = 0.45f;
            m_wake_phase = WakePhase::HoldHalf;
            m_wake_phase_start_ms = now;
        }
    } else if (m_wake_phase == WakePhase::HoldHalf) {
        m_eye_open = 0.45f;
        if (dt >= 170) {
            m_wake_phase = WakePhase::ToOpen;
            m_wake_phase_start_ms = now;
        }
    } else {
        m_eye_open = lerpf(0.45f, 1.0f, smoothstep((float)dt / 380.0f));
        if (dt >= 380) {
            m_eye_open = 1.0f;
            m_state = FaceState::Idle;
            m_state_start_ms = now;
            schedule_idle_events(now);
            m_next_blink_ms = now + 260;
            m_next_look_ms = now + 900;
        }
    }
}

void FaceScene::trigger_wake(uint32_t now) {
    if (m_state == FaceState::Sleeping) {
        m_state = FaceState::Waking;
        m_state_start_ms = now;
        m_wake_phase = WakePhase::ToHalf;
        m_wake_phase_start_ms = now;
        start_eye_move(0.0f, 0.0f, 700, now);
        Serial.println("B.M.O is waking up!");
    }
}

void FaceScene::toggle_love_pose() {
    if (m_pose == FacePose::Normal) {
        m_pose = FacePose::Love;
        Serial.println("B.M.O is feeling the LOVE! <3");
    } else {
        m_pose = FacePose::Normal;
        Serial.println("B.M.O is back to normal.");
    }
    // Force immediate invalidation to start transition
    if (m_mouth_obj) lv_obj_invalidate(m_mouth_obj);
    if (m_left_eye_obj) lv_obj_invalidate(m_left_eye_obj);
    if (m_right_eye_obj) lv_obj_invalidate(m_right_eye_obj);
}

void FaceScene::enter_blink(uint32_t now) {
    m_state = FaceState::Blinking;
    m_state_start_ms = now;
    m_blink_phase = BlinkPhase::Closing;
    m_blink_phase_start_ms = now;
    m_blink_close_ms = rand_range_u32(70, 110);
    m_blink_hold_ms = rand_range_u32(35, 65);
    m_blink_open_ms = rand_range_u32(90, 130);
}

void FaceScene::schedule_idle_events(uint32_t now) {
    m_next_blink_ms = now + rand_range_u32(3200, 6200);
    m_next_look_ms = now + rand_range_u32(500, 1600);
    m_sleep_at_ms = now + rand_range_u32(18000, 30000);
}

void FaceScene::start_eye_move(float target_x, float target_y, uint32_t duration_ms, uint32_t now) {
    m_eye_from_x = m_eye_dx;
    m_eye_from_y = m_eye_dy;
    m_eye_to_x = clampf(target_x, -EYE_MOVE_X, EYE_MOVE_X);
    m_eye_to_y = clampf(target_y, -EYE_MOVE_Y, EYE_MOVE_Y);
    m_eye_move_start_ms = now;
    m_eye_move_dur_ms = duration_ms;
    m_eye_move_active = true;
}

void FaceScene::update_eye_move(uint32_t now) {
    if (!m_eye_move_active) return;
    const float t = clampf((float)(now - m_eye_move_start_ms) / (float)m_eye_move_dur_ms, 0.0f, 1.0f);
    const float eased = smoothstep(t);
    m_eye_dx = lerpf(m_eye_from_x, m_eye_to_x, eased);
    m_eye_dy = lerpf(m_eye_from_y, m_eye_to_y, eased);
    if (t >= 1.0f) {
        m_eye_dx = m_eye_to_x;
        m_eye_dy = m_eye_to_y;
        m_eye_move_active = false;
    }
}

void FaceScene::redraw_eye_canvas(lv_obj_t *canvas) {
    if (!canvas) return;

    float breath_offset = (m_state == FaceState::Sleeping) ? sinf((float)millis() * 0.0019f) * 1.2f : 0.0f;
    const float draw_open = clampf(m_eye_open, 0.0f, 1.0f);
    const int eye_h = max(2, (int)lroundf((float)(EYE_R * 2) * (0.10f + (draw_open * 0.90f))));
    const int eye_w = max(10, (int)lroundf((float)(EYE_R * 1.6f) * (draw_open > 0.7f ? 1.0f : 0.92f)));
    const int draw_cx = (EYE_OBJ_W / 2) + (int)lroundf(m_eye_dx * draw_open);
    const int draw_cy = (EYE_OBJ_H / 2) + (int)lroundf(m_eye_dy * draw_open) + (int)lroundf(breath_offset);

    lv_canvas_fill_bg(canvas, lv_color_hex(COLOR_BG), LV_OPA_COVER);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;

    // Transition eye color to red during Love Pose
    uint8_t red_val = (uint8_t)lroundf(m_pose_progress * 255.0f);
    dsc.bg_color = lv_color_make(red_val, 0, 0);
    dsc.bg_opa = LV_OPA_COVER;

    if (m_pose_progress <= 0.01f) {
        // Normal oval eye
        lv_canvas_draw_rect(canvas, draw_cx - (eye_w / 2), draw_cy - (eye_h / 2), eye_w, eye_h, &dsc);
    } else {
        // Transition to heart eye
        // A simple heart can be made of two circles (lobes) and a triangle (bottom)
        const float heart_size = lerpf((float)eye_h, (float)EYE_R * 2.8f, m_pose_progress);
        const float s = heart_size * 0.5f;

        // Transition from centered oval to split lobes
        const float lobe_offset = lerpf(0.0f, s * 0.45f, m_pose_progress);
        const float lobe_y = draw_cy - (s * 0.2f);
        const float lobe_r = s * 0.55f;

        // Draw left lobe
        lv_canvas_draw_rect(canvas, (int)lroundf((float)draw_cx - lobe_offset - lobe_r), (int)lroundf(lobe_y - lobe_r), (int)lroundf(lobe_r * 2), (int)lroundf(lobe_r * 2), &dsc);
        // Draw right lobe
        lv_canvas_draw_rect(canvas, (int)lroundf((float)draw_cx + lobe_offset - lobe_r), (int)lroundf(lobe_y - lobe_r), (int)lroundf(lobe_r * 2), (int)lroundf(lobe_r * 2), &dsc);

        // Draw bottom triangle (using polygon)
        lv_draw_rect_dsc_t tri_dsc;
        lv_draw_rect_dsc_init(&tri_dsc);
        tri_dsc.bg_color = dsc.bg_color;
        tri_dsc.bg_opa = LV_OPA_COVER;

        lv_point_t pts[3];
        // Wide top
        pts[0] = {(lv_coord_t)lroundf((float)draw_cx - lobe_offset - lobe_r * 0.8f), (lv_coord_t)lroundf(lobe_y)};
        pts[1] = {(lv_coord_t)lroundf((float)draw_cx + lobe_offset + lobe_r * 0.8f), (lv_coord_t)lroundf(lobe_y)};
        // Pointy bottom
        pts[2] = {(lv_coord_t)draw_cx, (lv_coord_t)lroundf(draw_cy + s * 0.7f)};

        lv_canvas_draw_polygon(canvas, pts, 3, &tri_dsc);
    }
}
