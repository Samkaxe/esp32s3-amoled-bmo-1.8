# 🤖 BMO-ESP32-S3-AMOLED
### An Interactive BMO Companion for the Waveshare 1.8" AMOLED Board

Welcome to the **BMO-ESP32-S3-AMOLED** project! This repository contains a fully-featured, interactive, and highly optimized firmware for the **Waveshare ESP32-S3-Touch-AMOLED-1.8**. It transforms your development board into BMO from *Adventure Time*, complete with a live motion-sensing heart, a WiFi spectrum analyzer, and a system diagnostic dashboard.

---

## 🌟 Introduction
This project was born from the desire to create a "living" companion using modern hardware. Originally designed for the 2.41" board, this version has been completely refactored and optimized for the **1.8" portrait AMOLED** (368x448). It leverages the **LVGL 8.3** graphics library, the **ESP32-S3** dual-core processor, and a custom-built, stable I2C management layer to provide a smooth and reliable experience.

---

## 🛠️ Hardware Specifications
This firmware is specifically tailored for the **Waveshare ESP32-S3-Touch-AMOLED-1.8** development board:
*   **MCU**: ESP32-S3 Dual-Core LX7 (with PSRAM).
*   **Display**: 1.8" AMOLED, 368×448 pixels, SH8601 controller (QSPI).
*   **Touch**: FT3x68 Capacitive Touch.
*   **IMU**: QMI8658 6-axis (Accelerometer + Gyroscope).
*   **PMU**: AXP2101 Power Management Unit.
*   **RTC**: PCF85063 Real-Time Clock.
*   **IO Expander**: XCA9554 (used for hardware reset).

---

## 🚀 Key Features

### 1. **Interactive BMO Face**
*   **Live Animations**: Smoothly rendered eyes and mouth using LVGL primitives.
*   **Touch Interaction**: Touch the screen to trigger hidden expressions, such as the "Love Pose" (`(づ｡◕‿‿◕｡)づ`).
*   **Physical Feedback**: Physical interactions with the BOOT button trigger blinks and state changes.

### 2. **Sensor Mode (IMU Analyzer)**
*   **Full 6-Axis Visualization**: A high-frequency, full-screen chart showing real-time data from the QMI8658 sensor.
*   **7-Data Series**: AX, AY, AZ (Accelerometer), GX, GY, GZ (Gyroscope), and a "Motion Wave."
*   **Awesome Wave**: A unique visualization that modulates its frequency and amplitude based on the physical movement and rotation of the device.
*   **Terminal Aesthetic**: A high-contrast, black-and-green "hacker" look with a color-coded legend.

### 3. **WiFi Spectrum Analyzer**
*   **Real-time Scanning**: Visualizes 2.4GHz channel congestion (Channels 1-14).
*   **Memory-Efficient Drawing**: Uses a custom `EVENT_DRAW_MAIN` callback to render the spectrum directly to the screen, saving over 170KB of RAM and preventing allocation failures.
*   **SSID Mapping**: Automatically identifies and labels the strongest networks in each channel.

### 4. **System Info & Performance**
*   **Live Metrics**: Monitors FPS, CPU Load, and RAM usage (Internal vs. PSRAM).
*   **Diagnostic Tools**: Includes an I2C bus scanner at boot to ensure all hardware components are detected.

---

## 📂 Software Architecture

The project is structured into modular drivers and a scene-based UI manager:

### **Drivers**
*   `display_driver.cpp`: Handles the **SH8601** QSPI initialization, **AXP2101** voltage rail configuration (ALDOs/DCDCs), and **XCA9554** reset sequence.
*   `imu_driver.cpp`: High-speed data retrieval from the **QMI8658** sensor using the `SensorLib` library.
*   `rtc_driver.cpp`: Manages the **PCF85063** RTC and initializes the I2C bus with optimized stability settings.
*   `input_handler.cpp`: Processes the **BOOT button** events and coordinate mapping.

### **UI Managers**
*   `scene_manager.cpp`: A state machine that handles transitions between different modes (Face, Clock/Sensor, Info, WiFi).
*   `perf_monitor.cpp`: Background task to calculate system performance metrics.

### **Scenes**
*   `face_scene.cpp`: The interactive BMO interface.
*   `clock_scene.cpp`: The high-performance IMU data visualizer.
*   `info_scene.cpp`: Hardware telemetry and status.
*   `wifi_scene.cpp`: The memory-optimized WiFi spectrum analyzer.

---

## 🛠️ Technical Highlights

### **1. I2C Stability Layer**
To overcome the hardware-level interference common on the 1.8" board during WiFi bursts, we implemented:
*   A reduced I2C bus speed (50kHz) for maximum signal integrity.
*   Increased hardware timeouts to prevent "transaction failed" errors.
*   Explicit PMU voltage management to ensure stable 3.3V power to the Touch and IMU chips.

### **2. Direct Graphics Injection**
Instead of using heavy memory buffers (Canvas), the WiFi Analyzer uses LVGL's direct drawing API. This allows for complex spectrum rendering with virtually **zero extra RAM overhead**, ensuring stability even when system resources are low.

---

## ⚙️ Installation Guide

### **Environment Setup**
1.  Install **Arduino IDE** (v2.x recommended).
2.  Add ESP32 board support (v3.x is highly recommended for S3-AMOLED compatibility).
3.  Install Required Libraries:
    *   **LVGL** (v8.3.x)
    *   **SensorLib** (by Lewis He)
    *   **WiFi** (Built-in)
    *   **Wire** (Built-in)

### **Board Settings**
*   **Board**: `ESP32S3 Dev Module`
*   **USB CDC On Boot**: `Enabled`
*   **PSRAM**: `OPI PSRAM` (Crucial for LVGL performance)
*   **Flash Mode**: `QIO 80MHz`
*   **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)`

### **Upload**
1.  Connect your Waveshare 1.8" board via USB.
2.  Open `sketch_bmo.ino`.
3.  Hit **Upload** and wait for the "BMO is a real living boy!" message in the Serial Monitor.

---

## 🎮 Controls
*   **BOOT Button**: 
    *   **Short Press**: Cycle through scenes (Face -> Sensor -> Info -> WiFi).
    *   **Long Press (Face Scene)**: Toggle Sleep/Wake mode.
*   **Touch Screen**: 
    *   Tap BMO's eyes or mouth for reactions.
    *   Slide or hold to interact with the "Love Pose" system.
*   **Motion**: 
    *   Shake or tilt the device in **Sensor Mode** to see the Awesome Wave react.

---

*“BMO does weird junk when no one is around.”* 💚

**Developed with 💚 for the Waveshare Community.**
