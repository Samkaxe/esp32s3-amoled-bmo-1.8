#include "display_driver.h"
#include "config.h"
#include "globals.h"
#include <Wire.h>
#include "esp_lcd_panel_commands.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"

esp_lcd_panel_io_handle_t g_panel_io = nullptr;
lv_disp_t *g_display = nullptr;
static lv_disp_draw_buf_t g_draw_buf;
static lv_disp_drv_t g_disp_drv;
static lv_indev_drv_t g_indev_drv;
static lv_color_t *g_buf1 = nullptr;
static lv_color_t *g_buf2 = nullptr;

static constexpr uint8_t QSPI_WRITE_CMD   = 0x02;
static constexpr uint8_t QSPI_WRITE_COLOR = 0x32;

// I2C Addresses
static constexpr uint8_t ADDR_PMU      = 0x34; // AXP2101
static constexpr uint8_t ADDR_EXPANDER = 0x20; // XCA9554

static uint32_t encode_qspi_cmd(uint8_t opcode, uint8_t command) {
    return ((uint32_t)opcode << 24) | ((uint32_t)command << 8);
}

esp_err_t write_cmd(uint8_t command, const void *data, size_t len) {
    if (!g_panel_io) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_io_tx_param(g_panel_io, encode_qspi_cmd(QSPI_WRITE_CMD, command), data, len);
}

static esp_err_t write_color(const void *data, size_t len) {
    if (!g_panel_io) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_io_tx_color(g_panel_io, encode_qspi_cmd(QSPI_WRITE_COLOR, LCD_CMD_RAMWR), data, len);
}

static esp_err_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    if (Wire.endTransmission() == 0) {
        return ESP_OK;
    } else {
        Serial.printf("I2C WRITE ERROR: addr=0x%02X, reg=0x%02X\n", addr, reg);
        return ESP_FAIL;
    }
}

void init_pmu() {
    // Set ALDO Voltages (500mV + reg*100mV)
    i2c_write_reg(ADDR_PMU, 0x92, 0x0D); // ALDO1: 1.8V (CAM DVDD)
    i2c_write_reg(ADDR_PMU, 0x93, 0x17); // ALDO2: 2.8V (CAM DVDD)
    i2c_write_reg(ADDR_PMU, 0x94, 0x1C); // ALDO3: 3.3V (PIR VDD / VCC)
    i2c_write_reg(ADDR_PMU, 0x95, 0x19); // ALDO4: 3.0V (CAM AVDD)
    i2c_write_reg(ADDR_PMU, 0x96, 0x1C); // BLDO1: 3.3V (OLED VDD)
    i2c_write_reg(ADDR_PMU, 0x97, 0x1C); // BLDO2: 3.3V (MIC VDD)
    
    // Set DCDC Voltages (3.3V = 0xAF approx)
    i2c_write_reg(ADDR_PMU, 0x82, 0xAF); // DC1: 3.3V (Extern 3.3V)
    i2c_write_reg(ADDR_PMU, 0x84, 0xAF); // DC3: 3.3V (ESP32-S3 Core)
    
    // Enable ALDO1-4, BLDO1-2, CPUSLDO, DLDO1
    i2c_write_reg(ADDR_PMU, 0x90, 0xFF); 
    // Enable DCDC1-5
    i2c_write_reg(ADDR_PMU, 0x80, 0x1F); 
    
    Serial.println("PMU Voltages initialized (ALDOs + DCDCs).");
}

void reset_panel() {
    // Use XCA9554 to reset display and touch
    i2c_write_reg(ADDR_EXPANDER, 0x03, 0x00); // Config: All output
    i2c_write_reg(ADDR_EXPANDER, 0x01, 0x00); // Output: All LOW
    delay(50);
    i2c_write_reg(ADDR_EXPANDER, 0x01, 0x07); // Output: P0, P1, P2 HIGH
    delay(150);
}

void init_panel() {
    // SH8601 Initialization Sequence
    write_cmd(0x11); // Sleep Out
    delay(120);
    
    const uint8_t pixfmt = 0x05; // 16-bit
    write_cmd(0x3A, &pixfmt, 1);
    
    const uint8_t madctl = 0x00;
    write_cmd(0x36, &madctl, 1);
    
    const uint8_t ctrld1 = 0x28;
    write_cmd(0x53, &ctrld1, 1);
    
    const uint8_t brightness = 0xFF;
    write_cmd(0x51, &brightness, 1);
    
    write_cmd(0x29); // Display On
    delay(10);
}

static void set_address_window(int x1, int y1, int x2, int y2) {
    const int ys = y1 + DISPLAY_YOFFSET;
    const int ye = y2 + DISPLAY_YOFFSET;
    const uint8_t col[] = {(uint8_t)(x1 >> 8), (uint8_t)x1, (uint8_t)(x2 >> 8), (uint8_t)x2};
    const uint8_t row[] = {(uint8_t)(ys >> 8), (uint8_t)ys, (uint8_t)(ye >> 8), (uint8_t)ye};
    write_cmd(LCD_CMD_CASET, col, sizeof(col));
    write_cmd(LCD_CMD_RASET, row, sizeof(row));
}

static void rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area) {
    // Round start coordinates down to nearest even number
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    // Round end coordinates up to nearest odd number (so width/height is even)
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

static bool on_flush_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *ctx) {
    lv_disp_flush_ready(static_cast<lv_disp_drv_t *>(ctx));
    return false;
}

static void flush_display(lv_disp_drv_t *, const lv_area_t *area, lv_color_t *map) {
    set_address_window(area->x1, area->y1, area->x2, area->y2);
    const size_t pixel_count = (size_t)(area->x2 - area->x1 + 1) * (size_t)(area->y2 - area->y1 + 1);
    write_color(map, pixel_count * sizeof(lv_color_t));
}

void init_spi_bus() {
    spi_bus_config_t bus = {};
    bus.data0_io_num = PIN_LCD_D0;
    bus.data1_io_num = PIN_LCD_D1;
    bus.sclk_io_num = PIN_LCD_PCLK;
    bus.data2_io_num = PIN_LCD_D2;
    bus.data3_io_num = PIN_LCD_D3;
    bus.max_transfer_sz = DISPLAY_WIDTH * LVGL_BUF_LINES * (int)sizeof(lv_color_t) + 8;
    
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: SPI bus initialize failed (0x%x)\n", ret);
    }

    esp_lcd_panel_io_spi_config_t io = {};
    io.cs_gpio_num = PIN_LCD_CS;
    io.dc_gpio_num = -1;
    io.spi_mode = 0;
    io.pclk_hz = 60 * 1000 * 1000;
    io.trans_queue_depth = 10;
    io.on_color_trans_done = on_flush_done;
    io.user_ctx = &g_disp_drv;
    io.lcd_cmd_bits = 32;
    io.lcd_param_bits = 8;
    io.flags.quad_mode = 1;

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io, &g_panel_io);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Panel IO create failed (0x%x)\n", ret);
    }
}

void init_lvgl_display() {
    lv_init();
    size_t buf_sz = DISPLAY_WIDTH * LVGL_BUF_LINES * sizeof(lv_color_t);
    
    g_buf1 = (lv_color_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!g_buf1) {
        g_buf1 = (lv_color_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    
    g_buf2 = (lv_color_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!g_buf2) {
        g_buf2 = (lv_color_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    
    if (!g_buf1 || !g_buf2) {
        Serial.println("CRITICAL: Failed to allocate LVGL display buffers!");
        return;
    }
    
    lv_disp_draw_buf_init(&g_draw_buf, g_buf1, g_buf2, DISPLAY_WIDTH * LVGL_BUF_LINES);
    lv_disp_drv_init(&g_disp_drv);
    g_disp_drv.hor_res = DISPLAY_WIDTH;
    g_disp_drv.ver_res = DISPLAY_HEIGHT;
    g_disp_drv.flush_cb = flush_display;
    g_disp_drv.rounder_cb = rounder_cb;
    g_disp_drv.draw_buf = &g_draw_buf;
    g_display = lv_disp_drv_register(&g_disp_drv);
}

void set_brightness(uint8_t level) {
    const uint8_t data[] = {level};
    write_cmd(0x51, data, sizeof(data));
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    // Manual I2C read for FT3x68/FT5x06
    const uint8_t touch_addr = 0x38;
    
    Wire.beginTransmission(touch_addr);
    Wire.write(0x02); // TD_STATUS (Number of touch points)
    if (Wire.endTransmission(false) != 0) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (Wire.requestFrom(touch_addr, (uint8_t)5) != 5) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint8_t points = Wire.read();
    uint8_t x_high = Wire.read();
    uint8_t x_low  = Wire.read();
    uint8_t y_high = Wire.read();
    uint8_t y_low  = Wire.read();

    if (points > 0 && points <= 5) {
        uint16_t x = ((x_high & 0x0F) << 8) | x_low;
        uint16_t y = ((y_high & 0x0F) << 8) | y_low;
        
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void init_touch() {
    // Set FT3x68 Power Mode to Active (0x00) via manual I2C write
    i2c_write_reg(0x38, 0xA5, 0x00);

    lv_indev_drv_init(&g_indev_drv);
    g_indev_drv.type = LV_INDEV_TYPE_POINTER;
    g_indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&g_indev_drv);
    
    Serial.println("Touch system initialized (Manual Wire).");
}
