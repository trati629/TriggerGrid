// pad_parse.cpp — see pad_parse.h. Field names: docs/config-schema.md,
// "The pad section (compiled)".

#include "pad_parse.h"

#include <stdio.h>
#include <string.h>

namespace {

// Room for "page 12 tile 24: " plus the problem. Parsing runs only on
// loop() or the web task, one at a time, so one static buffer is enough.
char s_error[96];

const char* fail(int page, int tile, const char* what) {
    if (page < 0) {
        snprintf(s_error, sizeof(s_error), "%s", what);
    } else if (tile < 0) {
        snprintf(s_error, sizeof(s_error), "page %d: %s", page + 1, what);
    } else {
        snprintf(s_error, sizeof(s_error), "page %d tile %d: %s", page + 1, tile + 1, what);
    }
    return s_error;
}

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// `count` hex digits → value. False if any character isn't hex.
bool parse_hex(const char* s, int count, uint32_t& value) {
    value = 0;
    for (int i = 0; i < count; i++) {
        int d = hex_digit(s[i]);
        if (d < 0) {
            return false;
        }
        value = (value << 4) | (uint32_t)d;
    }
    return true;
}

// "RRGGBB" → 0xRRGGBB.
bool parse_rgb(JsonVariantConst v, uint32_t& rgb) {
    const char* s = v.as<const char*>();
    return s && strlen(s) == 6 && parse_hex(s, 6, rgb);
}

// A string that fits `max_bytes` (UTF-8 bytes, not characters).
bool copy_string(JsonVariantConst v, char* dest, size_t max_bytes, bool allow_empty) {
    const char* s = v.as<const char*>();
    if (!s) {
        return false;
    }
    size_t len = strlen(s);
    if (len > max_bytes || (len == 0 && !allow_empty)) {
        return false;
    }
    memcpy(dest, s, len + 1);
    return true;
}

bool in_range(JsonVariantConst v, long lo, long hi, long& out) {
    if (!v.is<long>()) {
        return false;
    }
    out = v.as<long>();
    return out >= lo && out <= hi;
}

}  // namespace

size_t pad_count_strokes(JsonObjectConst pad) {
    size_t total = 0;
    for (JsonObjectConst page : pad["pages"].as<JsonArrayConst>()) {
        for (JsonObjectConst tile : page["tiles"].as<JsonArrayConst>()) {
            JsonObjectConst a = tile["a"];
            const char* s = a["s"].as<const char*>();
            if (a["t"] == "s" && s) {
                total += strlen(s) / 4;
            }
        }
    }
    return total;
}

const char* pad_parse_action(JsonObjectConst a, PadAction& out, StrokeBuffer& strokes) {
    out = PadAction();
    long n;

    // A tile whose action the editor couldn't compile has "a": null. The
    // editor blocks saving it, but if one arrives, the tile simply does nothing.
    if (a.isNull()) {
        return nullptr;
    }

    const char* type = a["t"].as<const char*>();
    if (!type) {
        return "action has no type";
    }

    if (strcmp(type, "k") == 0) {
        if (!in_range(a["m"], 0, 0xFF, n)) return "key chord: bad modifier byte";
        out.modifiers = (uint8_t)n;
        JsonArrayConst keys = a["k"];
        if (keys.size() > PAD_MAX_CHORD_KEYS) return "key chord: more than 6 keys";
        for (JsonVariantConst key : keys) {
            if (!in_range(key, 0, 0xFF, n)) return "key chord: bad key code";
            out.keys[out.key_count++] = (uint8_t)n;
        }
        out.type = ActionType::Chord;
        return nullptr;
    }

    if (strcmp(type, "c") == 0) {
        if (!in_range(a["u"], 0, PAD_MAX_CONSUMER, n)) return "media key: bad usage code";
        out.usage = (uint16_t)n;
        out.type = ActionType::Consumer;
        return nullptr;
    }

    if (strcmp(type, "s") == 0) {
        if (!in_range(a["d"], 5, 100, n)) return "text: delay must be 5-100 ms";
        out.delay_ms = (uint8_t)n;
        out.numpad = a["np"] | false;

        const char* hex = a["s"].as<const char*>();
        if (!hex) return "text: no keystrokes";
        size_t len = strlen(hex);
        size_t count = len / 4;
        if (len % 4 != 0) return "text: keystroke list is not 4 hex digits each";
        if (count > PAD_MAX_STROKES) return "text: more than 4096 keystrokes";
        if (strokes.used + count > strokes.capacity) return "text: keystroke buffer full";

        uint16_t* dest = strokes.data + strokes.used;
        for (size_t i = 0; i < count; i++) {
            uint32_t v;
            if (!parse_hex(hex + i * 4, 4, v)) return "text: keystroke list is not hex";
            dest[i] = (uint16_t)v;
        }
        strokes.used += count;
        out.strokes = dest;
        out.stroke_count = (uint16_t)count;
        out.type = ActionType::Keystrokes;
        return nullptr;
    }

    return "unknown action type";
}

const char* pad_parse(JsonObjectConst pad, Pad& out, StrokeBuffer& strokes) {
    memset(&out, 0, sizeof(out));
    long n;

    if (!pad["v"].is<long>()) return fail(-1, -1, "no pad section");
    if (pad["v"].as<long>() != PAD_FORMAT_VERSION) return fail(-1, -1, "newer layout format");
    if (!copy_string(pad["name"], out.name, PAD_NAME_BYTES, false)) return fail(-1, -1, "bad device name");
    if (!in_range(pad["density"], 0, 1, n)) return fail(-1, -1, "bad density");
    out.density = (uint8_t)n;
    if (!in_range(pad["brightness"], 10, 255, n)) return fail(-1, -1, "brightness must be 10-255");
    out.brightness = (uint8_t)n;
    if (!in_range(pad["dim"], 0, 65535, n)) return fail(-1, -1, "bad dim time");
    out.dim_sec = (uint16_t)n;

    JsonArrayConst pages = pad["pages"];
    if (pages.size() < 1 || pages.size() > PAD_MAX_PAGES) return fail(-1, -1, "need 1-12 pages");

    const uint8_t cols = pad_grid_cols(out.density);
    const uint8_t rows = pad_grid_rows(out.density);

    int p = 0;
    for (JsonObjectConst page : pages) {
        PadPage& pg = out.pages[p];
        if (!copy_string(page["name"], pg.name, PAD_PAGE_NAME_BYTES, true)) return fail(p, -1, "bad page name");

        JsonArrayConst tiles = page["tiles"];
        if (tiles.size() > PAD_MAX_TILES) return fail(p, -1, "more than 24 tiles");

        int i = 0;
        for (JsonObjectConst tile : tiles) {
            PadTile& t = pg.tiles[i];
            long x, y, w, h;
            if (!in_range(tile["w"], 1, 2, w) || !in_range(tile["h"], 1, 2, h)) return fail(p, i, "w and h must be 1 or 2");
            if (!in_range(tile["x"], 0, cols - w, x) || !in_range(tile["y"], 0, rows - h, y)) return fail(p, i, "off the grid");
            t.x = (uint8_t)x; t.y = (uint8_t)y; t.w = (uint8_t)w; t.h = (uint8_t)h;

            if (!copy_string(tile["label"], t.label, PAD_LABEL_BYTES, true)) return fail(p, i, "label longer than 24 bytes");
            if (!in_range(tile["icon"], 0, 255, n)) return fail(p, i, "bad icon");
            t.icon = (uint8_t)n;
            if (!parse_rgb(tile["bg"], t.bg) || !parse_rgb(tile["fg"], t.fg) || !parse_rgb(tile["accent"], t.accent)) {
                return fail(p, i, "colours must be RRGGBB");
            }
            t.bar = tile["bar"] | false;

            const char* err = pad_parse_action(tile["a"], t.action, strokes);
            if (err) return fail(p, i, err);
            i++;
        }
        pg.tile_count = (uint8_t)i;
        p++;
    }
    out.page_count = (uint8_t)p;
    return nullptr;
}
