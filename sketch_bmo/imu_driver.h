#pragma once
#include <Arduino.h>

struct ImuData {
    float ax, ay, az;
    float gx, gy, gz;
};

void init_imu();
bool update_imu(ImuData &data);
