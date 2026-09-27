// lv_mem_psram.h — where LVGL's heap comes from (see lv_conf.h).
//
// LVGL keeps every widget and style in its own heap. A full layout (12 pages
// of 24 tiles) needs several hundred KB, far more than internal SRAM can
// spare next to Wi-Fi and Bluetooth, so the heap lives in PSRAM instead.
#pragma once

#include <string.h>
#include <esp32-hal-psram.h>

static inline void* lv_psram_pool_alloc(size_t size) {
    void* pool = ps_malloc(size);
    if (pool) {
        memset(pool, 0, size);   // ps_malloc does not zero memory (hardware doc §P5)
    }
    return pool;
}
