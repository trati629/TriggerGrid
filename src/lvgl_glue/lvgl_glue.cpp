// lvgl_glue.cpp — see lvgl_glue.h.

#include "lvgl_glue.h"

#include <Arduino.h>
#include <lvgl.h>

#include "board_pins.h"
#include "display/display.h"
#include "power/power.h"
#include "touch/touch.h"

// LVGL renders the landscape screen into s_draw_buf. The flush callback
// rotates it into s_panel_buf, laid out the way the portrait panel expects.
// Both are full frames (480 × 320 × 2 bytes = 300 KB each) in PSRAM.
static uint16_t*     s_draw_buf  = nullptr;
static uint16_t*     s_panel_buf = nullptr;
static lv_display_t* s_display   = nullptr;

static uint32_t   s_last_tick_ms = 0;
static FlushStats s_stats        = {};

// Turn one landscape frame onto the portrait panel. The direction comes from
// SCREEN_ROTATE_CW in board_pins.h; touch uses the same setting.
static void transpose(const uint16_t* src, uint16_t* dst) {
    if (SCREEN_ROTATE_CW) {
        for (int y = 0; y < SCREEN_H; y++) {
            for (int x = 0; x < SCREEN_W; x++) {
                dst[(PANEL_H - 1 - x) * PANEL_W + y] = src[y * SCREEN_W + x];
            }
        }
    } else {
        for (int y = 0; y < SCREEN_H; y++) {
            for (int x = 0; x < SCREEN_W; x++) {
                dst[x * PANEL_W + (PANEL_W - 1 - y)] = src[y * SCREEN_W + x];
            }
        }
    }
}

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // RENDER_MODE_FULL: px_map is always the whole screen, so `area` is not
    // needed. That suits this panel, which only takes whole frames (§P9).
    (void)area;

    uint32_t t0 = micros();
    transpose((const uint16_t*)px_map, s_panel_buf);
    uint32_t t1 = micros();
    display_push_frame(s_panel_buf);
    uint32_t t2 = micros();

    s_stats.transpose_us = t1 - t0;
    s_stats.push_us      = t2 - t1;
    s_stats.frames++;

    lv_display_flush_ready(disp);
}

// LVGL polls this during lv_timer_handler(). The touch module has already
// turned the point into landscape coordinates.
static void touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    // A touch on a dark screen only wakes it: LVGL sees nothing until that
    // finger lifts, so it can't press a tile the user couldn't see.
    static bool swallowing = false;

    int16_t x, y;
    bool down = touch_get(&x, &y);
    if (down && power_screen_off()) {
        power_wake();
        swallowing = true;
    }
    if (!down) {
        swallowing = false;
    }
    data->point.x = x;
    data->point.y = y;
    data->state = (down && !swallowing) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

bool lvgl_glue_init() {
    const size_t frame_bytes = (size_t)SCREEN_W * SCREEN_H * sizeof(uint16_t);
    s_draw_buf  = (uint16_t*)ps_malloc(frame_bytes);
    s_panel_buf = (uint16_t*)ps_malloc(frame_bytes);
    if (!s_draw_buf || !s_panel_buf) {
        return false;
    }
    // ps_malloc does not zero memory; stale PSRAM shows up as noise (§P5).
    memset(s_draw_buf, 0, frame_bytes);
    memset(s_panel_buf, 0, frame_bytes);

    lv_init();

    // Landscape size at ROTATION_0. Never use lv_display_set_rotation() or
    // MADCTL rotation on this board (hardware doc §P2–§P4).
    s_display = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(s_display, s_draw_buf, nullptr, frame_bytes,
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(s_display, flush_cb);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    s_last_tick_ms = millis();
    return true;
}

void lvgl_glue_update() {
    // LVGL's own tick source is unreliable when it is built as a PlatformIO
    // library, so feed it elapsed milliseconds by hand (hardware doc §P8).
    uint32_t now = millis();
    lv_tick_inc(now - s_last_tick_ms);
    s_last_tick_ms = now;

    lv_timer_handler();
}

FlushStats lvgl_glue_flush_stats() {
    return s_stats;
}
