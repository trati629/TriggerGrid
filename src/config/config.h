// config.h — owns the current layout (a Pad) for the rest of the firmware.
//
// The layout comes from the `pad` section of /config.json on LittleFS. If the
// file is missing or fails the checks, the built-in layout is used instead.
#pragma once

#include "pad.h"

constexpr const char* CONFIG_PATH     = "/config.json";
constexpr const char* CONFIG_TMP_PATH = "/config.tmp";
constexpr size_t      CONFIG_MAX_BYTES = 32 * 1024;

// Mount LittleFS and load the layout. Returns false only if PSRAM is missing.
bool config_init();

// The layout the UI draws and the actions come from. Only loop() reads it.
const Pad& config_pad();

// Why the file wasn't used (for the status bar and /api/status), or nullptr
// when the layout came from the file or no file exists yet.
const char* config_warning();

// Re-read /config.json. Call from loop() only, then rebuild the UI at once:
// the previous Pad stays valid until the next reload, not longer.
void config_reload();

// Check a candidate file with the same rules as config_reload(), without
// using it. Returns nullptr if it's good, else the problem. Any task.
const char* config_check_file(const char* path);

// Set by the web task after it saves a new file; loop() reloads.
void config_mark_dirty();
bool config_take_dirty();

// Fill `pad` with the built-in layout (defaults.cpp).
void pad_fill_default(Pad& pad);
