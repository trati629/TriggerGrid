# Roadmap and Next Steps

The project is built in small milestones. Each one ends with something you can see or use on the
device, so each can be a blog post and a git tag (`m1-display`, `m2-lvgl`, …) for readers to
check out.

Status key: ✅ done · 🔜 next · ⬜ planned

---

## ✅ M0 — Skeleton builds

- `platformio.ini` for pioarduino, 16 MB flash, OPI PSRAM, LittleFS.
- `include/board_pins.h`, `include/lv_conf.h`, and a `src/main.cpp` that prints chip, flash and PSRAM info.
- **Done when:** `pio run` builds; after flashing, the serial monitor shows `PSRAM: 8192 KB (ok)`.

## 🔜 M1 — Display bring-up

- `src/display/`: QSPI bus, `Arduino_AXS15231B` with **`axs15231b_320480_type1_init_operations`**,
  RST not passed, rotation 0.
- Backlight on GPIO 1 with `analogWrite`, 1 s ramp at boot.
- Draw a test pattern: colour bars plus a rectangle 1 px in from each edge, to prove the geometry.
- **Done when:** the colour bars fill the panel and all four edges of the rectangle are visible.
- Watch for: blank screen or a dashed line means the wrong init sequence (hardware doc §P1).

## ⬜ M2 — LVGL in landscape

- `src/lvgl_glue/`: two 480×320×2 PSRAM buffers, both `memset` to 0 (§P5).
- `lv_display_create(480, 320)`, `ROTATION_0`, `RENDER_MODE_FULL`, transposing flush (§Solution 4).
- Tick with `lv_tick_inc()` in `loop()` (§P8).
- Show a label in each corner ("TL", "TR", "BL", "BR") and one in the centre.
- Log flush time to serial: transpose time + push time.
- **Done when:** the labels are in the right corners with the USB port where the enclosure puts it,
  and a full frame takes < 40 ms.

## ⬜ M3 — Touch

- `src/touch/`: I2C at 400 kHz, the 8-byte unlock + full STOP before every read, INT on GPIO 3 sets a
  flag, 80 ms release timeout.
- Map portrait to landscape (`lx = py`, `ly = 319 − px`) so it matches the flush transpose.
- Register as an LVGL pointer `indev`.
- Test screen: a dot follows your finger, and there's a button in each corner.
- **Done when:** all four corner buttons respond, and a single tap fires `LV_EVENT_CLICKED` exactly once.

## ⬜ M4 — Theme and bento grid (hard-coded)

- `src/ui/theme.h` with the tokens and swatch table from the [style guide](style-guide.md).
- Tile widget: spans, solid and soft styles, label and icon placement, press animation.
- Status bar, page container with horizontal snap, page dots.
- Two hard-coded pages at `regular` density, plus one at `compact`, to compare them on the real screen.
- **Done when:** swiping is smooth, the tiles match the style guide numbers, and every swatch is
  checked on the panel (RGB565).

## ⬜ M5 — Bluetooth keyboard

- `src/hid/`: NimBLE-Arduino 2.x HID device with a keyboard report and a consumer-control report,
  battery service (fixed 100%), bonding, and auto-reconnect.
- `src/actions/`: FreeRTOS queue + task on core 0; `keys` and `media` actions; key-name table.
- Wire the M4 tiles to real actions. The status bar shows BLE state.
- Test pairing with Windows, macOS, Linux, and at least one phone.
- **Done when:** a tile tap types Ctrl+C / plays or pauses media on each OS, and the pad reconnects
  after a power cycle without pairing again.
- Risk: BLE HID behaviour differs across OSes. Leave time for this milestone.

## ⬜ M6 — Config file

- `src/config/`: load `config.json` with ArduinoJson, validate (grid fit, overlaps, key names,
  lengths), and fall back to built-in defaults.
- `text` action: US-layout character → key table, per-character delay.
- UI builds its pages from `Config` instead of hard-coded data.
- Native unit tests in `test/` for validation and the character map (`pio test -e native`).
- **Done when:** editing `data/config.json` and running `uploadfs` changes the pad's layout, and an
  invalid file shows a status-bar warning instead of crashing.

## ⬜ M7 — Wi-Fi with AP fallback

- `src/net/`: STA from NVS credentials with a 10 s timeout, falling back to a WPA2 AP
  `TriggerGrid-XXXX`; mDNS `triggergrid.local`.
- Device menu (tap status bar): brightness, Wi-Fi info with QR code, Forget Bluetooth, About.
- **Done when:** a fresh device starts in AP mode, and a phone can join by scanning the QR code.

## ⬜ M8 — Web API

- `src/web/`: routes from [architecture.md](architecture.md#web-api), static files from LittleFS.
- Minimal `data/index.html`: status, Wi-Fi setup form, raw JSON editor for the config.
- `POST /api/test` to try an action without saving.
- **Done when:** you can set home Wi-Fi from the AP page, then reach the device at
  `triggergrid.local` and change a tile over the network.
- Check: typing with a tile while saving a config from the browser shows no noticeable lag
  (Wi-Fi and BLE share the radio).

## ⬜ M9 — Visual web editor

Design reference: [mockups/web-config.html](mockups/web-config.html) (see [mockups/README.md](mockups/README.md)).

- True-scale preview of each page using the same tokens (see style guide).
- Tap a tile to edit it: label, icon, swatch picker, style, span, action editor with keycap input
  ("press the combo" capture in the browser).
- Drag to move, add and delete tiles and pages, and a density switch with warnings for tiles that
  don't fit.
- Export and import `config.json`. Optional web PIN.
- **Done when:** someone who has never seen the JSON can build a page from a phone.

## ⬜ M10 — Polish

- Custom fonts (Space Grotesk) and an icon font through `lv_font_conv`.
- Idle dimming, then screen off; wake on touch.
- OTA firmware update from the web page.
- Release build, v1.0 tag, photos for the README.

---

## Later ideas

- `sequence` action (steps with delays) and `page` action (jump to a page).
- Keyboard layouts other than US for the `text` action.
- Mouse report (scroll or jog tile).
- Multiple bonded hosts with a switcher.
- Printed enclosure files.

## Open questions

- Is 64 px (compact) comfortable to hit, or should compact be 6×3? Decide in M4 on the real panel.
- Is full-frame render + transpose fast enough for smooth swiping? If not, consider partial render
  with per-area transpose in M4.
- Do all target OSes accept the consumer-control report with the keyboard in one HID device? M5.

## Next steps (right now)

1. Open the folder in VS Code with the PlatformIO extension installed (VS Code will suggest it).
2. Connect the board by USB-C and run **Upload and Monitor** for M0. Confirm PSRAM shows 8192 KB.
3. Start M1: create `src/display/display.h/.cpp` from the init code in
   [hardware/jc3248w535.md](hardware/jc3248w535.md) and call `display_init()` from `setup()`.
