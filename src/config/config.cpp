// config.cpp — see config.h. Flow: docs/architecture.md, "Configuration flow".

#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "pad_parse.h"

// ArduinoJson keeps the parsed document in PSRAM, not in scarce SRAM.
struct PsramAllocator : ArduinoJson::Allocator {
    void* allocate(size_t size) override { return ps_malloc(size); }
    void deallocate(void* p) override { free(p); }
    void* reallocate(void* p, size_t size) override { return ps_realloc(p, size); }
};
static PsramAllocator s_json_alloc;

static Pad* s_pad = nullptr;       // current layout
static Pad* s_old_pad = nullptr;   // previous one, freed at the next reload
static char s_warning[96];
static bool s_has_warning = false;

// Parsing uses shared buffers; the web task and loop() may both parse.
static SemaphoreHandle_t s_parse_lock = nullptr;
static volatile bool s_dirty = false;

// A zeroed Pad in PSRAM. It is about 22 KB, too big to spend SRAM on.
static Pad* alloc_pad() {
    Pad* pad = (Pad*)ps_malloc(sizeof(Pad));
    if (pad) {
        memset(pad, 0, sizeof(Pad));   // ps_malloc does not zero memory (§P5)
    }
    return pad;
}

static void free_pad(Pad* pad) {
    if (pad) {
        free(pad->stroke_pool);
        free(pad);
    }
}

// Read `path` and parse its pad section. On success *out is a new Pad (with
// its own keystroke pool); on failure *out is null and the problem is returned.
static const char* load_file(const char* path, Pad** out) {
    *out = nullptr;
    File file = LittleFS.open(path, "r");
    if (!file) {
        return "no file";
    }
    if (file.size() > CONFIG_MAX_BYTES) {
        file.close();
        return "file larger than 32 KB";
    }

    // Only the pad section is parsed; the filter skips the editor section.
    JsonDocument filter;
    filter["pad"] = true;
    JsonDocument doc(&s_json_alloc);
    DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();
    if (err) {
        return "not valid JSON";
    }

    JsonObjectConst pad_json = doc["pad"];
    const size_t stroke_count = pad_count_strokes(pad_json);
    uint16_t* pool = nullptr;
    if (stroke_count > 0) {
        pool = (uint16_t*)ps_malloc(stroke_count * sizeof(uint16_t));
        if (!pool) {
            return "out of memory";
        }
        memset(pool, 0, stroke_count * sizeof(uint16_t));
    }

    Pad* pad = alloc_pad();
    if (!pad) {
        free(pool);
        return "out of memory";
    }
    StrokeBuffer strokes = {pool, stroke_count, 0};
    const char* problem = pad_parse(pad_json, *pad, strokes);
    if (problem) {
        free(pool);
        free(pad);
        return problem;
    }
    pad->stroke_pool = pool;
    *out = pad;
    return nullptr;
}

// Replace the current layout. The old one is kept one more round, because
// the UI still points into it until it is rebuilt.
static void use_pad(Pad* pad) {
    free_pad(s_old_pad);
    s_old_pad = s_pad;
    s_pad = pad;
}

void config_reload() {
    Pad* pad = nullptr;
    xSemaphoreTake(s_parse_lock, portMAX_DELAY);
    const char* problem = load_file(CONFIG_PATH, &pad);
    if (problem) {
        // Copy while still holding the lock: `problem` may point into a
        // buffer the next parse overwrites.
        snprintf(s_warning, sizeof(s_warning), "%s", problem);
    }
    xSemaphoreGive(s_parse_lock);

    if (pad) {
        s_has_warning = false;
        use_pad(pad);
        Serial.printf("config: loaded %s (%u pages)\n", CONFIG_PATH, pad->page_count);
        return;
    }

    // No usable file: the built-in layout. A missing file is normal on a new
    // pad, so only a broken file earns a warning.
    s_has_warning = strcmp(s_warning, "no file") != 0;
    if (s_has_warning) {
        Serial.printf("config: %s not used: %s\n", CONFIG_PATH, s_warning);
    }
    Pad* fallback = alloc_pad();
    if (fallback) {
        pad_fill_default(*fallback);
        use_pad(fallback);
    }
}

bool config_init() {
    s_parse_lock = xSemaphoreCreateMutex();
    // Format on first boot, when the partition has never held a file system.
    if (!LittleFS.begin(true)) {
        Serial.println("config: LittleFS mount failed, using the built-in layout");
    }
    config_reload();
    return s_pad != nullptr;
}

const Pad& config_pad() {
    return *s_pad;
}

const char* config_warning() {
    return s_has_warning ? s_warning : nullptr;
}

const char* config_check_file(const char* path) {
    static char problem_copy[96];
    Pad* pad = nullptr;
    xSemaphoreTake(s_parse_lock, portMAX_DELAY);
    const char* problem = load_file(path, &pad);
    if (problem) {
        snprintf(problem_copy, sizeof(problem_copy), "%s", problem);
    }
    xSemaphoreGive(s_parse_lock);
    free_pad(pad);
    return problem ? problem_copy : nullptr;
}

void config_mark_dirty() {
    s_dirty = true;
}

bool config_take_dirty() {
    if (!s_dirty) {
        return false;
    }
    s_dirty = false;
    return true;
}
