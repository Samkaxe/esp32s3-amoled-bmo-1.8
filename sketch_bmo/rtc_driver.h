#pragma once
#include <Arduino.h>

struct BmoTime {
    int year;
    int month;
    int day;
    int week;
    int hour;
    int minute;
    int second;
};

void init_i2c();
bool get_rtc_time(BmoTime &tm);
