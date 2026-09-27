// defaults.cpp — the built-in layout, used until a valid config.json exists.
//
// These are the compiled values the browser produces for the default layout
// in the web editor (colours already mixed, keys already HID codes). Don't
// invent new colours here: take them from the compiler's output.

#include <string.h>

#include "config.h"

// HID codes used below, named for readability (USB HID Usage Tables).
namespace {
constexpr uint8_t MOD_CTRL  = 0x01;
constexpr uint8_t MOD_SHIFT = 0x02;
constexpr uint8_t MOD_ALT   = 0x04;
constexpr uint8_t MOD_GUI   = 0x08;

constexpr uint16_t MEDIA_PLAY_PAUSE  = 0xCD;
constexpr uint16_t MEDIA_NEXT_TRACK  = 0xB5;
constexpr uint16_t MEDIA_PREV_TRACK  = 0xB6;
constexpr uint16_t MEDIA_MUTE        = 0xE2;
constexpr uint16_t MEDIA_VOLUME_UP   = 0xE9;
constexpr uint16_t MEDIA_VOLUME_DOWN = 0xEA;

// "Cheers,\nAlex" typed on a US layout (docs/config-schema.md, worked example).
const uint16_t kEmailSig[] = {
    0x0206, 0x000B, 0x0008, 0x0008, 0x0015, 0x0016,   // C h e e r s
    0x0036, 0x0028,                                   // , Enter
    0x0204, 0x000F, 0x0008, 0x001B,                   // A l e x
};

PadAction chord(uint8_t modifiers, uint8_t key) {
    PadAction a;
    a.type = ActionType::Chord;
    a.modifiers = modifiers;
    a.keys[0] = key;
    a.key_count = 1;
    return a;
}

PadAction media(uint16_t usage) {
    PadAction a;
    a.type = ActionType::Consumer;
    a.usage = usage;
    return a;
}

PadAction keystrokes(const uint16_t* strokes, uint16_t count, uint8_t delay_ms) {
    PadAction a;
    a.type = ActionType::Keystrokes;
    a.strokes = strokes;
    a.stroke_count = count;
    a.delay_ms = delay_ms;
    return a;
}

PadPage& add_page(Pad& pad, const char* name) {
    PadPage& page = pad.pages[pad.page_count++];
    strncpy(page.name, name, PAD_PAGE_NAME_BYTES);
    page.tile_count = 0;
    return page;
}

// Colours are 0xRRGGBB: fill, label, accent (icon and soft-style bar).
void add_tile(PadPage& page, uint8_t x, uint8_t y, uint8_t w, uint8_t h,
              const char* label, uint8_t icon,
              uint32_t bg, uint32_t fg, uint32_t accent, bool bar,
              const PadAction& action) {
    PadTile& t = page.tiles[page.tile_count++];
    t.x = x; t.y = y; t.w = w; t.h = h;
    strncpy(t.label, label, PAD_LABEL_BYTES);
    t.icon = icon;
    t.bg = bg; t.fg = fg; t.accent = accent;
    t.bar = bar;
    t.action = action;
}
}  // namespace

void pad_fill_default(Pad& pad) {
    memset(&pad, 0, sizeof(pad));
    strncpy(pad.name, "TriggerGrid", PAD_NAME_BYTES);
    pad.density = PAD_DENSITY_REGULAR;
    pad.brightness = 180;
    pad.dim_sec = 120;

    // Icon numbers follow web/lib/icons.js (see ui/icons.h).
    PadPage& edit = add_page(pad, "Editing");
    add_tile(edit, 0, 0, 1, 1, "Copy",  1, 0x3D8BFF, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL, 0x06));
    add_tile(edit, 1, 0, 1, 1, "Paste", 2, 0x3D8BFF, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL, 0x19));
    add_tile(edit, 2, 0, 2, 1, "Email sig", 3, 0x2F2942, 0xF2F3F4, 0x9B6BFF, true,
             keystrokes(kEmailSig, sizeof(kEmailSig) / sizeof(kEmailSig[0]), 10));
    add_tile(edit, 4, 0, 1, 1, "Undo",  4, 0x2C3034, 0xF2F3F4, 0xF2F3F4, false, chord(MOD_CTRL, 0x1D));
    add_tile(edit, 0, 1, 2, 2, "Play / Pause", 5, 0xC6F432, 0x0E0F10, 0x0E0F10, false, media(MEDIA_PLAY_PAUSE));
    add_tile(edit, 2, 1, 1, 1, "Vol +", 8, 0x3EE6A8, 0x0E0F10, 0x0E0F10, false, media(MEDIA_VOLUME_UP));
    add_tile(edit, 3, 1, 1, 1, "Mute", 10, 0x3F2628, 0xF2F3F4, 0xFF5C5C, true, media(MEDIA_MUTE));
    add_tile(edit, 4, 1, 1, 2, "Screen shot", 11, 0xFFD60A, 0x0E0F10, 0x0E0F10, false, chord(MOD_GUI | MOD_SHIFT, 0x16));
    add_tile(edit, 2, 2, 1, 1, "Vol -", 9, 0x3EE6A8, 0x0E0F10, 0x0E0F10, false, media(MEDIA_VOLUME_DOWN));
    add_tile(edit, 3, 2, 1, 1, "Lock", 12, 0xC8233B, 0xF2F3F4, 0xF2F3F4, false, chord(MOD_GUI, 0x0F));

    PadPage& mediap = add_page(pad, "Media");
    add_tile(mediap, 0, 0, 1, 1, "Prev", 7, 0x1D393E, 0xF2F3F4, 0x2BD4E6, true, media(MEDIA_PREV_TRACK));
    add_tile(mediap, 1, 0, 2, 2, "Play / Pause", 5, 0xF24FD1, 0x0E0F10, 0x0E0F10, false, media(MEDIA_PLAY_PAUSE));
    add_tile(mediap, 3, 0, 1, 1, "Next", 6, 0x1D393E, 0xF2F3F4, 0x2BD4E6, true, media(MEDIA_NEXT_TRACK));
    add_tile(mediap, 4, 0, 1, 1, "Mic mute", 16, 0xFF5C5C, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL | MOD_SHIFT, 0x10));
    add_tile(mediap, 0, 1, 1, 1, "Vol -", 9, 0x2C3034, 0xF2F3F4, 0xF2F3F4, false, media(MEDIA_VOLUME_DOWN));
    add_tile(mediap, 3, 1, 1, 1, "Vol +", 8, 0x2C3034, 0xF2F3F4, 0xF2F3F4, false, media(MEDIA_VOLUME_UP));

    PadPage& code = add_page(pad, "Code");
    add_tile(code, 0, 0, 2, 1, "Terminal", 17, 0x8A96A3, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL, 0x35));
    add_tile(code, 2, 0, 1, 1, "Find", 15, 0x3F2E1E, 0xF2F3F4, 0xFF8A1F, true, chord(MOD_CTRL | MOD_SHIFT, 0x09));
    add_tile(code, 3, 0, 2, 1, "PIO build", 13, 0xC6F432, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL | MOD_ALT, 0x05));
    add_tile(code, 0, 1, 1, 1, "Save", 14, 0x9B6BFF, 0x0E0F10, 0x0E0F10, false, chord(MOD_CTRL, 0x16));
}
