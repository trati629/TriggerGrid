// lvgl_glue.h — connects LVGL to the display (and, from M3, the touch panel).
//
// LVGL draws a 480×320 landscape screen. The flush callback turns each frame
// onto the 320×480 portrait panel and pushes it whole (hardware doc,
// Solution 4 and §P9). Only loop() may call anything here or in LVGL.
#pragma once

#include <stdint.h>

// Time spent on the most recent frame, for the serial log.
struct FlushStats {
    uint32_t transpose_us;   // rotating the frame into panel order
    uint32_t push_us;        // sending it over QSPI
    uint32_t frames;         // frames flushed since boot
};

// lv_init(), the two PSRAM frame buffers and the LVGL display.
// Returns false if PSRAM is missing.
bool lvgl_glue_init();

// Advance LVGL's clock and run its timers. Call on every loop() pass.
void lvgl_glue_update();

FlushStats lvgl_glue_flush_stats();
