// lvgl_glue.cpp — see lvgl_glue.h.

#include "lvgl_glue.h"

#include <Arduino.h>
#include <lvgl.h>

#include "board_pins.h"
#include "display/display.h"
#include "power/power.h"
#include "touch/touch.h"

// LVGL renders only what changed, in strips of up to DRAW_LINES rows, into
// s_draw_buf in internal RAM (much faster than PSRAM for drawing). The flush
// callback copies each strip, rotated, into s_panel_buf: the whole panel
// frame, 320 × 480 × 2 bytes = 300 KB in PSRAM, laid out the way the portrait
// panel expects. Unchanged parts of the frame keep their old pixels.
constexpr int32_t DRAW_LINES = 40;   // 480 × 40 × 2 = 38 KB of internal RAM

static uint16_t*     s_draw_buf  = nullptr;
static uint16_t*     s_panel_buf = nullptr;
static lv_display_t* s_display   = nullptr;

static uint32_t   s_last_tick_ms = 0;
static uint32_t   s_refresh_start_us = 0;
static uint32_t   s_transpose_sum_us = 0;
static FlushStats s_stats        = {};

// Copy one rendered strip (landscape `area`, pixels in `src`, row by row)
// into the portrait panel frame. The direction comes from SCREEN_ROTATE_CW
// in board_pins.h; touch uses the same setting.
static void transpose_area(const lv_area_t* area, const uint16_t* src) {
    const int32_t w = lv_area_get_width(area);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        const uint16_t* row = src + (y - area->y1) * w;
        for (int32_t x = area->x1; x <= area->x2; x++) {
            const uint16_t px = row[x - area->x1];
            if (SCREEN_ROTATE_CW) {
                s_panel_buf[(PANEL_H - 1 - x) * PANEL_W + y] = px;
            } else {
                s_panel_buf[x * PANEL_W + (PANEL_W - 1 - y)] = px;
            }
        }
    }
}

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t t0 = micros();
    transpose_area(area, (const uint16_t*)px_map);
    s_transpose_sum_us += micros() - t0;

    // LVGL may send several strips per refresh. The panel only takes whole
    // frames (§P9), so push once, after the last strip of this refresh.
    if (lv_display_flush_is_last(disp)) {
        uint32_t t1 = micros();
        display_push_frame(s_panel_buf);
        uint32_t t2 = micros();

        s_stats.transpose_us = s_transpose_sum_us;
        s_stats.push_us = t2 - t1;
        s_stats.refresh_us = t2 - s_refresh_start_us;
        s_stats.frames++;
        s_transpose_sum_us = 0;
    }
    lv_display_flush_ready(disp);
}

// Marks the start of a refresh, so the stats include LVGL's render time.
static void on_refresh_start(lv_event_t* e) {
    (void)e;
    s_refresh_start_us = micros();
    s_transpose_sum_us = 0;
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
    const size_t draw_bytes = (size_t)SCREEN_W * DRAW_LINES * sizeof(uint16_t);
    const size_t frame_bytes = (size_t)PANEL_W * PANEL_H * sizeof(uint16_t);
    s_draw_buf  = (uint16_t*)heap_caps_malloc(draw_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_panel_buf = (uint16_t*)ps_malloc(frame_bytes);
    if (!s_draw_buf || !s_panel_buf) {
        return false;
    }
    // ps_malloc does not zero memory; stale PSRAM shows up as noise (§P5).
    memset(s_draw_buf, 0, draw_bytes);
    memset(s_panel_buf, 0, frame_bytes);

    lv_init();

    // Landscape size at ROTATION_0. Never use lv_display_set_rotation() or
    // MADCTL rotation on this board (hardware doc §P2–§P4). PARTIAL mode
    // redraws only what changed; the flush still pushes whole frames.
    s_display = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(s_display, s_draw_buf, nullptr, draw_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_display, flush_cb);
    lv_display_add_event_cb(s_display, on_refresh_start, LV_EVENT_REFR_START, nullptr);

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
