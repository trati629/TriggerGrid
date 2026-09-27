// board_pins.h — Guition JC3248W535 pin map.
// Source of truth: docs/hardware/jc3248w535.md. Change pins here only.
#pragma once

#include <stdint.h>

// Display — QSPI, AXS15231B
constexpr int8_t PIN_LCD_CS   = 45;
constexpr int8_t PIN_LCD_SCK  = 47;
constexpr int8_t PIN_LCD_D0   = 21;
constexpr int8_t PIN_LCD_D1   = 48;
constexpr int8_t PIN_LCD_D2   = 40;
constexpr int8_t PIN_LCD_D3   = 39;
constexpr int8_t PIN_LCD_RST  = 38;   // wired, but do NOT pass to the GFX constructor
constexpr int8_t PIN_LCD_BL   = 1;    // backlight, analogWrite() 0–255

// Touch — I2C, AXS15231B (same chip as the display)
constexpr int8_t  PIN_TOUCH_SDA  = 4;
constexpr int8_t  PIN_TOUCH_SCL  = 8;
constexpr int8_t  PIN_TOUCH_INT  = 3;   // active-low, 3–6 pulses per touch
constexpr int8_t  PIN_TOUCH_RST  = 12;
constexpr uint8_t TOUCH_I2C_ADDR = 0x3B;

// Panel geometry. The panel is portrait-native; the enclosure is landscape.
// LVGL renders landscape and display code transposes (hardware.md, Solution 4).
constexpr int16_t PANEL_W = 320;   // physical
constexpr int16_t PANEL_H = 480;   // physical
constexpr int16_t SCREEN_W = 480;  // logical (LVGL, touch, UI layout)
constexpr int16_t SCREEN_H = 320;  // logical
