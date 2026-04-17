#pragma once
#include "scene_manager.h"
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>

enum class FaceState : uint8_t { Idle, Blinking, Sleeping, Waking };
enum class FacePose : uint8_t { Normal, Love };
enum class BlinkPhase : uint8_t { Closing, Hold, Opening };
enum class WakePhase : uint8_t { ToHalf, HoldHalf, ToOpen };

class FaceScene : public Scene {
public:
    void init() override;
    void update(uint32_t now) override;
    void deinit() override;

    float get_pose_progress() const { return m_pose_progress; }
    void toggle_love_pose();

private:
    void create_ui();
    void animate_face(uint32_t now);
    void update_idle(uint32_t now);
    void update_blinking(uint32_t now);
    void update_sleeping(uint32_t now);
    void update_waking(uint32_t now);
    
    void trigger_wake(uint32_t now);
    void enter_blink(uint32_t now);
    void schedule_idle_events(uint32_t now);
    void start_eye_move(float target_x, float target_y, uint32_t duration_ms, uint32_t now);
    void update_eye_move(uint32_t now);
    
    void redraw_eye_canvas(lv_obj_t *canvas);

    FaceState m_state = FaceState::Idle;
    FacePose m_pose = FacePose::Normal;
    BlinkPhase m_blink_phase = BlinkPhase::Closing;
    WakePhase m_wake_phase = WakePhase::ToHalf;

    uint32_t m_state_start_ms = 0;
    uint32_t m_blink_phase_start_ms = 0;
    uint32_t m_blink_close_ms = 90;
    uint32_t m_blink_hold_ms = 55;
    uint32_t m_blink_open_ms = 120;
    uint32_t m_wake_phase_start_ms = 0;

    uint32_t m_next_blink_ms = 0;
    uint32_t m_next_look_ms = 0;
    uint32_t m_sleep_at_ms = 0;
    uint32_t m_wake_at_ms = 0;

    float m_eye_open = 1.0f;
    float m_pose_progress = 0.0f; // 0.0 = Normal, 1.0 = Love
    float m_eye_dx = 0.0f;
    float m_eye_dy = 0.0f;
    float m_eye_from_x = 0.0f;
    float m_eye_from_y = 0.0f;
    float m_eye_to_x = 0.0f;
    float m_eye_to_y = 0.0f;
    uint32_t m_eye_move_start_ms = 0;
    uint32_t m_eye_move_dur_ms = 0;
    bool m_eye_move_active = false;

    lv_obj_t *m_face = nullptr;
    lv_obj_t *m_left_eye_obj = nullptr;
    lv_obj_t *m_right_eye_obj = nullptr;
    lv_obj_t *m_mouth_obj = nullptr;
    lv_color_t *m_left_eye_canvas_buf = nullptr;
    lv_color_t *m_right_eye_canvas_buf = nullptr;
};
