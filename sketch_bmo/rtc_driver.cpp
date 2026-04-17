#include "rtc_driver.h"
#include "config.h"
#include <Wire.h>

#define I2C_PORT I2C_NUM_0
#define PCF85063_ADDR 0x51

static uint8_t bcd_to_dec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

void init_i2c() {
    Serial.println("Initializing I2C Bus via Wire...");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 50000);
    Wire.setTimeOut(100); // 100ms timeout

    Serial.println("I2C Bus Scan:");
    for (uint8_t i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            Serial.printf(" - 0x%02X found\n", i);
        }
    }
}

bool get_rtc_time(BmoTime &tm) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(0x04); // Seconds register
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom(PCF85063_ADDR, (uint8_t)7) != 7) return false;

    uint8_t data[7];
    for (int i = 0; i < 7; i++) {
        data[i] = Wire.read();
    }

    tm.second = bcd_to_dec(data[0] & 0x7F);
    tm.minute = bcd_to_dec(data[1] & 0x7F);
    tm.hour   = bcd_to_dec(data[2] & 0x3F);
    tm.day    = bcd_to_dec(data[3] & 0x3F);
    tm.week   = bcd_to_dec(data[4] & 0x07);
    tm.month  = bcd_to_dec(data[5] & 0x1F);
    tm.year   = bcd_to_dec(data[6]) + 2000;

    return true;
}
