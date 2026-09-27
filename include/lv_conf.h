// lv_conf.h — LVGL 9 configuration for TriggerGrid.
// Only settings that differ from LVGL defaults are listed; everything else
// falls back to lv_conf_internal.h. Found via -DLV_CONF_INCLUDE_SIMPLE.

// Guard: PlatformIO also runs this through the assembler for startup stubs.
#ifndef __ASSEMBLER__

#if 1 /* Set to 0 to disable this file */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16            // RGB565, matches the panel

// Memory: LVGL's own heap. Frame buffers are allocated separately in PSRAM.
#define LV_MEM_SIZE (96U * 1024U)

// Ticks are driven manually from loop() with lv_tick_inc() — see
// hardware.md §P8. Do not rely on a custom tick source.

#define LV_DEF_REFR_PERIOD 16        // ~60 fps target; full-frame transpose caps it lower

// Fonts. Montserrat is the bootstrap font; custom fonts are added in src/ui/fonts/.
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_LOG 0

#endif // LV_CONF_H
#endif // #if 1
#endif // __ASSEMBLER__
