#pragma once
#include <Arduino.h>

extern uint32_t g_perf_last_ms;
extern uint32_t g_perf_frames;
extern uint32_t g_perf_busy_us;

// Latest calculated metrics
extern float g_last_fps;
extern float g_last_cpu_load;

void update_perf_monitor(uint32_t now);
