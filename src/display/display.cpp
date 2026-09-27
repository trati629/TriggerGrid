// display.cpp — see display.h.

#include "display.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "board_pins.h"

static Arduino_DataBus* s_bus = nullptr;
static Arduino_GFX*     s_gfx = nullptr;

bool display_init() {
    s_bus = new Arduino_ESP32QSPI(PIN_LCD_CS, PIN_LCD_SCK,
                                  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);

    // Three settings here are required on this board (see hardware doc §P1, §P2):
    // - RST is not passed: giving the driver GPIO 38 breaks init on some units.
    // - rotation stays 0: MADCTL rotation leaves the panel blank or corrupted.
    // - the 320×480 "type1" init sequence: the library default is for a
    //   180×640 panel and gives a blank screen or a single dashed line.
    s_gfx = new Arduino_AXS15231B(s_bus, GFX_NOT_DEFINED, 0 /* rotation */, false /* ips */,
                                  PANEL_W, PANEL_H, 0, 0, 0, 0,
                                  axs15231b_320480_type1_init_operations,
                                  sizeof(axs15231b_320480_type1_init_operations));

    if (!s_gfx->begin()) {
        return false;
    }
    // Safe despite §P9: fillScreen() covers the whole panel from (0,0).
    s_gfx->fillScreen(RGB565_BLACK);
    return true;
}

Arduino_GFX* display_get_gfx() {
    return s_gfx;
}

void display_push_frame(const uint16_t* frame) {
    // Always the whole panel from (0,0). The AXS15231B ignores the row window
    // for partial writes, so fillRect() and friends all draw at the top (§P9).
    // Keep x = 0, y = 0 and the full PANEL_W × PANEL_H size here, even when
    // only part of the picture changed.
    s_gfx->draw16bitRGBBitmap(0, 0, const_cast<uint16_t*>(frame), PANEL_W, PANEL_H);
}

void display_set_brightness(uint8_t level) {
    // Core 3.x: analogWrite() sets up LEDC itself; ledcSetup() no longer exists.
    analogWrite(PIN_LCD_BL, level);
}

void display_ramp_backlight(uint8_t level, uint32_t duration_ms) {
    if (level == 0) {
        display_set_brightness(0);
        return;
    }
    const uint32_t step_ms = duration_ms / level;
    for (uint16_t v = 0; v <= level; v++) {
        display_set_brightness(v);
        delay(step_ms);
    }
}

// Fill a rectangle in a PANEL_W-wide frame buffer.
static void fill_rect(uint16_t* frame, int x, int y, int w, int h, uint16_t color) {
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            frame[row * PANEL_W + col] = color;
        }
    }
}

bool display_draw_test_pattern() {
    // Draw into a full frame in PSRAM, then push it in one go (§P9).
    const size_t frame_bytes = (size_t)PANEL_W * PANEL_H * sizeof(uint16_t);
    uint16_t* frame = (uint16_t*)ps_malloc(frame_bytes);
    if (!frame) {
        return false;
    }
    memset(frame, 0, frame_bytes);   // ps_malloc does not zero memory (§P5); 0 = black

    // Diagnostic colours straight from the GFX library, not UI colours
    // (those live in src/ui/theme.h). Drawn in physical portrait coordinates.
    static const uint16_t kBands[] = {
        RGB565_WHITE, RGB565_YELLOW, RGB565_CYAN, RGB565_LIME,
        RGB565_MAGENTA, RGB565_RED, RGB565_BLUE,
    };
    constexpr int kBandCount = sizeof(kBands) / sizeof(kBands[0]);

    // Bands fill the panel inset by 4 px, so the black gap sets off the frame.
    constexpr int kInset = 4;
    constexpr int kAreaW = PANEL_W - 2 * kInset;
    constexpr int kAreaH = PANEL_H - 2 * kInset;

    // Bands run along the long (480 px) side. White is at panel row 0, so the
    // white band shows which way the panel is mounted in the case.
    for (int i = 0; i < kBandCount; i++) {
        int y0 = kInset + (kAreaH * i) / kBandCount;
        int y1 = kInset + (kAreaH * (i + 1)) / kBandCount;
        fill_rect(frame, kInset, y0, kAreaW, y1 - y0, kBands[i]);
    }

    // Frame 1 px in from every edge. If any side is missing, the panel
    // geometry or offsets are wrong.
    fill_rect(frame, 1, 1, PANEL_W - 2, 1, RGB565_WHITE);             // top
    fill_rect(frame, 1, PANEL_H - 2, PANEL_W - 2, 1, RGB565_WHITE);   // bottom
    fill_rect(frame, 1, 1, 1, PANEL_H - 2, RGB565_WHITE);             // left
    fill_rect(frame, PANEL_W - 2, 1, 1, PANEL_H - 2, RGB565_WHITE);   // right

    display_push_frame(frame);
    free(frame);
    return true;
}
