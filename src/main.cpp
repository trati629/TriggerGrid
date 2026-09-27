// TriggerGrid — touchscreen BLE macro pad for the Guition JC3248W535.
//
// Skeleton entry point. Each module listed in docs/architecture.md is brought
// up in order by the milestones in docs/roadmap.md; until then this sketch
// only proves the toolchain, PSRAM and serial output work.

#include <Arduino.h>
#include "board_pins.h"

void setup() {
    Serial.begin(115200);
    delay(500);   // give USB CDC a moment to enumerate

    // Backlight off until the display is initialised (milestone M1).
    pinMode(PIN_LCD_BL, OUTPUT);
    analogWrite(PIN_LCD_BL, 0);

    Serial.println();
    Serial.println("TriggerGrid skeleton");
    Serial.printf("  Chip:  %s rev %d, %d cores @ %lu MHz\n",
                  ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("  Flash: %lu KB\n", (unsigned long)(ESP.getFlashChipSize() / 1024));
    Serial.printf("  PSRAM: %lu KB (%s)\n", (unsigned long)(ESP.getPsramSize() / 1024),
                  psramFound() ? "ok" : "MISSING - check BOARD_HAS_PSRAM / qio_opi");
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        Serial.printf("alive, free heap %lu, free PSRAM %lu\n",
                      (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
    }
}
