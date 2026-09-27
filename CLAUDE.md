# CLAUDE.md

Guidance for Claude Code (and humans) working in this repo.

## Project

TriggerGrid: a BLE HID macro pad on the Guition JC3248W535 (ESP32-S3, AXS15231B 320×480 QSPI panel
used in landscape, capacitive touch). It is configured from a web page served by the device. It
is also a teaching reference for a blog series, so **clarity beats cleverness**. Code should be
easy to quote in a blog post.

Read before working:
- `docs/hardware/jc3248w535.md`: board facts and known failures. Most display/touch "bugs"
  are already documented there.
- `docs/architecture.md`: modules, threading rule, API.
- `docs/style-guide.md`: all UI colours, sizes and fonts.
- `docs/roadmap.md`: current milestone. Stay within it.

## Build

```bash
pio run                    # build
pio run -t upload          # flash
pio device monitor         # serial, 115200
pio run -t uploadfs        # flash data/ to LittleFS
pio run -t clean           # after changing any shared header (hardware doc §P7)
```

Nothing can be flashed or observed from a cloud session. After hardware-facing changes, say what
the user should look for on the device and on serial.

## Hard rules (the board breaks otherwise)

- Platform is **pioarduino** (Arduino core 3.x). Don't switch to `espressif32`.
- `Arduino_AXS15231B` must get `axs15231b_320480_type1_init_operations`, `rotation = 0`, RST =
  `GFX_NOT_DEFINED`.
- Never use MADCTL rotation or `lv_display_set_rotation(90/270)`. Landscape = LVGL 480×320 at
  `ROTATION_0` + transpose in the flush callback.
- Only full-frame writes to the panel (`display_push_frame()`, LVGL `RENDER_MODE_FULL`). Partial
  GFX draws such as `fillRect` all land at the top (hardware doc §P9).
- `memset` every `ps_malloc` buffer.
- Drive LVGL ticks manually with `lv_tick_inc()` in `loop()`.
- Touch: 8-byte unlock with `endTransmission()` (full STOP) before every read.
- Core 3.x APIs: `analogWrite`/`ledcAttach` (no `ledcSetup`). BLE is NimBLE-Arduino 2.x, not
  the core's Bluedroid `BLEDevice`; use NimBLE's own types and callback signatures.
- `board_upload.flash_size = 16MB` stays in `platformio.ini` (the devkitc-1 board file says 8 MB).

## Code conventions

- GPIO numbers live only in `include/board_pins.h`. Colours live only in `src/ui/theme.h`.
- One folder per module under `src/`; a small `module.h` with `module_init()`-style functions.
  No module includes another module's `.cpp` internals.
- **Only `loop()` touches LVGL.** Other tasks communicate by queue or flag.
- No `delay()` in `loop()` after boot. Long work goes in the `actions` task.
- C++17, Arduino types are fine. Prefer `constexpr` over `#define` for constants.
- Comment the *why*, especially for workarounds, and link the hardware doc section (`// see hardware doc §P5`).
- Web UI in `data/`: plain HTML/CSS/JS, no framework, no build step, no CDN.

## Style (UI)

Dark only, neutral graphite interface; colour comes from user-chosen tile swatches. No sand,
beige, cream or ink/navy backgrounds, no gradients, glows or glass effects. Use the tokens in
the style guide and don't invent new colours.
