// actions.cpp — see actions.h. Playback rules: docs/config-schema.md,
// "Compiled actions" and "Keystroke rules".

#include "actions.h"

#include <Arduino.h>

#include "hid/hid.h"

// How long a chord or media key is held down. Long enough for every OS to
// see it, short enough to feel instant.
constexpr uint32_t HOLD_MS = 20;

constexpr uint8_t USAGE_NUM_LOCK = 0x53;
constexpr UBaseType_t QUEUE_DEPTH = 8;

static QueueHandle_t s_queue = nullptr;

// ---- playback ---------------------------------------------------------------

static void tap_key(uint8_t modifiers, uint8_t key, uint32_t delay_ms) {
    hid_send_keys(modifiers, &key, 1);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    hid_send_keys(modifiers, nullptr, 0);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

static void play_chord(const PadAction& a) {
    hid_send_keys(a.modifiers, a.keys, a.key_count);
    vTaskDelay(pdMS_TO_TICKS(HOLD_MS));
    hid_release_keys();
}

static void play_consumer(const PadAction& a) {
    hid_send_consumer(a.usage);
    vTaskDelay(pdMS_TO_TICKS(HOLD_MS));
    hid_send_consumer(0);
}

// Each keystroke is 0xMMKK: modifier byte, key usage.
//  - Modifiers stay held across keystrokes that share them. Windows Alt
//    codes and macOS Option sequences depend on this.
//  - A different modifier byte releases everything first.
//  - 0x0000 releases everything: it ends an Alt or Option sequence.
//  - If the list uses numpad keys and NumLock is off, NumLock is turned on
//    for the duration and restored after.
static void play_keystrokes(const PadAction& a) {
    const uint32_t d = a.delay_ms;

    bool toggled_numlock = false;
    if (a.numpad && !hid_numlock_on()) {
        tap_key(0, USAGE_NUM_LOCK, d);
        toggled_numlock = true;
    }

    uint8_t held = 0;   // modifiers currently held down
    for (uint16_t i = 0; i < a.stroke_count && hid_connected(); i++) {
        const uint8_t modifiers = a.strokes[i] >> 8;
        const uint8_t key = a.strokes[i] & 0xFF;

        if (modifiers == 0 && key == 0) {
            hid_release_keys();
            held = 0;
            vTaskDelay(pdMS_TO_TICKS(d));
            continue;
        }
        if (modifiers != held) {
            if (held) {
                hid_release_keys();
                vTaskDelay(pdMS_TO_TICKS(d));
            }
            if (modifiers) {
                // Press the modifiers on their own first, like a person would.
                hid_send_keys(modifiers, nullptr, 0);
                vTaskDelay(pdMS_TO_TICKS(d));
            }
            held = modifiers;
        }
        tap_key(modifiers, key, d);
    }
    hid_release_keys();

    if (toggled_numlock) {
        vTaskDelay(pdMS_TO_TICKS(d));
        tap_key(0, USAGE_NUM_LOCK, d);
    }
}

// ---- task -------------------------------------------------------------------

// What goes on the queue: the action plus its own copy of the keystrokes.
struct Job {
    PadAction action;
    uint16_t* strokes;   // PSRAM copy, freed after the job runs (or null)
};

static void actions_task(void*) {
    Job job;
    for (;;) {
        if (xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (hid_connected()) {
            switch (job.action.type) {
                case ActionType::Chord:      play_chord(job.action); break;
                case ActionType::Consumer:   play_consumer(job.action); break;
                case ActionType::Keystrokes: play_keystrokes(job.action); break;
                default: break;
            }
        }
        free(job.strokes);
    }
}

bool actions_init() {
    s_queue = xQueueCreate(QUEUE_DEPTH, sizeof(Job));
    if (!s_queue) {
        return false;
    }
    return xTaskCreatePinnedToCore(actions_task, "actions", 4096, nullptr, 2, nullptr, 0) == pdPASS;
}

bool actions_run(const PadAction& action) {
    if (action.type == ActionType::None || !hid_connected()) {
        return false;
    }

    Job job;
    job.action = action;
    job.strokes = nullptr;
    if (action.type == ActionType::Keystrokes && action.stroke_count > 0) {
        const size_t bytes = action.stroke_count * sizeof(uint16_t);
        job.strokes = (uint16_t*)ps_malloc(bytes);
        if (!job.strokes) {
            return false;
        }
        memcpy(job.strokes, action.strokes, bytes);
        job.action.strokes = job.strokes;
    }

    if (xQueueSend(s_queue, &job, 0) != pdTRUE) {
        free(job.strokes);
        return false;
    }
    return true;
}
