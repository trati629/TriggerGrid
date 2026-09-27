// diag.cpp — see diag.h.

#include "diag.h"

#include <lvgl.h>

// Diagnostic screens use plain black and white rather than theme colours,
// so they work before the theme exists.
static lv_obj_t* add_label(lv_obj_t* parent, const char* text,
                           lv_align_t align, int32_t x_ofs, int32_t y_ofs) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, align, x_ofs, y_ofs);
    return label;
}

void diag_show_corners() {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    // A 1 px white border on the screen itself shows that no edge is cut off.
    lv_obj_set_style_border_width(screen, 1, 0);
    lv_obj_set_style_border_color(screen, lv_color_white(), 0);

    add_label(screen, "TL", LV_ALIGN_TOP_LEFT, 6, 4);
    add_label(screen, "TR", LV_ALIGN_TOP_RIGHT, -6, 4);
    add_label(screen, "BL", LV_ALIGN_BOTTOM_LEFT, 6, -4);
    add_label(screen, "BR", LV_ALIGN_BOTTOM_RIGHT, -6, -4);
    add_label(screen, "centre\n480 x 320", LV_ALIGN_CENTER, 0, 0);
}
