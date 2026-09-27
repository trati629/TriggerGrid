// theme.h — interface colours and layout numbers from docs/style-guide.md.
//
// These are the only hex colours in UI code. Tile colours are not here: they
// arrive already compiled in the Pad (docs/config-schema.md).
#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "fonts/fonts.h"

namespace theme {

// ---- colour tokens (style guide, "UI tokens") ----
inline lv_color_t bg()         { return lv_color_hex(0x101112); }
inline lv_color_t surface1()   { return lv_color_hex(0x1A1C1E); }
inline lv_color_t surface2()   { return lv_color_hex(0x24272A); }
inline lv_color_t surface3()   { return lv_color_hex(0x2E3236); }
inline lv_color_t line()       { return lv_color_hex(0x383C41); }
inline lv_color_t text()       { return lv_color_hex(0xF2F3F4); }
inline lv_color_t text_muted() { return lv_color_hex(0xA0A6AC); }
inline lv_color_t text_faint() { return lv_color_hex(0x62696F); }
inline lv_color_t on_bright()  { return lv_color_hex(0x0E0F10); }
inline lv_color_t focus()      { return lv_color_hex(0xF2F3F4); }

// Status colours: only ever used to show state.
inline lv_color_t ok()         { return lv_color_hex(0x3DDC84); }
inline lv_color_t warn()       { return lv_color_hex(0xFFB224); }
inline lv_color_t error()      { return lv_color_hex(0xFF4D4F); }

// ---- screen layout (style guide, "Layout (device)") ----
constexpr int32_t STATUS_BAR_H = 22;
constexpr int32_t GRID_H       = 288;   // one page of tiles
constexpr int32_t DOTS_H       = 10;    // page dots under the grid
constexpr int32_t DOT_SIZE     = 6;
constexpr int32_t DOT_GAP      = 8;

// Numbers for one grid density (style guide, "Grid densities").
struct Grid {
    uint8_t  cols, rows;
    int32_t  tile_w, tile_h;
    int32_t  gap;
    int32_t  pad_x, pad_y;       // space around the grid inside the page
    int32_t  radius;             // same on every span: that is the bento look
    int32_t  label_pad;          // label and icon inset from the tile edge
    const lv_font_t* label_font;
    int32_t  icon_size;          // px; the tile icons come in 20 and 18
};

// regular: 8 + 5·88 + 4·6 + 8 = 480 wide, 6 + 3·88 + 2·6 + 6 = 288 tall
// compact: 6 + 6·73 + 5·6 + 6 = 480 wide, 7 + 4·64 + 3·6 + 7 = 288 tall
// Labels are Space Grotesk; icons are the web editor's SVGs (ui/icons.h).
inline const Grid& grid(uint8_t density) {
    static const Grid regular = {5, 3, 88, 88, 6, 8, 6, 14, 10,
                                 &space_grotesk_medium_16, 20};
    static const Grid compact = {6, 4, 73, 64, 6, 6, 7, 12, 8,
                                 &space_grotesk_medium_14, 18};
    return density == 1 ? compact : regular;
}

// ---- motion (style guide, "Motion and feedback") ----
constexpr uint32_t PRESS_MS      = 80;
constexpr uint8_t  PRESS_DARKEN  = 51;    // 20% darker
constexpr int32_t  PRESS_SHRINK_PERCENT = 4;   // scale to 96%
constexpr uint32_t FLASH_MS      = 150;

}  // namespace theme
