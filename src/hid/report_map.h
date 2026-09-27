// report_map.h — the HID report descriptor: what the computer thinks we are.
//
// One device with two reports:
//   ID 1  keyboard: 8-byte input (modifiers, reserved, 6 keys) and a 1-byte
//         LED output (NumLock, CapsLock, …) that the computer writes to us.
//   ID 2  consumer control: one 16-bit media usage (Play/Pause, volume, …).
#pragma once

#include <stdint.h>

constexpr uint8_t REPORT_ID_KEYBOARD = 1;
constexpr uint8_t REPORT_ID_CONSUMER = 2;

static const uint8_t kReportMap[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,
    //   Modifier byte: 8 bits, Left Ctrl (0xE0) … Right GUI (0xE7)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    //   Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant)
    //   LED output: 5 bits (NumLock, CapsLock, ScrollLock, Compose, Kana) + 3 padding
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Constant)
    //   Six key slots. The range goes up to 0xFF: many examples stop at
    //   0x65, which silently drops F13–F24 (0x68–0x73).
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (255)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection

    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_CONSUMER,
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (0x3FF)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (0x3FF)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection
};
