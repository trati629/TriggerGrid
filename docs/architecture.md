# Architecture

TriggerGrid is a touchscreen macro pad. You tap a tile on the screen and the ESP32-S3 sends a
keystroke, a typed string or a media key to your computer over **Bluetooth LE HID**. To the
computer it is an ordinary Bluetooth keyboard, so no driver or companion app is needed.

The tiles are set up from a **web page served by the device**. The device joins your home Wi-Fi
and is reachable at `http://triggergrid.local`. If it has no saved network, or can't connect, it
starts its own access point instead.

```
 ┌──────── Computer ────────┐     ┌───────────── Browser (phone / laptop) ─────────────┐
 │ sees a Bluetooth keyboard│     │ does all the thinking:                              │
 └────────────▲─────────────┘     │  editor UI · drag & drop · validation               │
              │                   │  key names → HID codes · text → keystrokes          │
              │                   │  swatch names → RGB · compiles the "pad" section     │
              │                   └───────▲──────────────────────────────┬──────────────┘
              │ BLE HID reports           │ static files (gzip, cached)   │ config file as-is
              │                           │ tiny JSON status              │ (≤ 32 KB)
 ┌────────────┴───────────────────────────┴──────────────────────────────▼──────────────┐
 │ ESP32-S3: stores, streams and acts. Knows no key names, no colour names, no layouts.   │
 │                                                                                        │
 │  touch ─► ui (LVGL) ─tap─► action queue ─► hid (NimBLE) ─────────────────────────────► │
 │              ▲                                                                         │
 │              │ "config changed" flag                                                   │
 │   config: reads only the "pad" section ◄── LittleFS /config.json ◄── web task (core 0) │
 └────────────────────────────────────────────────────────────────────────────────────────┘
```

## Thin device, smart browser

The ESP32-S3 on this board has 8 MB of PSRAM, 16 MB of flash and two 240 MHz cores, so it isn't
tiny. But the scarce resources are **internal SRAM** (about 320 KB, shared by Wi-Fi, Bluetooth
and LVGL) and **time**. Every millisecond spent on a web request is a millisecond not spent
drawing the screen or sending a keystroke. So the web side follows one rule:

> **The device stores, streams and acts. The browser thinks.**

| Job | Browser | Device |
|-----|---------|--------|
| Draw the editor, preview, drag & drop | ✅ all of it | — |
| Validate the layout (grid fit, overlaps, lengths, key names) | ✅ full checks, with messages | Cheap bounds checks only, so bad data can't crash it |
| Key names (`CTRL`, `F13`) → HID usage codes | ✅ | — (receives numbers) |
| Typed text → keystrokes (the host's keyboard layout, plus its OS's Unicode input for é, €, emoji…) | ✅ | — (receives a keystroke list; holds modifiers across keystrokes and handles NumLock, see schema) |
| Swatch names and solid/soft style → final colours | ✅ | — (receives RGB values) |
| Icon names → icon font index | ✅ | — (receives an index) |
| Import / export / backup | ✅ (a file download or upload in the browser) | Streams the file as-is |
| Compress the web app | At build time (gzip, on your PC) | Serves `.gz` bytes as-is; never compresses |
| Build JSON responses | — | Only `/api/status`, about 200 bytes, with `snprintf` |
| Draw tiles, send HID reports | — | ✅ |

What this buys:

- **No lookup tables in firmware.** The device needs no key-name table, no character → key table,
  no swatch table and no colour maths. Supporting a new keyboard layout (for example UK or German), or a new
  Unicode input method, is a browser-only change.
- **No config parsing on the web path.** `GET /api/config` streams the file from flash in 1 KB
  chunks. The device parses only when the UI reloads, and then only the compact `pad` section.
- **Tiny, cacheable web app.** The browser downloads the app once, and later visits get a
  `304 Not Modified` of a few hundred bytes.
- **Less to test on hardware.** Most logic runs in the browser, where you can open it straight from disk and
  iterate without flashing (see *Developing the web app*).

The device still does basic checks on what it receives. Anyone on the network could send a
request, so the device checks that counts, sizes and grid positions are in range before using
them. It doesn't need to know what a key *means* to do that.

## Decisions

| Area            | Choice                                              | Why |
|-----------------|-----------------------------------------------------|-----|
| Toolchain       | PlatformIO in VS Code, pioarduino (Arduino core 3.x) | Needed by GFX Library ≥ 1.6. Arduino API is easy to follow in blog posts |
| Display driver  | Arduino_GFX `Arduino_AXS15231B`, type1 init          | Only known-good path for this panel (see hardware doc §P1) |
| UI              | LVGL 9, landscape 480×320, full-frame render + software transpose | Hardware and LVGL rotation are broken on this board; Solution 4 is confirmed working |
| Touch           | Own I2C driver for AXS15231B                         | No library handles the unlock command and multi-pulse INT |
| Bluetooth       | NimBLE-Arduino 2.x, own HID keyboard + consumer report map | Smaller and more reliable than Bluedroid; older `BleKeyboard` libraries break on core 3.x |
| Web server      | Arduino core `WebServer` (synchronous) in its **own task on core 0** | Ships with the core; one connection at a time keeps RAM predictable; runs off the UI core so page loads never stall the screen |
| Web app files   | Gzipped at build time and **embedded in the firmware** | One firmware upload (or OTA) updates the device and its web app together, so they can never be out of step. No LittleFS reads to serve a page |
| JSON            | ArduinoJson 7 with a filter, allocations in PSRAM    | The filter skips the `editor` section, so the device parses only `pad` |
| Config storage  | One file, `/config.json` on LittleFS, with an `editor` section and a compiled `pad` section | One file means one atomic save. `editor` is for people and the browser; `pad` is for the firmware |
| Live status     | Browser polls `GET /api/status` every 5 s while the tab is visible | Cheaper than WebSockets or SSE, which hold RAM per connection |
| Secrets         | Wi-Fi credentials and web PIN in NVS (`Preferences`) | Kept out of `config.json` so an exported layout never contains passwords |
| Web UI          | Plain HTML/CSS/JS in `web/`, no framework, no bundler, no CDN | Must work in AP mode with no internet; easy to read. The only build step is gzip, run automatically by PlatformIO |
| Discovery       | mDNS `triggergrid.local`                             | No need to look up an IP address |

## Modules

Each module is a folder under `src/` with a small header that exposes a C-style API
(`display_init()`, `hid_send_keys(...)`). Modules don't reach into each other's internals.

| Module     | Folder         | Responsibility |
|------------|----------------|----------------|
| `board`    | `include/board_pins.h` | Pin map and panel geometry. The only place with GPIO numbers |
| `display`  | `src/display/` | QSPI bus, AXS15231B init, backlight, `display_push_frame()`: every frame goes to the panel whole, from (0,0) (hardware doc §P9) |
| `touch`    | `src/touch/`   | I2C read with unlock command, INT debounce, portrait → landscape mapping |
| `lvgl_glue`| `src/lvgl_glue/` | LVGL display + input device registration, PSRAM buffers, the transposing flush, tick |
| `ui`       | `src/ui/`      | Status bar, pages, tiles, device menu. Draws from the `Pad` struct: colours arrive as final RGB values |
| `config`   | `src/config/`  | Reads the `pad` section of `config.json` into a `Pad` struct; bounds checks; built-in default |
| `actions`  | `src/actions/` | Runs a tile's pre-compiled action: one chord, one consumer key, or a keystroke list with a delay |
| `hid`      | `src/hid/`     | NimBLE HID device: keyboard + consumer reports, pairing, connection state |
| `net`      | `src/net/`     | Wi-Fi STA with AP fallback, mDNS, credentials in NVS |
| `web`      | `src/web/`     | Web task: serves the embedded app, streams `config.json`, small JSON API |

## Threads and ownership

LVGL is **not thread-safe**. The rule:

> Only the Arduino `loop()` task (core 1) calls LVGL. Every other task sends it a message.

| Task              | Core | Stack | Owns                                  | Talks to others via |
|-------------------|------|-------|---------------------------------------|---------------------|
| `loop()`          | 1    | 8 KB  | LVGL, touch, reading `config.json`    | Pushes actions onto `action_queue` |
| `web` task        | 0    | 8 KB  | `WebServer`, writing `config.json`    | Sets `config_dirty`; pushes "Try it" actions onto `action_queue` |
| `actions` task    | 0    | 4 KB  | Running action steps (with delays)    | Reads `action_queue`, calls `hid_*` |
| NimBLE host       | 0    | —     | BLE stack (created by NimBLE)         | Sets connection state atomically |
| Wi-Fi / lwIP      | 0    | —     | Created by the core                   | — |

- **Web requests never touch LVGL.** The web task writes the new file, then sets an atomic
  `config_dirty` flag. On its next pass, `loop()` re-reads the `pad` section and rebuilds the screens.
- **Only the web task writes `config.json`,** and it writes a temporary file first, then renames it
  (see *Saving*). The UI only reads the file after the rename, so it never sees a half-written file.
- Typing a long string takes time (one key down/up per character at 8–15 ms). This happens in
  the `actions` task so neither the UI nor the web page freezes.
- The web task sits at a lower priority than the Wi-Fi and BLE stacks, so a page load can't delay
  a keystroke.

## Web app delivery

The web app is `web/index.html`, `web/app.js`, `web/app.css` and `web/fonts/*.woff2`, about
60 KB compressed in total.

1. **Build:** a PlatformIO pre-build script (`tools/embed_web.py`) gzips each file in `web/`
   and writes `src/web/web_assets.h`: one `const uint8_t[]` per file (stored in flash, not RAM),
   plus its content type and a build hash. Fonts are already compressed, so they are embedded as-is.
2. **Serve:** the device sends the bytes straight from flash with `Content-Encoding: gzip`. It
   does no compression, no templating and no file-system access.
3. **Cache:** every response carries `ETag: "<build hash>"` and `Cache-Control: no-cache`. The
   browser re-checks on each visit and gets a `304` with no body until the firmware changes.
4. **Updates:** a new firmware (USB or OTA) brings the matching web app with it, so the page and
   the firmware always agree on the `pad` format.

Keep the request count low. The synchronous server handles one connection at a time, and each
request costs a TCP setup. Avoid lazy-loaded chunks, and inline icons as SVG in `app.js`.

### Developing the web app

You don't need the device for most web work. Open `web/index.html` straight from disk: when the
page is loaded from `file://`, `app.js` swaps its API calls for an in-browser **mock device**
that keeps state in memory, like `docs/mockups/web-config.html` does today. You iterate in the
browser, and flash only to test against real hardware.

## Configuration flow

`/config.json` has two sections (full format in [config-schema.md](config-schema.md)):

- **`editor`**: the human-readable layout: pages, tiles, labels, swatch *names*, key *names*,
  text. The browser edits this; the device never parses it.
- **`pad`**: the same layout **compiled by the browser** into what the firmware draws and sends:
  RGB colours, icon indexes, HID modifier bits and usage codes, keystroke lists.

```
 load:  browser ── GET /api/config ──► device streams the file (no parsing)
        browser edits the "editor" section

 save:  browser validates editor → compiles pad → PUT /api/config { editor, pad }
        device streams body to /config.tmp  (size ≤ 32 KB, else 413)
        device parses only "pad" from /config.tmp and checks bounds
          ├─ bad  → delete tmp, 400 { "error": "…" }, old file kept
          └─ good → rename tmp → /config.json, set config_dirty, 204
        loop() sees config_dirty → reloads Pad → rebuilds LVGL screens
```

- The browser always regenerates `pad` from `editor` before saving, including after an import,
  so the two sections never drift apart. `editor` is the source of truth.
- If the firmware finds a `pad.v` (format version) it doesn't understand, it keeps the built-in
  default layout and shows a warning. The embedded web app always matches its firmware, so
  opening the page and saving once fixes it.

### Device-side checks (cheap, no tables)

When reading `pad`, the device rejects the file if any of these fail:

- `pad.v` is a supported format version;
- 1–12 pages, 0–24 tiles per page;
- each tile's `x + w` and `y + h` fit the grid for `pad.density`, with `w` and `h` in 1–2;
- labels ≤ 24 bytes;
- a keystroke list is ≤ 4096 strokes, and the per-keystroke delay is 5–100 ms;
- modifier and usage values are single bytes (keyboard) or ≤ `0x3FF` (consumer).

Overlap checking stays in the browser. If two tiles overlap, the device just draws both, which
is harmless.

## Networking

- **STA mode:** credentials from NVS. Hostname `triggergrid` (configurable), mDNS
  `triggergrid.local`.
- **AP fallback:** SSID `TriggerGrid-XXXX` (XXXX = last 2 bytes of the MAC). The AP uses WPA2 with
  a random 8-character password generated on first boot and stored in NVS. The device menu shows
  the SSID, password and a **QR code** (LVGL `lv_qrcode`) so a phone can join by scanning.
- **Re-entering setup:** hold the status bar for 3 s → *Wi-Fi setup* forces AP mode until the next
  reboot.
- **Wi-Fi scan** is asynchronous (`WiFi.scanNetworks(true)`). `POST /api/wifi/scan` starts it and
  `GET /api/wifi/scan` returns the last result, so the browser polls instead of the device blocking
  for about 3 s.
- **Coexistence:** Wi-Fi and BLE share the 2.4 GHz radio on the ESP32-S3. The core handles this,
  but it can add latency to both. Milestone M8 includes a check that key presses stay responsive
  while the web page is saving.

## Web API

Responses are small. Only `/api/status` and `/api/wifi/scan` build JSON on the device. Every
`/api/*` route requires the PIN, if one is set.

| Method | Path              | Body / result | Device work |
|--------|-------------------|---------------|-------------|
| GET    | `/`, `/app.js`, … | Embedded gzip bytes, `ETag`, `304` when unchanged | Copy from flash |
| GET    | `/api/status`     | Firmware version, `pad.v`, uptime, Wi-Fi mode/IP/RSSI, BLE connected + host, free heap/PSRAM | `snprintf` ~200 bytes |
| GET    | `/api/config`     | `/config.json` as stored | Stream from LittleFS |
| PUT    | `/api/config`     | `{ "editor": …, "pad": … }` ≤ 32 KB. `204`, or `400 { "error" }` | Stream to tmp, parse `pad`, bounds check, rename |
| POST   | `/api/test`       | One compiled action, like a tile's `a` (see schema): runs it now without saving | Parse ≤ 4 KB, queue it |
| POST   | `/api/wifi/scan`  | Starts a scan | Start async scan |
| GET    | `/api/wifi/scan`  | `[{ "ssid", "rssi", "secure" }]` from the last scan | `snprintf` per network |
| POST   | `/api/wifi`       | `{ "ssid", "password" }`: saves to NVS, reboots into STA | NVS write |
| POST   | `/api/ble/forget` | Clears bonds so the pad can pair with another computer | — |
| POST   | `/api/pin`        | `{ "old", "new" }`: set or change the web PIN | NVS write |
| POST   | `/api/reboot`     | — | — |

Request limits, checked from `Content-Length` before reading the body: 32 KB for
`/api/config`, 4 KB for everything else. Anything larger gets a `413` without being read.

## Security

A device that types into your computer, controlled by a web page on your network, needs
some guarding:

- **Web PIN** (optional, recommended). When set, API calls need the header `X-TG-Pin`.
  The PIN is stored in NVS and can only be changed from the device menu or with the current PIN.
- **The AP always uses WPA2** with its own password. It is never open.
- **Bounds checks on the device** (above), so bad or malicious data can't overflow a buffer or crash
  the firmware. Meaning-level validation (key names, overlaps) is the browser's job. A forged
  request can make the pad type something, but a PIN-less LAN client could do that through the
  real page anyway. That is what the PIN is for.
- **BLE uses bonding** with "Just Works" pairing (the pad has no keyboard for a passkey). Use
  *Forget Bluetooth* to pair with a different computer.

## Memory budget (approximate)

| Item                                     | Where | Size     |
|------------------------------------------|-------|----------|
| LVGL draw buffer 480×320×2               | PSRAM | 300 KB   |
| Transpose buffer 320×480×2               | PSRAM | 300 KB   |
| LVGL heap (`LV_MEM_SIZE`)                | SRAM  | 96 KB    |
| NimBLE host                              | SRAM  | ~50 KB   |
| Wi-Fi + lwIP                             | SRAM  | ~60 KB   |
| Web task stack + 1 KB stream buffer      | SRAM  | 9 KB     |
| One TCP connection (lwIP buffers)        | SRAM  | ~6 KB    |
| Parsing `pad` (ArduinoJson, filtered)    | PSRAM | < 24 KB, freed after load |
| `Pad` struct (up to 12 × 24 tiles)       | PSRAM | < 40 KB  |
| Embedded web app                         | Flash | ~60 KB   |

The web path adds about 15 KB of SRAM while serving. The thing to watch is SRAM
fragmentation, not the total, which is why everything large goes in PSRAM or flash.

## Planned source layout

```
TriggerGrid/
├── platformio.ini            (extra_scripts = pre:tools/embed_web.py)
├── include/
│   ├── board_pins.h          pin map + geometry
│   └── lv_conf.h             LVGL config
├── src/
│   ├── main.cpp              boot sequence + loop
│   ├── display/              display.h/.cpp
│   ├── touch/                touch.h/.cpp
│   ├── lvgl_glue/            lvgl_glue.h/.cpp
│   ├── ui/                   theme.h (UI chrome colours only), status_bar, page_view, tile, menu
│   ├── config/               config.h/.cpp (Pad struct, pad reader), defaults.cpp
│   ├── actions/              actions.h/.cpp
│   ├── hid/                  hid.h/.cpp, report_map.h
│   ├── net/                  net.h/.cpp
│   └── web/                  web.h/.cpp, web_assets.h (generated, not committed)
├── web/                      the web app source: index.html, app.js, app.css, fonts/
│   └── lib/                  compile.js (editor → pad), keymap.js, layouts/, mock-device.js
├── data/                     LittleFS image: only the default config.json
├── tools/                    embed_web.py, make_mockup.py
├── test/                     native unit tests (pad reader bounds checks)
└── docs/
```

`web/lib/compile.js` also gets a small test page (`web/test.html`) that runs in the browser and
checks the compiler against known layouts, for example that `CTRL + C` compiles to modifier `0x01`
and usage `0x06`.

## Boot sequence

1. Serial, backlight off.
2. PSRAM check. Stop with a serial error if it's missing.
3. `display_init()` → `touch_init()` → `lvgl_glue_init()`; show the boot screen, then ramp up the backlight.
4. `config_load()`: read the `pad` section. If the file is missing, invalid or a newer format,
   use the built-in default layout and show a warning in the status bar.
5. `ui_build(pad)`.
6. `hid_init(pad.name)`, then start advertising (or reconnect to a bonded host).
7. `net_begin()`: try saved Wi-Fi for 10 s; on failure start AP `TriggerGrid-XXXX`.
8. `web_begin()`: start the web task and mDNS.
9. `loop()`: tick LVGL, run timers, apply `config_dirty`.
