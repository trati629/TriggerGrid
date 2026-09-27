// power.h — backlight level and idle dimming.
//
// After `dim` idle seconds (from the layout) the backlight drops to 20%;
// after a further 5 minutes it turns off. Touch wakes it. A touch that wakes
// a dark screen is swallowed, so it can't press a tile you couldn't see.
#pragma once

#include <stdint.h>

// The normal ("awake") backlight level, 10–255. Applies at once if awake.
void power_set_level(uint8_t level);
uint8_t power_level();

// Seconds of no touch before dimming; 0 never dims.
void power_set_dim_after(uint16_t seconds);

// Call from loop(): dims or turns off the backlight when idle.
void power_update();

// True while the backlight is off (touch should only wake).
bool power_screen_off();

// Back to the normal level, and count this as activity.
void power_wake();

// One line on serial: state, idle time and the dim setting (for debugging).
void power_log_status();
