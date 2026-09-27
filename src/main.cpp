// TriggerGrid — touchscreen BLE macro pad for the Guition JC3248W535.
//
// Entry point. Each module listed in docs/architecture.md is brought up in
// order by the milestones in docs/roadmap.md. M6: the layout comes from
// /config.json, compiled by the browser.

#include <Arduino.h>
#include "actions/actions.h"
#include "board_pins.h"
#include "config/config.h"
#include "display/display.h"
#include "hid/hid.h"
#include "lvgl_glue/lvgl_glue.h"
#include "touch/touch.h"
#include "ui/ui.h"

constexpr uint32_t BACKLIGHT_RAMP_MS = 1000;
constexpr uint32_t STATS_PERIOD_MS = 5000;

// Stop here with a message on serial. Used when the board can't run at all.
static void halt(const char* why) {
    Serial.println(why);
    while (true) {
        delay(1000);
    }
}

// A tile was tapped: queue its action. False makes the tile flash red.
static bool on_tile_tap(const PadTile& tile) {
    bool sent = actions_run(tile.action);
    Serial.printf("tap: %s (%s)\n", tile.label,
                  sent ? "sent" : "not sent: Bluetooth not connected");
    return sent;
}

// Draw the current layout, with a status-bar warning if /config.json was
// rejected (the full reason is on serial).
static void show_layout() {
    const Pad& pad = config_pad();
    ui_build(pad);
    ui_set_warning(config_warning() ? "Layout error: built-in used" : nullptr);
}

// Show the Bluetooth state in the status bar whenever it changes.
static void update_ble_status() {
    static bool first = true;
    static bool shown = false;
    bool connected = hid_connected();
    if (first || connected != shown) {
        ui_set_ble_state(connected ? LinkState::Ok : LinkState::Waiting);
        shown = connected;
        first = false;
    }
}

void setup() {
    // Backlight off first, so nothing flashes on screen while the panel starts.
    display_set_brightness(0);

    Serial.begin(115200);
    delay(500);   // give USB CDC a moment to enumerate

    Serial.println();
    Serial.println("TriggerGrid");
    Serial.printf("  Chip:  %s rev %d, %d cores @ %lu MHz\n",
                  ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("  Flash: %lu KB\n", (unsigned long)(ESP.getFlashChipSize() / 1024));
    Serial.printf("  PSRAM: %lu KB (%s)\n", (unsigned long)(ESP.getPsramSize() / 1024),
                  psramFound() ? "ok" : "MISSING - check BOARD_HAS_PSRAM / qio_opi");
    if (!psramFound()) {
        halt("halt: the frame buffers need PSRAM");
    }

    if (!display_init()) {
        halt("display: init FAILED (see hardware doc §P1)");
    }
    if (!touch_init()) {
        // Keep going: the screen still works, and serial says what's wrong.
        Serial.println("touch: no answer from the chip at 0x3B (check I2C pins)");
    }
    if (!lvgl_glue_init()) {
        halt("lvgl: could not allocate frame buffers");
    }

    if (!config_init()) {
        halt("config: no PSRAM for the layout");
    }
    const Pad& pad = config_pad();
    ui_on_tile_tap(on_tile_tap);
    show_layout();

    if (!actions_init()) {
        halt("actions: could not start the action task");
    }
    if (!hid_init(pad.name)) {
        Serial.println("ble: could not start advertising");
    }
    Serial.printf("ble: advertising as \"%s\"\n", pad.name);

    lvgl_glue_update();   // render the first frame before the backlight comes up
    display_ramp_backlight(pad.brightness, BACKLIGHT_RAMP_MS);
    Serial.printf("ui: %u pages\n", pad.page_count);
}

void loop() {
    lvgl_glue_update();
    update_ble_status();

    // A new config.json was saved: reload it and rebuild the screens at once
    // (the UI points into the Pad, see config_reload()).
    if (config_take_dirty()) {
        config_reload();
        show_layout();
        display_set_brightness(config_pad().brightness);
    }

    static uint32_t last_stats = 0;
    if (millis() - last_stats >= STATS_PERIOD_MS) {
        last_stats = millis();
        FlushStats s = lvgl_glue_flush_stats();
        Serial.printf("frame: transpose %lu us + push %lu us (%lu frames) | heap %lu, PSRAM %lu\n",
                      (unsigned long)s.transpose_us, (unsigned long)s.push_us,
                      (unsigned long)s.frames, (unsigned long)ESP.getFreeHeap(),
                      (unsigned long)ESP.getFreePsram());
    }
}
