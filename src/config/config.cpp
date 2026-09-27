// config.cpp — see config.h.

#include "config.h"

#include <Arduino.h>

static Pad* s_pad = nullptr;

// A zeroed Pad in PSRAM. It is about 22 KB, too big to spend SRAM on.
static Pad* alloc_pad() {
    Pad* pad = (Pad*)ps_malloc(sizeof(Pad));
    if (pad) {
        memset(pad, 0, sizeof(Pad));   // ps_malloc does not zero memory (§P5)
    }
    return pad;
}

bool config_init() {
    s_pad = alloc_pad();
    if (!s_pad) {
        return false;
    }
    pad_fill_default(*s_pad);
    return true;
}

const Pad& config_pad() {
    return *s_pad;
}
