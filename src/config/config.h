// config.h — owns the current layout (a Pad) for the rest of the firmware.
#pragma once

#include "pad.h"

// Allocate the Pad in PSRAM and fill it with the built-in layout.
// Returns false if PSRAM is missing.
bool config_init();

// The layout the UI draws and the actions come from. Only loop() reads it.
const Pad& config_pad();

// Fill `pad` with the built-in layout (defaults.cpp).
void pad_fill_default(Pad& pad);
