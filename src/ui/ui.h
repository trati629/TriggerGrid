// ui.h — the pad's screen: status bar, swipeable pages of tiles, page dots.
//
// Only loop() may call these (LVGL is not thread-safe).
#pragma once

#include <lvgl.h>

#include "config/pad.h"

// State of a link shown as an icon in the status bar.
enum class LinkState {
    Off,       // not started / not available  (faint)
    Waiting,   // advertising, connecting, setup hotspot  (warn)
    Ok,        // connected  (ok)
    Failed,    // lost or failed  (error)
};

// Called when a tile is tapped. Return true if the action was sent, false if
// it couldn't be (for example Bluetooth isn't connected); the tile flashes
// green or red accordingly.
using TileTapHandler = bool (*)(const PadTile& tile);

// Build (or rebuild) every screen from `pad`. The Pad must stay alive until
// the next ui_build(): tiles point into it.
void ui_build(const Pad& pad);

void ui_on_tile_tap(TileTapHandler handler);

void ui_set_wifi_state(LinkState state);
void ui_set_ble_state(LinkState state);

// A short warning after the page name, e.g. "layout error". nullptr clears it.
void ui_set_warning(const char* text);

// The status bar, so the device menu can attach to it (M7).
lv_obj_t* ui_status_bar();
