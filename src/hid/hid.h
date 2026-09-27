// hid.h — the pad as a Bluetooth LE keyboard (NimBLE-Arduino 2.x).
//
// The computer sees an ordinary keyboard with media keys. Reports are sent
// from the actions task; connection state is read from anywhere.
#pragma once

#include <stdint.h>

// Start Bluetooth, the HID services and advertising under `device_name`.
bool hid_init(const char* device_name);

// True when a computer is connected and the link is encrypted, so reports
// will be accepted.
bool hid_connected();

// The computer's NumLock state, from the LED report it sends us.
bool hid_numlock_on();

// Keyboard report: modifier bits (0x01 L-Ctrl … 0x80 R-GUI) plus up to six
// key usage codes. Pass count 0 to release every key but keep `modifiers`.
void hid_send_keys(uint8_t modifiers, const uint8_t* keys, uint8_t count);

// Release every key and modifier.
void hid_release_keys();

// Media key (HID consumer usage, e.g. 0xCD Play/Pause). 0 releases it.
void hid_send_consumer(uint16_t usage);

// Forget every paired computer and disconnect, so the pad can pair anew.
void hid_forget_bonds();
