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
pio run -t uploadfs        # flash data/ (default config.json) to LittleFS
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
- `memset` every `ps_malloc` buffer.
- Drive LVGL ticks manually with `lv_tick_inc()` in `loop()`.
- Touch: 8-byte unlock with `endTransmission()` (full STOP) before every read.
- Core 3.x APIs: `analogWrite`/`ledcAttach` (no `ledcSetup`), BLE callbacks use `String`.

## Code conventions

- GPIO numbers live only in `include/board_pins.h`. UI chrome colours (background, status bar,
  menus) live only in `src/ui/theme.h`. Tile swatches live only in `web/lib/` and reach the
  firmware as compiled RGB values in `pad`.
- One folder per module under `src/`; a small `module.h` with `module_init()`-style functions.
  No module includes another module's `.cpp` internals.
- **Only `loop()` touches LVGL.** Other tasks communicate by queue or flag.
- No `delay()` in `loop()` after boot. Long work goes in the `actions` task.
- C++17, Arduino types are fine. Prefer `constexpr` over `#define` for constants.
- Comment the *why*, especially for workarounds, and link the hardware doc section (`// see hardware doc §P5`).
- **Thin device, smart browser** (architecture.md). The firmware stores, streams and acts. Key
  names, colour names, keyboard layouts, validation and anything that builds or parses the
  `editor` section belong in `web/`, never in `src/`. The firmware reads only the compiled `pad`
  section and does range checks on it.
- Web requests never touch LVGL, never block on BLE, and never compress or template on the device.
- Web UI in `web/`: plain HTML/CSS/JS, no framework, no bundler, no CDN. The only build step is
  the gzip-and-embed script PlatformIO runs.

## Style (UI)

Dark only, neutral graphite interface; colour comes from user-chosen tile swatches. No sand,
beige, cream or ink/navy backgrounds, no gradients, glows or glass effects. Use the tokens in
the style guide and don't invent new colours.
