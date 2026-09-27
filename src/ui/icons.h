// icons.h — tile icon numbers → glyphs.
//
// The browser sends an icon as a number: its position in web/lib/icons.js
// (1 = copy, 2 = paste, …; 0 = no icon). Until the custom icon font exists
// (roadmap M10), each number maps to the closest built-in LV_SYMBOL glyph.
#pragma once

#include <lvgl.h>
#include <stdint.h>

inline const char* icon_symbol(uint8_t index) {
    static const char* const kSymbols[] = {
        nullptr,               //  0 none
        LV_SYMBOL_COPY,        //  1 copy
        LV_SYMBOL_PASTE,       //  2 paste
        LV_SYMBOL_ENVELOPE,    //  3 mail
        LV_SYMBOL_REFRESH,     //  4 undo
        LV_SYMBOL_PLAY,        //  5 playpause
        LV_SYMBOL_NEXT,        //  6 next
        LV_SYMBOL_PREV,        //  7 prev
        LV_SYMBOL_VOLUME_MAX,  //  8 volup
        LV_SYMBOL_VOLUME_MID,  //  9 voldown
        LV_SYMBOL_MUTE,        // 10 mute
        LV_SYMBOL_IMAGE,       // 11 shot
        LV_SYMBOL_EYE_CLOSE,   // 12 lock
        LV_SYMBOL_LIST,        // 13 code
        LV_SYMBOL_SAVE,        // 14 save
        LV_SYMBOL_EYE_OPEN,    // 15 search
        LV_SYMBOL_AUDIO,       // 16 mic
        LV_SYMBOL_KEYBOARD,    // 17 terminal
        LV_SYMBOL_KEYBOARD,    // 18 keyboard
        LV_SYMBOL_EDIT,        // 19 type
    };
    constexpr uint8_t count = sizeof(kSymbols) / sizeof(kSymbols[0]);
    return index < count ? kSymbols[index] : nullptr;
}
