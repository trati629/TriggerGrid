// tile.cpp — see tile.h. Numbers and motion from docs/style-guide.md.

#include "tile.h"

#include "icons.h"

// Pressed-state changes animate over PRESS_MS, both on press and release.
static const lv_style_prop_t kPressProps[] = {
    LV_STYLE_BG_COLOR, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_PROP_INV,
};
static lv_style_transition_dsc_t s_press_transition;
static bool s_transition_ready = false;

static const lv_style_transition_dsc_t* press_transition() {
    if (!s_transition_ready) {
        lv_style_transition_dsc_init(&s_press_transition, kPressProps, lv_anim_path_ease_out,
                                     theme::PRESS_MS, 0, nullptr);
        s_transition_ready = true;
    }
    return &s_press_transition;
}

lv_obj_t* tile_create(lv_obj_t* page, const PadTile& t, const theme::Grid& g) {
    const int32_t w = t.w * g.tile_w + (t.w - 1) * g.gap;
    const int32_t h = t.h * g.tile_h + (t.h - 1) * g.gap;
    const lv_color_t bg = lv_color_hex(t.bg);

    lv_obj_t* tile = lv_obj_create(page);
    lv_obj_remove_style_all(tile);
    lv_obj_set_pos(tile, g.pad_x + t.x * (g.tile_w + g.gap), g.pad_y + t.y * (g.tile_h + g.gap));
    lv_obj_set_size(tile, w, h);
    lv_obj_set_user_data(tile, (void*)&t);
    lv_obj_set_scrollable(tile, false);   // a drag on a tile scrolls the page instead

    // Flat fill with equal corners; no border, no shadow, no gradient.
    lv_obj_set_style_bg_color(tile, bg, 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tile, g.radius, 0);

    // Press: 20% darker and 96% size. transform_width/height shrink what is
    // drawn without an extra render layer, which a real scale would need.
    lv_obj_set_style_bg_color(tile, lv_color_darken(bg, theme::PRESS_DARKEN), LV_STATE_PRESSED);
    lv_obj_set_style_transform_width(tile, -w * theme::PRESS_SHRINK_PERCENT / 200, LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(tile, -h * theme::PRESS_SHRINK_PERCENT / 200, LV_STATE_PRESSED);
    lv_obj_set_style_transition(tile, press_transition(), 0);
    lv_obj_set_style_transition(tile, press_transition(), LV_STATE_PRESSED);

    // Outline used by tile_flash(); zero width until then.
    lv_obj_set_style_outline_width(tile, 0, 0);
    lv_obj_set_style_outline_pad(tile, 2, 0);

    const lv_image_dsc_t* icon_src = icon_image(t.icon, g.icon_size);
    // A 1×1 compact tile is too small for both: it shows the icon if it has one.
    const bool icon_only = (g.cols == 6 && t.w == 1 && t.h == 1 && icon_src);

    if (!icon_only && t.label[0]) {
        lv_obj_t* label = lv_label_create(tile);
        lv_label_set_text(label, t.label);
        lv_obj_set_style_text_font(label, g.label_font, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(t.fg), 0);
        // Up to two lines, then "…". Text is never scaled down.
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_size(label, w - 2 * g.label_pad, 2 * lv_font_get_line_height(g.label_font));
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, g.label_pad, g.label_pad);
    }

    if (icon_src) {
        // An A8 image is only a shape; LVGL fills it with the recolor colour.
        lv_obj_t* icon = lv_image_create(tile);
        lv_image_set_src(icon, icon_src);
        lv_obj_set_style_image_recolor(icon, lv_color_hex(t.accent), 0);
        lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
        lv_obj_align(icon, LV_ALIGN_BOTTOM_LEFT, g.label_pad, -g.label_pad);
    }

    if (t.bar) {
        // Clip the children to the rounded corners so the bar follows them.
        // Only here: clipping draws the tile through an extra layer.
        lv_obj_set_style_clip_corner(tile, true, 0);

        lv_obj_t* bar = lv_obj_create(tile);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, w, 3);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(t.accent), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_clickable(bar, false);
    }

    return tile;
}

static void end_flash(lv_anim_t* anim) {
    lv_obj_set_style_outline_width((lv_obj_t*)anim->var, 0, 0);
}

void tile_flash(lv_obj_t* tile, bool ok) {
    lv_obj_set_style_outline_color(tile, ok ? theme::ok() : theme::error(), 0);
    lv_obj_set_style_outline_width(tile, 2, 0);

    // An animation that only waits, then clears the outline. Unlike a timer,
    // LVGL deletes it with the tile, so a layout reload can't leave it dangling.
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, tile);
    lv_anim_set_duration(&anim, theme::FLASH_MS);
    lv_anim_set_completed_cb(&anim, end_flash);
    lv_anim_start(&anim);
}
