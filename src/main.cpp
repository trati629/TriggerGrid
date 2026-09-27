// TriggerGrid — touchscreen BLE macro pad for the Guition JC3248W535.
//
// Entry point. Each module listed in docs/architecture.md is brought up in
// order by the milestones in docs/roadmap.md. M2: LVGL draws a landscape
// test screen through the transposing flush.

#include <Arduino.h>
#include "board_pins.h"
#include "diag/diag.h"
#include "display/display.h"
#include "lvgl_glue/lvgl_glue.h"

constexpr uint8_t  BOOT_BRIGHTNESS = 180;   // matches the config default
constexpr uint32_t BACKLIGHT_RAMP_MS = 1000;
constexpr uint32_t STATS_PERIOD_MS = 5000;

// Stop here with a message on serial. Used when the board can't run at all.
static void halt(const char* why) {
    Serial.println(why);
    while (true) {
        delay(1000);
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
    if (!lvgl_glue_init()) {
        halt("lvgl: could not allocate frame buffers");
    }

    diag_show_corners();
    lvgl_glue_update();   // render the first frame before the backlight comes up
    display_ramp_backlight(BOOT_BRIGHTNESS, BACKLIGHT_RAMP_MS);
    Serial.println("lvgl: corner test screen shown");
}

void loop() {
    lvgl_glue_update();

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
