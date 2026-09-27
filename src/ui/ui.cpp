// ui.cpp — see ui.h. Layout numbers from docs/style-guide.md.

#include "ui.h"

#include "fonts/fonts.h"
#include "theme.h"
#include "tile.h"

static const Pad*     s_pad = nullptr;
static TileTapHandler s_tap_handler = nullptr;

static lv_obj_t* s_status_bar = nullptr;
static lv_obj_t* s_page_name  = nullptr;
static lv_obj_t* s_warning    = nullptr;
static lv_obj_t* s_wifi_icon  = nullptr;
static lv_obj_t* s_ble_icon   = nullptr;
static lv_obj_t* s_pages      = nullptr;   // the tileview
static lv_obj_t* s_dots[PAD_MAX_PAGES] = {};

// Remembered across ui_build() so a layout reload keeps the status icons.
static LinkState   s_wifi_state = LinkState::Off;
static LinkState   s_ble_state  = LinkState::Off;
static const char* s_warning_text = nullptr;

static lv_color_t link_color(LinkState state) {
    switch (state) {
        case LinkState::Waiting: return theme::warn();
        case LinkState::Ok:      return theme::ok();
        case LinkState::Failed:  return theme::error();
        default:                 return theme::text_faint();
    }
}

// ---- status bar ----------------------------------------------------------

// Text uses Space Grotesk; the Wi-Fi and Bluetooth icons need Montserrat's
// symbol glyphs.
static lv_obj_t* add_bar_label(lv_obj_t* bar, const char* text, lv_color_t color,
                               const lv_font_t* font = &space_grotesk_regular_12) {
    lv_obj_t* label = lv_label_create(bar);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static void build_status_bar(lv_obj_t* screen) {
    s_status_bar = lv_obj_create(screen);
    lv_obj_remove_style_all(s_status_bar);
    lv_obj_set_size(s_status_bar, LV_PCT(100), theme::STATUS_BAR_H);
    lv_obj_set_style_bg_color(s_status_bar, theme::surface2(), 0);
    lv_obj_set_style_bg_opa(s_status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(s_status_bar, 10, 0);
    lv_obj_set_style_pad_column(s_status_bar, 6, 0);
    lv_obj_set_scrollable(s_status_bar, false);

    // Page name, warning, spacer, then the link icons on the right.
    lv_obj_set_layout(s_status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    s_page_name = add_bar_label(s_status_bar, "", theme::text());
    s_warning = add_bar_label(s_status_bar, "", theme::warn());

    lv_obj_t* spacer = lv_obj_create(s_status_bar);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_height(spacer, 1);
    lv_obj_set_flex_grow(spacer, 1);

    s_wifi_icon = add_bar_label(s_status_bar, LV_SYMBOL_WIFI, link_color(s_wifi_state), &lv_font_montserrat_12);
    s_ble_icon = add_bar_label(s_status_bar, LV_SYMBOL_BLUETOOTH, link_color(s_ble_state), &lv_font_montserrat_12);

    ui_set_warning(s_warning_text);
}

// ---- pages ------------------------------------------------------------------

static void on_tile_click(lv_event_t* e) {
    lv_obj_t* tile = (lv_obj_t*)lv_event_get_current_target(e);
    const PadTile* t = (const PadTile*)lv_obj_get_user_data(tile);
    bool sent = s_tap_handler ? s_tap_handler(*t) : false;
    tile_flash(tile, sent);
}

static void show_page(uint32_t index) {
    lv_label_set_text(s_page_name, s_pad->pages[index].name);
    for (uint8_t i = 0; i < s_pad->page_count; i++) {
        lv_obj_set_style_bg_color(s_dots[i], i == index ? theme::text() : theme::text_faint(), 0);
    }
}

static void on_page_changed(lv_event_t* e) {
    (void)e;
    lv_obj_t* active = lv_tileview_get_tile_active(s_pages);
    show_page(lv_obj_get_index(active));
}

static void build_pages(lv_obj_t* screen, const Pad& pad) {
    const theme::Grid& g = theme::grid(pad.density);

    // One tileview tile per page, side by side; LVGL snaps between them.
    s_pages = lv_tileview_create(screen);
    lv_obj_remove_style_all(s_pages);
    lv_obj_set_pos(s_pages, 0, theme::STATUS_BAR_H);
    lv_obj_set_size(s_pages, LV_PCT(100), theme::GRID_H);
    lv_obj_set_scrollbar_mode(s_pages, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_elastic(s_pages, false);   // no rubber-banding past the last page
    lv_obj_add_event_cb(s_pages, on_page_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    for (uint8_t p = 0; p < pad.page_count; p++) {
        lv_dir_t dir = LV_DIR_NONE;
        if (pad.page_count > 1) {
            dir = (p == 0) ? LV_DIR_RIGHT : (p == pad.page_count - 1) ? LV_DIR_LEFT : LV_DIR_HOR;
        }
        lv_obj_t* page = lv_tileview_add_tile(s_pages, p, 0, dir);
        lv_obj_remove_style_all(page);
        lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
        lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

        const PadPage& pg = pad.pages[p];
        for (uint8_t i = 0; i < pg.tile_count; i++) {
            lv_obj_t* tile = tile_create(page, pg.tiles[i], g);
            lv_obj_add_event_cb(tile, on_tile_click, LV_EVENT_CLICKED, nullptr);
        }
    }
}

static void build_dots(lv_obj_t* screen, const Pad& pad) {
    lv_obj_t* row = lv_obj_create(screen);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), theme::DOTS_H);
    lv_obj_set_pos(row, 0, theme::STATUS_BAR_H + theme::GRID_H);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, theme::DOT_GAP, 0);
    lv_obj_set_hidden(row, pad.page_count < 2);   // no dots for a single page

    for (uint8_t i = 0; i < pad.page_count; i++) {
        s_dots[i] = lv_obj_create(row);
        lv_obj_remove_style_all(s_dots[i]);
        lv_obj_set_size(s_dots[i], theme::DOT_SIZE, theme::DOT_SIZE);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
    }
}

// ---- public -----------------------------------------------------------------

void ui_build(const Pad& pad) {
    s_pad = &pad;

    lv_obj_t* screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, theme::bg(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(screen, false);

    build_status_bar(screen);
    build_pages(screen, pad);
    build_dots(screen, pad);
    show_page(0);
}

void ui_on_tile_tap(TileTapHandler handler) {
    s_tap_handler = handler;
}

void ui_set_wifi_state(LinkState state) {
    s_wifi_state = state;
    if (s_wifi_icon) {
        lv_obj_set_style_text_color(s_wifi_icon, link_color(state), 0);
    }
}

void ui_set_ble_state(LinkState state) {
    s_ble_state = state;
    if (s_ble_icon) {
        lv_obj_set_style_text_color(s_ble_icon, link_color(state), 0);
    }
}

void ui_set_warning(const char* text) {
    s_warning_text = text;
    if (s_warning) {
        lv_label_set_text(s_warning, text ? text : "");
        lv_obj_set_hidden(s_warning, text == nullptr);
    }
}

lv_obj_t* ui_status_bar() {
    return s_status_bar;
}
