// display.h — AXS15231B panel over QSPI, plus the backlight.
//
// The panel is portrait-native (320×480). This module only knows physical
// coordinates; the landscape transpose is added by lvgl_glue in M2.
//
// Rendering rule: everything reaches the panel as one full 320×480 frame
// written from (0,0). The AXS15231B ignores the row window of a partial
// write, so a fillRect() at y = 200 is actually drawn from row 0, on top of
// whatever is there (hardware doc §P9). Build the picture in a PSRAM frame,
// then call display_push_frame().
#pragma once

#include <stdint.h>

class Arduino_GFX;

// Bring up the QSPI bus and the panel, and clear it to black.
// The backlight is not touched. Returns false if the panel fails to start.
bool display_init();

// The underlying Arduino_GFX object. Don't draw with its primitives
// (fillRect, drawLine, print...): they write partial windows, which land at
// row 0. Only full-panel calls from (0,0) are safe.
Arduino_GFX* display_get_gfx();

// Push one full PANEL_W × PANEL_H RGB565 frame, starting at (0,0). This is
// the only safe way to draw on this panel (hardware doc §P9).
void display_push_frame(const uint16_t* frame);

// Backlight level, 0 = off, 255 = full.
void display_set_brightness(uint8_t level);
uint8_t display_get_brightness();

// Fade the backlight from off up to `level` over `duration_ms`.
// Blocking: call from setup() only.
void display_ramp_backlight(uint8_t level, uint32_t duration_ms);

// Colour bands plus a 1 px frame, to check init, colours and panel edges.
// Returns false if the temporary PSRAM frame can't be allocated.
bool display_draw_test_pattern();
