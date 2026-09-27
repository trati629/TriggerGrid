// pad.h — the layout as the firmware uses it.
//
// This mirrors the compiled `pad` section of config.json
// (docs/config-schema.md). Everything is already numbers: RGB colours, icon
// indexes and HID codes. The browser did the thinking; the firmware only
// draws tiles and sends reports.
#pragma once

#include <stddef.h>
#include <stdint.h>

// Limits shared with the browser compiler. The device checks every value
// against these before using it (architecture.md, "Device-side checks").
constexpr uint8_t  PAD_FORMAT_VERSION  = 1;
constexpr uint8_t  PAD_MAX_PAGES       = 12;
constexpr uint8_t  PAD_MAX_TILES       = 24;    // per page
constexpr size_t   PAD_NAME_BYTES      = 24;    // device name, UTF-8 bytes
constexpr size_t   PAD_PAGE_NAME_BYTES = 60;    // 20 characters of UTF-8
constexpr size_t   PAD_LABEL_BYTES     = 24;
constexpr uint8_t  PAD_MAX_CHORD_KEYS  = 6;     // HID boot keyboard limit
constexpr uint16_t PAD_MAX_STROKES     = 4096;  // per keystroke list
constexpr uint16_t PAD_MAX_CONSUMER    = 0x3FF;

constexpr uint8_t PAD_DENSITY_REGULAR = 0;   // 5 × 3 grid
constexpr uint8_t PAD_DENSITY_COMPACT = 1;   // 6 × 4 grid

enum class ActionType : uint8_t {
    None,         // nothing to send (the tile has no valid action)
    Chord,        // "k": press modifiers + keys together, release together
    Consumer,     // "c": one media key
    Keystrokes,   // "s": typed text as a list of keystrokes
};

// A keystroke is the modifier byte and the key's usage code packed as 0xMMKK,
// the same 4 hex digits the browser writes per keystroke.
struct PadAction {
    ActionType type = ActionType::None;

    // Chord
    uint8_t modifiers = 0;
    uint8_t keys[PAD_MAX_CHORD_KEYS] = {};
    uint8_t key_count = 0;

    // Consumer
    uint16_t usage = 0;

    // Keystrokes. The list itself belongs to the Pad (see Pad::stroke_pool).
    uint8_t         delay_ms = 10;
    bool            numpad = false;   // uses numpad keys, so NumLock must be on
    uint16_t        stroke_count = 0;
    const uint16_t* strokes = nullptr;
};

struct PadTile {
    uint8_t   x, y, w, h;                      // grid cell and span
    char      label[PAD_LABEL_BYTES + 1];
    uint8_t   icon;                            // 0 = none; see ui/icons.h
    uint32_t  bg, fg, accent;                  // 0xRRGGBB, final colours
    bool      bar;                             // 3 px accent bar (soft style)
    PadAction action;
};

struct PadPage {
    char    name[PAD_PAGE_NAME_BYTES + 1];
    uint8_t tile_count;
    PadTile tiles[PAD_MAX_TILES];
};

struct Pad {
    char     name[PAD_NAME_BYTES + 1];   // Bluetooth name and mDNS host
    uint8_t  density;                    // PAD_DENSITY_*
    uint8_t  brightness;                 // 10–255
    uint16_t dim_sec;                    // 0 = never dim
    uint8_t  page_count;
    PadPage  pages[PAD_MAX_PAGES];

    // Keystroke lists of all tiles, in one PSRAM block. Null for the built-in
    // layout, whose lists are constants in flash.
    uint16_t* stroke_pool;
};

// Grid size for a density.
inline uint8_t pad_grid_cols(uint8_t density) { return density == PAD_DENSITY_COMPACT ? 6 : 5; }
inline uint8_t pad_grid_rows(uint8_t density) { return density == PAD_DENSITY_COMPACT ? 4 : 3; }
