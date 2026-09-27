// power.cpp — see power.h. Style guide: no idle animations; the pad just
// goes quiet. Only loop() calls these (they use LVGL's inactivity timer).

#include "power.h"

#include <Arduino.h>
#include <lvgl.h>

#include "display/display.h"

constexpr uint32_t OFF_AFTER_DIM_MS = 5 * 60 * 1000;
constexpr uint8_t  DIM_PERCENT = 20;
constexpr uint8_t  MIN_LEVEL = 10;

enum class Screen { Awake, Dimmed, Off };

static Screen   s_screen = Screen::Awake;
static uint8_t  s_level = 180;
static uint32_t s_dim_after_ms = 0;

static uint8_t dim_level() {
    uint16_t level = (uint16_t)s_level * DIM_PERCENT / 100;
    return level < MIN_LEVEL ? MIN_LEVEL : (uint8_t)level;
}

void power_set_level(uint8_t level) {
    s_level = level < MIN_LEVEL ? MIN_LEVEL : level;
    if (s_screen == Screen::Awake) {
        display_set_brightness(s_level);
    }
}

uint8_t power_level() {
    return s_level;
}

void power_set_dim_after(uint16_t seconds) {
    s_dim_after_ms = (uint32_t)seconds * 1000;
}

void power_update() {
    if (s_dim_after_ms == 0) {
        return;   // "Never" in the settings
    }
    const uint32_t idle = lv_display_get_inactive_time(nullptr);

    if (s_screen == Screen::Awake && idle >= s_dim_after_ms) {
        s_screen = Screen::Dimmed;
        display_set_brightness(dim_level());
        Serial.printf("power: dimmed after %lu s idle\n", (unsigned long)(idle / 1000));
    } else if (s_screen == Screen::Dimmed && idle >= s_dim_after_ms + OFF_AFTER_DIM_MS) {
        s_screen = Screen::Off;
        display_set_brightness(0);
        Serial.println("power: screen off");
    } else if (s_screen != Screen::Awake && idle < s_dim_after_ms) {
        power_wake();   // a touch LVGL saw while dimmed
    }
}

bool power_screen_off() {
    return s_screen == Screen::Off;
}

void power_wake() {
    if (s_screen != Screen::Awake) {
        Serial.println("power: awake");
    }
    s_screen = Screen::Awake;
    display_set_brightness(s_level);
    lv_display_trigger_activity(nullptr);
}

void power_log_status() {
    const char* state = s_screen == Screen::Awake ? "awake" : s_screen == Screen::Dimmed ? "dimmed" : "off";
    Serial.printf("power: %s, idle %lu s, dims after %lu s%s\n", state,
                  (unsigned long)(lv_display_get_inactive_time(nullptr) / 1000),
                  (unsigned long)(s_dim_after_ms / 1000), s_dim_after_ms ? "" : " (never)");
}
