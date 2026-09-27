// TriggerGrid — touchscreen BLE macro pad for the Guition JC3248W535.
//
// Entry point. Each module listed in docs/architecture.md is brought up in
// order by the milestones in docs/roadmap.md. M1: the display shows a test
// pattern.

#include <Arduino.h>
#include "board_pins.h"
#include "display/display.h"

constexpr uint8_t  BOOT_BRIGHTNESS = 180;   // matches the config default
constexpr uint32_t BACKLIGHT_RAMP_MS = 1000;

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

    if (!display_init()) {
        Serial.println("display: init FAILED (see hardware doc §P1)");
        return;
    }
    if (display_draw_test_pattern()) {
        Serial.println("display: test pattern drawn");
    } else {
        Serial.println("display: no PSRAM for the test frame");
    }
    display_ramp_backlight(BOOT_BRIGHTNESS, BACKLIGHT_RAMP_MS);
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        Serial.printf("alive, free heap %lu, free PSRAM %lu\n",
                      (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
    }
}
