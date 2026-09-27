// pad_parse.h — the compiled `pad` section (JSON) → a Pad, with bounds checks.
//
// Pure C++ and ArduinoJson, no Arduino APIs, so the same code runs in the
// native unit tests (test/test_pad). The checks are the cheap ones from
// architecture.md, "Device-side checks": counts, sizes and ranges. The
// device never needs to know what a key or colour *means*.
#pragma once

#include <ArduinoJson.h>

#include "pad.h"

// Where keystroke lists go while parsing. The caller allocates `data` with
// room for pad_count_strokes() entries and keeps it alive with the Pad.
struct StrokeBuffer {
    uint16_t* data;
    size_t    capacity;
    size_t    used;
};

// Keystrokes needed by every "s" action in `pad` (4 hex digits each), so the
// caller can allocate the buffer once. Malformed lists count as 0.
size_t pad_count_strokes(JsonObjectConst pad);

// Fill `out` from `pad`. Returns nullptr on success, or a short message
// ("page 2 tile 5: w must be 1 or 2") describing the first problem found.
const char* pad_parse(JsonObjectConst pad, Pad& out, StrokeBuffer& strokes);

// One compiled action, e.g. {"t":"c","u":205}. Also used by POST /api/test.
const char* pad_parse_action(JsonObjectConst a, PadAction& out, StrokeBuffer& strokes);
