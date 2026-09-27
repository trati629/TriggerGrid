// diag.cpp — see diag.h.

#include "diag.h"

#include <Arduino.h>
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

// ---- M3: touch test -------------------------------------------------------

static lv_obj_t* s_dot = nullptr;

// The dot follows the finger anywhere on the screen.
static void on_screen_touch(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_active();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_obj_set_pos(s_dot, p.x - 10, p.y - 10);
    lv_obj_set_hidden(s_dot, false);
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        Serial.printf("touch: released at %ld, %ld\n", (long)p.x, (long)p.y);
    }
}

// Each corner button counts its own clicks in its label.
static void on_corner_click(lv_event_t* e) {
    lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(button, 0);
    uintptr_t count = (uintptr_t)lv_obj_get_user_data(button) + 1;
    lv_obj_set_user_data(button, (void*)count);
    const char* name = (const char*)lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "%s %lu", name, (unsigned long)count);
    Serial.printf("touch: %s clicked (%lu)\n", name, (unsigned long)count);
}

static void add_corner_button(lv_obj_t* parent, const char* name, lv_align_t align,
                              int32_t x_ofs, int32_t y_ofs) {
    lv_obj_t* button = lv_button_create(parent);
    lv_obj_set_size(button, 110, 64);
    lv_obj_align(button, align, x_ofs, y_ofs);
    lv_obj_set_user_data(button, (void*)0);
    lv_obj_add_event_cb(button, on_corner_click, LV_EVENT_CLICKED, (void*)name);

    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text_fmt(label, "%s 0", name);
    lv_obj_center(label);
}

void diag_show_touch() {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_scrollable(screen, false);
    lv_obj_add_event_cb(screen, on_screen_touch, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(screen, on_screen_touch, LV_EVENT_RELEASED, nullptr);

    add_label(screen, "Touch test\ndrag anywhere, tap the corners", LV_ALIGN_CENTER, 0, 0);
    add_corner_button(screen, "TL", LV_ALIGN_TOP_LEFT, 4, 4);
    add_corner_button(screen, "TR", LV_ALIGN_TOP_RIGHT, -4, 4);
    add_corner_button(screen, "BL", LV_ALIGN_BOTTOM_LEFT, 4, -4);
    add_corner_button(screen, "BR", LV_ALIGN_BOTTOM_RIGHT, -4, -4);

    s_dot = lv_obj_create(screen);
    lv_obj_remove_style_all(s_dot);
    lv_obj_set_size(s_dot, 20, 20);
    lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_dot, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_dot, LV_OPA_COVER, 0);
    lv_obj_set_hidden(s_dot, true);
    // Let presses on the dot reach the screen underneath.
    lv_obj_set_clickable(s_dot, false);
}
