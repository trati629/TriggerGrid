// menu.cpp — see menu.h. Panel style: docs/style-guide.md (surface-2 sheet,
// 12 px panel radius, 8 px control radius, neutral buttons).

#include "menu.h"

#include <Arduino.h>

#include "display/display.h"
#include "hid/hid.h"
#include "net/net.h"
#include "theme.h"
#include "version.h"

constexpr uint32_t SETUP_HOLD_MS = 3000;
constexpr int32_t  QR_SIZE = 120;

static lv_obj_t*   s_menu = nullptr;       // the open menu, or null
static lv_obj_t*   s_wifi_text = nullptr;
static lv_obj_t*   s_about_text = nullptr;
static lv_obj_t*   s_qr = nullptr;
static lv_timer_t* s_refresh = nullptr;
static uint32_t    s_press_ms = 0;
static bool        s_hold_fired = false;

// ---- widgets in the style guide's look --------------------------------------

static lv_obj_t* add_text(lv_obj_t* parent, const char* text, const lv_font_t* font, lv_color_t color) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

// Secondary button: 1 px `line` outline, `surface-3` while pressed.
static lv_obj_t* add_button(lv_obj_t* parent, const char* text, lv_event_cb_t on_click) {
    lv_obj_t* button = lv_obj_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_set_size(button, LV_SIZE_CONTENT, 36);
    lv_obj_set_style_pad_hor(button, 14, 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, theme::line(), 0);
    lv_obj_set_style_bg_color(button, theme::surface3(), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, on_click, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* label = add_text(button, text, &lv_font_montserrat_14, theme::text());
    lv_obj_center(label);
    return button;
}

// A column that stacks its children with a gap.
static lv_obj_t* add_column(lv_obj_t* parent, int32_t width) {
    lv_obj_t* col = lv_obj_create(parent);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, width, LV_SIZE_CONTENT);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 10, 0);
    return col;
}

// ---- live text ------------------------------------------------------------

static void refresh(lv_timer_t* timer) {
    (void)timer;
    char ip[20];
    net_ip(ip, sizeof(ip));

    switch (net_mode()) {
        case NetMode::AccessPoint:
            lv_label_set_text_fmt(s_wifi_text,
                                  "Setup hotspot\n%s\nPassword: %s\nthen open http://%s",
                                  net_ssid(), net_ap_password(), ip);
            break;
        case NetMode::Station:
            if (net_connected()) {
                lv_label_set_text_fmt(s_wifi_text, "Connected to %s\nhttp://%s.local\n%s  (%d dBm)",
                                      net_ssid(), net_hostname(), ip, net_rssi());
            } else {
                lv_label_set_text_fmt(s_wifi_text, "Reconnecting to %s", net_ssid());
            }
            break;
        default:
            lv_label_set_text_fmt(s_wifi_text, "Joining %s...", net_ssid());
            break;
    }

    // The QR code joins the hotspot, so it only shows in hotspot mode.
    lv_obj_set_hidden(s_qr, net_mode() != NetMode::AccessPoint);

    uint32_t up = millis() / 1000;
    lv_label_set_text_fmt(s_about_text,
                          "Firmware %s\nUp %lu h %02lu m\nBluetooth: %s\nFree heap %lu KB, PSRAM %lu KB",
                          FW_VERSION, (unsigned long)(up / 3600), (unsigned long)(up / 60 % 60),
                          hid_connected() ? "connected" : "waiting for a computer",
                          (unsigned long)(ESP.getFreeHeap() / 1024),
                          (unsigned long)(ESP.getFreePsram() / 1024));
}

static void update_qr() {
    // Standard Wi-Fi QR payload: phones offer to join when they scan it.
    char payload[128];
    snprintf(payload, sizeof(payload), "WIFI:T:WPA;S:%s;P:%s;;", net_ssid(), net_ap_password());
    lv_qrcode_update(s_qr, payload, strlen(payload));
}

// ---- actions ----------------------------------------------------------------

static void close_menu() {
    if (s_refresh) {
        lv_timer_delete(s_refresh);
        s_refresh = nullptr;
    }
    if (s_menu) {
        lv_obj_delete(s_menu);
        s_menu = nullptr;
    }
}

static void on_close(lv_event_t* e) {
    (void)e;
    close_menu();
}

static void on_brightness(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    // Until the next reboot or layout save; the saved value is in config.json.
    display_set_brightness((uint8_t)lv_slider_get_value(slider));
}

static void on_forget_ble(lv_event_t* e) {
    (void)e;
    hid_forget_bonds();
    Serial.println("menu: Bluetooth pairings cleared");
}

static void on_setup_hotspot(lv_event_t* e) {
    (void)e;
    net_force_setup_hotspot();
    update_qr();
    refresh(nullptr);
}

static void on_reboot(lv_event_t* e) {
    (void)e;
    ESP.restart();
}

static void open_menu() {
    if (s_menu) {
        return;
    }
    // A full-screen sheet on top of everything, on the active screen's top layer.
    s_menu = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_menu);
    lv_obj_set_size(s_menu, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_menu, theme::surface2(), 0);
    lv_obj_set_style_bg_opa(s_menu, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_menu, 16, 0);
    lv_obj_set_scrollable(s_menu, false);
    lv_obj_set_clickable(s_menu, true);   // don't let taps reach the tiles below

    lv_obj_t* title = add_text(s_menu, "Device", &lv_font_montserrat_20, theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t* close = add_button(s_menu, "Close", on_close);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, 0, -6);

    // Left: brightness, buttons, About. Right: Wi-Fi and the QR code.
    lv_obj_t* left = add_column(s_menu, 230);
    lv_obj_align(left, LV_ALIGN_TOP_LEFT, 0, 40);

    add_text(left, "Brightness", &lv_font_montserrat_12, theme::text_muted());
    lv_obj_t* slider = lv_slider_create(left);
    lv_obj_remove_style_all(slider);
    lv_obj_set_size(slider, 210, 6);
    lv_obj_set_style_margin_ver(slider, 8, 0);
    lv_slider_set_range(slider, 10, 255);
    lv_slider_set_value(slider, display_get_brightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, theme::surface3(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, theme::text(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, theme::text(), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 7, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, on_brightness, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* row = lv_obj_create(left);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(row, 8, 0);
    add_button(row, "Forget Bluetooth", on_forget_ble);
    add_button(row, "Setup hotspot", on_setup_hotspot);
    add_button(row, "Reboot", on_reboot);

    s_about_text = add_text(left, "", &lv_font_montserrat_12, theme::text_muted());

    lv_obj_t* right = add_column(s_menu, 200);
    lv_obj_align(right, LV_ALIGN_TOP_RIGHT, 0, 40);
    add_text(right, "Wi-Fi", &lv_font_montserrat_12, theme::text_muted());
    s_wifi_text = add_text(right, "", &lv_font_montserrat_14, theme::text());
    lv_label_set_long_mode(s_wifi_text, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(s_wifi_text, 200);

    s_qr = lv_qrcode_create(right);
    lv_qrcode_set_size(s_qr, QR_SIZE);
    lv_qrcode_set_dark_color(s_qr, theme::bg());
    lv_qrcode_set_light_color(s_qr, theme::text());
    // A light margin around the code: scanners need a quiet zone.
    lv_obj_set_style_border_width(s_qr, 6, 0);
    lv_obj_set_style_border_color(s_qr, theme::text(), 0);
    update_qr();

    refresh(nullptr);
    s_refresh = lv_timer_create(refresh, 1000, nullptr);
}

// ---- status bar gestures ----------------------------------------------------

static void on_status_bar(lv_event_t* e) {
    switch (lv_event_get_code(e)) {
        case LV_EVENT_PRESSED:
            s_press_ms = millis();
            s_hold_fired = false;
            break;
        case LV_EVENT_PRESSING:
            if (!s_hold_fired && millis() - s_press_ms >= SETUP_HOLD_MS) {
                s_hold_fired = true;
                Serial.println("menu: status bar held, starting the setup hotspot");
                net_force_setup_hotspot();
                open_menu();
            }
            break;
        case LV_EVENT_CLICKED:
            if (!s_hold_fired) {
                open_menu();
            }
            break;
        default:
            break;
    }
}

void menu_attach(lv_obj_t* status_bar) {
    // ui_build() rebuilt the screen; a menu left open would point at stale state.
    close_menu();
    lv_obj_set_clickable(status_bar, true);
    lv_obj_add_event_cb(status_bar, on_status_bar, LV_EVENT_ALL, nullptr);
}
