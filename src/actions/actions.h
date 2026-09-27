// actions.h — runs a tile's compiled action on its own task (core 0).
//
// Typing a long text takes seconds, so it never runs in loop(): the UI and
// the web page stay responsive while the pad types.
#pragma once

#include "config/pad.h"

// Create the action queue and task.
bool actions_init();

// Queue `action` to run. It is copied, keystroke list included, so the layout
// can be reloaded while it runs. Returns false if Bluetooth isn't connected,
// the action is empty, or the queue is full.
bool actions_run(const PadAction& action);
