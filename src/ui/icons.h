// icons.h — tile icon numbers → images.
//
// The browser sends an icon as a number: its position in web/lib/icons.js
// (1 = copy, 2 = paste, …; 0 = no icon). icon_images.c holds those same SVG
// icons drawn as A8 (alpha) images by tools/make_icons.js, so the pad shows
// exactly what the web editor previews. Re-run the tool after adding icons.
#pragma once

#include <lvgl.h>
#include <stdint.h>

extern "C" {
extern const lv_image_dsc_t* const icon_images_20[];   // regular grid
extern const lv_image_dsc_t* const icon_images_18[];   // compact grid
extern const unsigned icon_image_count;
}

// The image for icon `index` at `size` px (20 or 18), or nullptr for none
// or an unknown number (a newer web page may know more icons).
inline const lv_image_dsc_t* icon_image(uint8_t index, int32_t size) {
    if (index == 0 || index >= icon_image_count) {
        return nullptr;
    }
    return size == 18 ? icon_images_18[index] : icon_images_20[index];
}
