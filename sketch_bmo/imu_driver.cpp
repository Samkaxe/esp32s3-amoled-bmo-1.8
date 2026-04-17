#include "imu_driver.h"
#include "config.h"
#include <Wire.h>
#include "SensorQMI8658.hpp"

static SensorQMI8658 qmi;
static bool imu_initialized = false;

void init_imu() {
    Serial.println("Initializing QMI8658 IMU (via Wire)...");
    
    // Wire is initialized in rtc_driver.cpp via init_i2c() using legacy IDF driver.
    // We use the TwoWire overload to stay in Arduino land and hopefully avoid driver conflicts.
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
        Serial.println("Failed to find QMI8658!");
        return;
    }

    Serial.printf("QMI8658 ID: 0x%02X\n", qmi.getChipID());

    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0);
    qmi.configGyroscope(SensorQMI8658::GYR_RANGE_64DPS, SensorQMI8658::GYR_ODR_896_8Hz, SensorQMI8658::LPF_MODE_3);
    
    qmi.enableGyroscope();
    qmi.enableAccelerometer();
    
    imu_initialized = true;
    Serial.println("QMI8658 IMU Initialized.");
}

bool update_imu(ImuData &data) {
    if (!imu_initialized) return false;
    
    if (qmi.getDataReady()) {
        qmi.getAccelerometer(data.ax, data.ay, data.az);
        qmi.getGyroscope(data.gx, data.gy, data.gz);
        return true;
    }
    return false;
}
