#pragma once
#include <Arduino.h>

static inline float clampf(float value, float lo, float hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static inline float lerpf(float a, float b, float t) {
    return a + ((b - a) * t);
}

static inline float smoothstep(float t) {
    t = clampf(t, 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
}

static inline float rand_range(float lo, float hi) {
    return lo + ((float)random(10000) * 0.0001f * (hi - lo));
}

static inline uint32_t rand_range_u32(uint32_t lo, uint32_t hi) {
    return lo + (uint32_t)random((long)(hi - lo + 1));
}
