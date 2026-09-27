// touch.h — AXS15231B capacitive touch over I2C.
//
// A small task reads the chip as soon as its INT pin pulses and keeps the
// latest point, already turned into landscape screen coordinates. LVGL asks
// for it from loop() through touch_get().
#pragma once

#include <stdint.h>

// Start I2C, the INT interrupt and the reader task.
// Returns false if the chip doesn't answer on I2C.
bool touch_init();

// The latest point in landscape coordinates (0–479, 0–319).
// Returns true while a finger is on the screen. When it returns false,
// x and y still hold the last point, which is what LVGL wants on release.
bool touch_get(int16_t* x, int16_t* y);
