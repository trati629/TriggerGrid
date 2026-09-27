// lvgl_glue.h — connects LVGL to the display and the touch panel.
//
// LVGL draws a 480×320 landscape screen, but only the parts that changed,
// in strips, into a small buffer in fast internal RAM. The flush callback
// turns each strip onto a full portrait frame in PSRAM, and when LVGL has
// finished the refresh, that whole frame goes to the panel (hardware doc,
// Solution 4 and §P9). Only loop() may call anything here or in LVGL.
#pragma once

#include <stdint.h>

// Timings of the most recent refresh, for the serial log.
struct FlushStats {
    uint32_t refresh_us;     // whole refresh: render + transpose + push
    uint32_t transpose_us;   // rotating the changed areas into panel order
    uint32_t push_us;        // sending the frame over QSPI
    uint32_t frames;         // frames pushed since boot
};

// lv_init(), the frame buffers, the LVGL display and the touch input
// device. Call touch_init() first. Returns false if memory is missing.
bool lvgl_glue_init();

// Advance LVGL's clock and run its timers. Call on every loop() pass.
void lvgl_glue_update();

FlushStats lvgl_glue_flush_stats();
