# Roadmap and Next Steps

The project is built in small milestones. Each one ends with something you can see or use on the
device, so each can be a blog post and a git tag (`m1-display`, `m2-lvgl`, …) for readers to
check out.

Status key: ✅ done · 🧪 written and builds, not yet checked on the board · 🔜 next · ⬜ planned

---

## ✅ M0 — Skeleton builds

- `platformio.ini` for pioarduino, 16 MB flash, OPI PSRAM, LittleFS.
- `include/board_pins.h`, `include/lv_conf.h`, and a `src/main.cpp` that prints chip, flash and PSRAM info.
- **Done when:** `pio run` builds; after flashing, the serial monitor shows `PSRAM: 8192 KB (ok)`.

## ✅ M1 — Display bring-up

- `src/display/`: QSPI bus, `Arduino_AXS15231B` with **`axs15231b_320480_type1_init_operations`**,
  RST not passed, rotation 0.
- Backlight on GPIO 1 with `analogWrite`, 1 s ramp at boot.
- Draw a test pattern: colour bars plus a rectangle 1 px in from each edge, to prove the geometry.
- **Done when:** the colour bars fill the panel and all four edges of the rectangle are visible.
- Watch for: blank screen or a dashed line means the wrong init sequence (hardware doc §P1).
- Learned: drawing with `fillRect()` straight to the panel put every band at the top. The pattern is
  now drawn into a PSRAM frame and pushed whole with `display_push_frame()` (hardware doc §P9).

## 🧪 M2 — LVGL in landscape

- `src/lvgl_glue/`: two 480×320×2 PSRAM buffers, both `memset` to 0 (§P5).
- `lv_display_create(480, 320)`, `ROTATION_0`, `RENDER_MODE_FULL`, transposing flush (§Solution 4).
- Tick with `lv_tick_inc()` in `loop()` (§P8).
- Show a label in each corner ("TL", "TR", "BL", "BR") and one in the centre.
- Log flush time to serial: transpose time + push time.
- The rotation direction is `SCREEN_ROTATE_CW` in `board_pins.h`, shared with touch. If the corner
  labels are upside down, flip it.
- **Done when:** the labels are in the right corners with the USB port where the enclosure puts it,
  and a full frame takes < 40 ms.

## 🧪 M3 — Touch

- `src/touch/`: I2C at 400 kHz, the 8-byte unlock + full STOP before every read, INT on GPIO 3 sets a
  flag, 80 ms release timeout.
- Map portrait to landscape (`lx = py`, `ly = 319 − px`) so it matches the flush transpose.
- Register as an LVGL pointer `indev`.
- The chip is read in a small task on core 0, woken by INT, because its data goes stale within
  about 1 ms. While a finger is down the task also polls every 20 ms.
- Test screen: a dot follows your finger, and there's a button in each corner.
- **Done when:** all four corner buttons respond, and a single tap fires `LV_EVENT_CLICKED` exactly once.

## 🧪 M4 — Theme and bento grid (hard-coded)

- `src/ui/theme.h` with the interface tokens and grid numbers from the [style guide](style-guide.md).
  No swatch table: tiles get final RGB values, as they will from the compiled `pad`.
- `src/config/pad.h`: the `Pad` struct the UI draws from, and `defaults.cpp`, the built-in layout
  written as compiled values (the web editor's default layout).
- Tile widget: spans, solid and soft styles, label and icon placement, press animation.
- Status bar, page container with horizontal snap, page dots.
- LVGL's heap moved to PSRAM (1 MB, `include/lv_mem_psram.h`): a full 12-page layout would not fit
  in the 96 KB of SRAM it had.
- The built-in layout is `regular`. To compare `compact` on the panel, set `pad.density` in
  `defaults.cpp`.
- **Done when:** swiping is smooth, the tiles match the style guide numbers, and every swatch is
  checked on the panel (RGB565).

## 🧪 M5 — Bluetooth keyboard

- `src/hid/`: NimBLE-Arduino 2.x HID device with a keyboard report and a consumer-control report,
  battery service (fixed 100%), bonding, and auto-reconnect.
  - The keyboard report descriptor's usage range must reach at least `0x73`. Many examples stop at
    `0x65`, which silently drops F13–F24.
  - Include the LED **output** report, so the pad can read the computer's NumLock state.
- `src/actions/`: FreeRTOS queue + task on core 0 that runs the three compiled action types from
  the [config schema](config-schema.md#compiled-actions): key chord, consumer key and
  keystroke list. Keystroke lists follow the schema's *Keystroke rules* (modifiers held
  between same-modifier keystrokes, `0000` release, NumLock handling). The firmware works in HID
  codes only and has no key-name table.
- Wire the M4 tiles to real actions, with the codes written directly in the hard-coded layout
  (Ctrl+C = modifier `0x01`, usage `0x06`). The status bar shows BLE state.
- Test pairing with Windows, macOS, Linux, and at least one phone.
- Test Unicode typing with hard-coded keystroke lists for `é` and `👋` on each OS (see the method
  table in the schema). The mockup's compiler produces the lists: set *Computer* in Settings,
  type the text, and copy the `s` value from the *pad* pane.
- **Done when:** a tile tap types Ctrl+C / plays or pauses media on each OS; `é` types correctly
  on each OS and `👋` on macOS and Linux; and the pad reconnects after a power cycle without
  pairing again.
- Risk: BLE HID behaviour differs across OSes. Leave time for this milestone.
- Built: `src/hid/report_map.h` (keyboard with LED output, usage range to 0xFF, consumer 0-0x3FF),
  `src/hid/` (bonding, Just Works, advertises again on disconnect), `src/actions/` (queue + task on
  core 0; each job carries its own copy of the keystroke list). The built-in layout already uses real
  codes, so every tile is live.

## 🧪 M6 — Layout compiler and pad reader

This milestone has two halves. The browser half needs no hardware.

- **Browser (`web/lib/`):** `compile.js` turns the `editor` section into the `pad` section
  (swatches → RGB, icon names → indexes, key names → HID codes, text → keystroke hex using
  `keymap.js`, `layouts/us.js` and `unicode.js` for the per-OS Unicode input methods). Also
  `validate.js` for the grid rules.
- **Browser tests:** `web/test.html` runs in any browser and checks the compiler against known
  answers (Ctrl+C, `"Cheers,\nAlex"`, a soft tile's mixed colour, `é` and `👋` on each OS). It also has a *Download
  default config* button that writes `data/config.json`.
- **Firmware (`src/config/`):** read only the `pad` section with an ArduinoJson filter into a `Pad`
  struct in PSRAM, run the bounds checks, and fall back to the built-in default.
- UI builds its pages from `Pad` instead of hard-coded data.
- Native unit tests in `test/` for the bounds checks (`pio test -e native`).
- Built: `web/lib/` (swatches, icons, keymap, layouts/us, unicode, validate, compile,
  default-layout), `web/test.html` + `web/test.js` (29 known-answer tests, also run by
  `node tools/test_web.js`), `tools/make_default_config.js` (writes `data/config.json`),
  `src/config/pad_parse.cpp` (bounds checks, Arduino-free) and `config.cpp` (LittleFS, PSRAM JSON,
  fallback). `test/test_pad` (14 native tests) also checks that `data/config.json` matches the
  built-in layout in `defaults.cpp`.
- **Done when:** the compiler tests pass in the browser; flashing a compiled `data/config.json`
  with `uploadfs` changes the pad's layout; and a corrupted or oversized file shows a status-bar
  warning instead of crashing.

## ⬜ M7 — Wi-Fi with AP fallback

- `src/net/`: STA from NVS credentials with a 10 s timeout, falling back to a WPA2 AP
  `TriggerGrid-XXXX`; mDNS `triggergrid.local`.
- Device menu (tap status bar): brightness, Wi-Fi info with QR code, Forget Bluetooth, About.
- **Done when:** a fresh device starts in AP mode, and a phone can join by scanning the QR code.

## ⬜ M8 — Web server and API

- `tools/embed_web.py`: PlatformIO pre-build script that gzips `web/` into
  `src/web/web_assets.h` (see [architecture.md](architecture.md#web-app-delivery)).
- `src/web/`: web task on core 0, embedded files with `ETag` / `304`, the API routes from
  [architecture.md](architecture.md#web-api), request size limits, and the save flow (stream to
  a temporary file, check `pad`, rename).
- `web/`: first real page: status (polled every 5 s), Wi-Fi setup, and a raw JSON editor that
  compiles and saves. It also includes `mock-device.js`, so the page works from `file://` with no device.
- **Done when:** you can set home Wi-Fi from the AP page, then reach the device at
  `triggergrid.local` and change a tile over the network. A second visit to the page transfers
  only `304` responses.
- Measure: free SRAM before, during and after a page load and a save (log to serial). Typing
  with a tile while saving shows no noticeable lag (Wi-Fi and BLE share the radio).

## ⬜ M9 — Visual web editor

Design reference: [mockups/web-config.html](mockups/web-config.html) (see [mockups/README.md](mockups/README.md)).
All of this is browser code: no firmware changes expected.

- True-scale preview of each page using the same tokens (see style guide).
- Tap a tile to edit it: label, icon, swatch picker, style, span, action editor with keycap input
  ("press the combo" capture in the browser).
- Drag to move, add and delete tiles and pages, and a density switch with warnings for tiles that
  don't fit.
- Export and import `config.json` (the browser recompiles `pad` on import). Optional web PIN.
- **Done when:** someone who has never seen the JSON can build a page from a phone.

## ⬜ M10 — Polish

- Custom fonts (Space Grotesk) and an icon font through `lv_font_conv`.
- Idle dimming, then screen off; wake on touch.
- OTA firmware update from the web page.
- Release build, v1.0 tag, photos for the README.

---

## Later ideas

- `sequence` action (steps with delays) and `page` action (jump to a page).
- Keyboard layouts other than US for the `text` action (browser-only: add a file in `web/lib/layouts/`).
- Mouse report (scroll or jog tile).
- Multiple bonded hosts with a switcher.
- Printed enclosure files.

## Open questions

- Is 64 px (compact) comfortable to hit, or should compact be 6×3? Decide in M4 on the real panel.
- Is full-frame render + transpose fast enough for smooth swiping? If not, consider partial render
  with per-area transpose in M4.
- Do all target OSes accept the consumer-control report with the keyboard in one HID device? M5.

## Next steps (right now)

1. Note which case edge the white band of the M1 test pattern sits on, relative to the USB port.
   That fixes the transpose direction (90° CW or CCW) for M2.
2. Start M2: create `src/lvgl_glue/` with the two PSRAM buffers and the transposing flush from
   [hardware/jc3248w535.md](hardware/jc3248w535.md) §Solution 4, pushing through `display_push_frame()`.
