// menu.h — the device menu: brightness, Wi-Fi details with a QR code,
// Forget Bluetooth, setup hotspot and About.
//
// Tap the status bar to open it. Hold the status bar for 3 s to start the
// setup hotspot straight away.
#pragma once

#include <lvgl.h>

// Attach the menu gestures to the status bar. Call after every ui_build().
void menu_attach(lv_obj_t* status_bar);
