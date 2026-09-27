// touch.cpp — see touch.h. Protocol details: hardware doc, "Touch Read Protocol".

#include "touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"

// No INT and no valid read for this long means the finger has lifted.
// The chip has no "finger up" event (hardware doc, "release detection").
constexpr uint32_t RELEASE_MS = 80;

// While a finger is down, check the chip this often even without an INT,
// so a slow drag keeps updating and a release is noticed on time.
constexpr uint32_t POLL_MS = 20;

static TaskHandle_t s_task = nullptr;

// Shared between the reader task and loop(). Guarded by s_lock.
static portMUX_TYPE s_lock    = portMUX_INITIALIZER_UNLOCKED;
static int16_t      s_x       = 0;
static int16_t      s_y       = 0;
static bool         s_down    = false;
static uint32_t     s_seen_ms = 0;

static void IRAM_ATTR on_touch_int() {
    // Only wake the task here: I2C can't run inside an interrupt handler.
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    portYIELD_FROM_ISR(woken);
}

// One read from the chip, in its own portrait coordinates (0–319, 0–479).
static bool read_point(int16_t* px, int16_t* py) {
    // The chip answers with zeros unless this command comes first, and it
    // must end with a full STOP: endTransmission(false) doesn't work.
    static const uint8_t kReadCmd[8] = {0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, 0x00, 0x08};
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(kReadCmd, sizeof(kReadCmd));
    if (Wire.endTransmission() != 0) {
        return false;
    }

    uint8_t data[8] = {};
    if (Wire.requestFrom(TOUCH_I2C_ADDR, (uint8_t)sizeof(data)) != sizeof(data)) {
        return false;
    }
    for (uint8_t& b : data) {
        b = Wire.read();
    }

    // data[1] is the number of touch points; 0 means nothing is touching.
    if (data[0] != 0 || data[1] == 0) {
        return false;
    }
    *px = ((data[2] & 0x0F) << 8) | data[3];
    *py = ((data[4] & 0x0F) << 8) | data[5];
    return *px < PANEL_W && *py < PANEL_H;
}

// Portrait panel point → landscape screen point. This is the inverse of the
// transpose in lvgl_glue, chosen by the same SCREEN_ROTATE_CW setting.
static void to_landscape(int16_t px, int16_t py, int16_t* x, int16_t* y) {
    if (SCREEN_ROTATE_CW) {
        *x = PANEL_H - 1 - py;
        *y = px;
    } else {
        *x = py;
        *y = PANEL_W - 1 - px;
    }
}

static void touch_task(void*) {
    for (;;) {
        // Sleep until the chip pulses INT. While a finger is down, also wake
        // every POLL_MS: the chip sends only a few pulses per touch.
        bool down;
        portENTER_CRITICAL(&s_lock);
        down = s_down;
        portEXIT_CRITICAL(&s_lock);
        ulTaskNotifyTake(pdTRUE, down ? pdMS_TO_TICKS(POLL_MS) : portMAX_DELAY);

        int16_t px, py, x, y;
        bool got = read_point(&px, &py);
        if (got) {
            to_landscape(px, py, &x, &y);
        }

        portENTER_CRITICAL(&s_lock);
        if (got) {
            s_x = x;
            s_y = y;
            s_down = true;
            s_seen_ms = millis();
        } else if (s_down && millis() - s_seen_ms > RELEASE_MS) {
            s_down = false;
        }
        portEXIT_CRITICAL(&s_lock);
    }
}

bool touch_init() {
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);

    // Is the chip there? An empty write is acknowledged if it is.
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    bool present = (Wire.endTransmission() == 0);

    // PIN_TOUCH_RST is left alone: touch and display are the same chip, and
    // resetting it would also reset the panel.

    // Core 0, above loop()'s priority: the chip's data is only valid for a
    // moment after each INT pulse, so the read can't wait for a frame to finish.
    xTaskCreatePinnedToCore(touch_task, "touch", 3072, nullptr, 3, &s_task, 0);

    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), on_touch_int, FALLING);
    return present;
}

bool touch_get(int16_t* x, int16_t* y) {
    portENTER_CRITICAL(&s_lock);
    *x = s_x;
    *y = s_y;
    bool down = s_down;
    portEXIT_CRITICAL(&s_lock);
    return down;
}
