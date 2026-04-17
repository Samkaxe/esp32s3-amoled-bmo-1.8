#include "perf_monitor.h"

uint32_t g_perf_last_ms = 0;
uint32_t g_perf_frames = 0;
uint32_t g_perf_busy_us = 0;

float g_last_fps = 0;
float g_last_cpu_load = 0;

void update_perf_monitor(uint32_t now) {
    if (now - g_perf_last_ms >= 1000) { // Update every 1s for better responsiveness in charts
        const float elapsed_s = (float)(now - g_perf_last_ms) / 1000.0f;
        g_last_fps = (float)g_perf_frames / elapsed_s;
        g_last_cpu_load = (float)g_perf_busy_us / (elapsed_s * 1000000.0f) * 100.0f;

        const size_t free_heap = esp_get_free_heap_size();
        const size_t min_free = esp_get_minimum_free_heap_size();

        // Serial.printf("B.M.O Stats - FPS: %.1f | CPU Load: %.1f%% | RAM Free: %u bytes (Min: %u)\n",
        //               g_last_fps, g_last_cpu_load, (uint32_t)free_heap, (uint32_t)min_free);

        g_perf_frames = 0;
        g_perf_busy_us = 0;
        g_perf_last_ms = now;
    }
}
