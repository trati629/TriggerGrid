# Architecture

TriggerGrid is a touchscreen macro pad. You tap a tile on the screen and the ESP32-S3 sends a
keystroke, a typed string or a media key to your computer over **Bluetooth LE HID**. To the
computer it is an ordinary Bluetooth keyboard, so no driver or companion app is needed.

The tiles are set up from a **web page served by the device**. The device joins your home Wi-Fi
and is reachable at `http://triggergrid.local`. If it has no saved network, or can't connect, it
starts its own access point instead.

```
 ┌────────────── Computer ─────────────┐        ┌──── Phone / laptop browser ────┐
 │  sees a Bluetooth keyboard          │        │  http://triggergrid.local       │
 └────────────────▲────────────────────┘        └───────────────┬────────────────┘
                  │ BLE HID reports                             │ HTTP + JSON
 ┌────────────────┴─────────────────────────────────────────────▼────────────────┐
 │ ESP32-S3 (JC3248W535)                                                          │
 │                                                                                │
 │  touch ──► ui (LVGL) ──tap──► action queue ──► hid (NimBLE) ─────────────────► │
 │               ▲                                                                │
 │               │ "config changed"                                               │
 │            config (LittleFS /config.json) ◄── web (HTTP server) ◄── net (Wi-Fi)│
 └────────────────────────────────────────────────────────────────────────────────┘
```

## Decisions

| Area            | Choice                                              | Why |
|-----------------|-----------------------------------------------------|-----|
| Toolchain       | PlatformIO in VS Code, pioarduino (Arduino core 3.x) | Needed by GFX Library ≥ 1.6. Arduino API is easy to follow in blog posts |
| Display driver  | Arduino_GFX `Arduino_AXS15231B`, type1 init          | Only known-good path for this panel (see hardware doc §P1) |
| UI              | LVGL 9, landscape 480×320, full-frame render + software transpose | Hardware and LVGL rotation are broken on this board; Solution 4 is confirmed working |
| Touch           | Own I2C driver for AXS15231B                         | No library handles the unlock command and multi-pulse INT |
| Bluetooth       | NimBLE-Arduino 2.x, own HID keyboard + consumer report map | Smaller and more reliable than Bluedroid; older `BleKeyboard` libraries break on core 3.x |
| Web server      | Arduino core `WebServer` (synchronous)               | Ships with the core, no version-compatibility issues. Low traffic |
| JSON            | ArduinoJson 7                                        | Standard, well documented |
| Config storage  | `/config.json` on LittleFS                           | Human-readable; easy to back up and restore from the web page |
| Secrets         | Wi-Fi credentials and web PIN in NVS (`Preferences`) | Kept out of `config.json` so an exported layout never contains passwords |
| Web UI          | Plain HTML/CSS/JS in `data/`, no build step, no CDN  | Must work in AP mode with no internet; easy to read |
| Discovery       | mDNS `triggergrid.local`                             | No need to look up an IP address |

## Modules

Each module is a folder under `src/` with a small header that exposes a C-style API
(`display_init()`, `hid_send_keys(...)`). Modules don't reach into each other's internals.

| Module     | Folder         | Responsibility |
|------------|----------------|----------------|
| `board`    | `include/board_pins.h` | Pin map and panel geometry. The only place with GPIO numbers |
| `display`  | `src/display/` | QSPI bus, AXS15231B init, backlight, the transposing flush |
| `touch`    | `src/touch/`   | I2C read with unlock command, INT debounce, portrait → landscape mapping |
| `lvgl_glue`| `src/lvgl_glue/` | LVGL display + input device registration, PSRAM buffers, tick |
| `ui`       | `src/ui/`      | Theme tokens, status bar, pages, tiles, device menu. Only builds screens from a `Config` |
| `config`   | `src/config/`  | Load / validate / save `config.json`; defaults; schema versioning |
| `actions`  | `src/actions/` | Turns a tile's action into a sequence of HID steps; runs the queue |
| `hid`      | `src/hid/`     | NimBLE HID device: keyboard + consumer reports, pairing, connection state |
| `net`      | `src/net/`     | Wi-Fi STA with AP fallback, mDNS, credentials in NVS |
| `web`      | `src/web/`     | HTTP routes: static files from LittleFS, JSON API |

## Threads and ownership

LVGL is **not thread-safe**. The rule:

> Only the Arduino `loop()` task (core 1) calls LVGL. Every other task sends it a message.

| Task              | Core | Owns                                  | Talks to others via |
|-------------------|------|---------------------------------------|---------------------|
| `loop()`          | 1    | LVGL, touch, web `handleClient()`     | Pushes actions onto `action_queue` |
| `actions` task    | 0    | Running action steps (with delays)    | Reads `action_queue`, calls `hid_*` |
| NimBLE host       | 0    | BLE stack (created by NimBLE)         | Sets connection state atomically |
| Wi-Fi / lwIP      | 0    | Created by the core                   | — |

- Typing a long string takes time (one key down/up per character at around 8–15 ms). This
  happens in the `actions` task so the UI never freezes.
- When the web API saves a new config, it writes the file and sets a `config_dirty` flag. On
  the next `loop()` pass the UI rebuilds its screens from the new config.
- Web requests are handled in `loop()`. Handlers must return quickly: no `delay()` and no
  waiting on BLE.

## Boot sequence

1. Serial, backlight off.
2. PSRAM check. Stop with a serial error if it's missing.
3. `display_init()` → `touch_init()` → `lvgl_glue_init()`; show the boot screen, then ramp up the backlight.
4. `config_load()`. If the file is missing or invalid, use the built-in default layout and show a
   warning in the status bar.
5. `ui_build(config)`.
6. `hid_init(device_name)`, then start advertising (or reconnect to a bonded host).
7. `net_begin()`: try saved Wi-Fi for 10 s; on failure start AP `TriggerGrid-XXXX`.
8. `web_begin()` and mDNS.
9. `loop()`: tick LVGL, run timers, handle HTTP, apply `config_dirty`.

## Networking

- **STA mode:** credentials from NVS. Hostname `triggergrid` (configurable), mDNS
  `triggergrid.local`.
- **AP fallback:** SSID `TriggerGrid-XXXX` (XXXX = last 2 bytes of the MAC). The AP uses WPA2 with
  a random 8-character password generated on first boot and stored in NVS. The device menu shows
  the SSID, password and a **QR code** (LVGL `lv_qrcode`) so a phone can join by scanning.
- **Re-entering setup:** hold the status bar for 3 s → *Wi-Fi setup* forces AP mode until the next
  reboot.
- **Coexistence:** Wi-Fi and BLE share the 2.4 GHz radio on the ESP32-S3. The core handles this,
  but it can add latency to both. Milestone M5 includes a check that key presses stay responsive
  while the web page is saving.

## Web API

All responses are JSON. Every route except `GET /` and static files requires the PIN, if one is set.

| Method | Path              | Body / result |
|--------|-------------------|---------------|
| GET    | `/`               | `index.html` and assets from LittleFS |
| GET    | `/api/status`     | Firmware version, uptime, Wi-Fi mode/IP/RSSI, BLE connected + host, free heap/PSRAM |
| GET    | `/api/config`     | The current `config.json` |
| PUT    | `/api/config`     | New config. Validated → saved → UI rebuilds. `400` with a list of errors if invalid |
| POST   | `/api/test`       | `{ "action": {...} }`: runs an action now, without saving (the editor's "Try it" button) |
| GET    | `/api/wifi/scan`  | Nearby networks |
| POST   | `/api/wifi`       | `{ "ssid", "password" }`: saves to NVS, reboots into STA |
| POST   | `/api/ble/forget` | Clears bonds so the pad can pair with another computer |
| POST   | `/api/reboot`     | — |

The config format is in [config-schema.md](config-schema.md).

## Security

A device that types into your computer, controlled by a web page on your network, needs
some guarding:

- **Web PIN** (optional, recommended). When set, API calls need the header `X-TG-Pin`.
  The PIN is stored in NVS and can only be changed from the device menu or with the current PIN.
- **The AP always uses WPA2** with its own password. It is never open.
- **Actions are validated** on save: a length limit on typed text (1024 chars), and known key names only.
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
| Parsed config (ArduinoJson)              | PSRAM | < 32 KB  |

8 MB PSRAM and ~320 KB usable SRAM leave plenty of margin. The thing to watch is SRAM
fragmentation, not the total.

## Planned source layout

```
TriggerGrid/
├── platformio.ini
├── include/
│   ├── board_pins.h          pin map + geometry
│   └── lv_conf.h             LVGL config
├── src/
│   ├── main.cpp              boot sequence + loop
│   ├── display/              display.h/.cpp
│   ├── touch/                touch.h/.cpp
│   ├── lvgl_glue/            lvgl_glue.h/.cpp
│   ├── ui/                   theme.h, status_bar, page_view, tile, menu
│   ├── config/               config.h/.cpp, defaults.cpp
│   ├── actions/              actions.h/.cpp, keymap.cpp
│   ├── hid/                  hid.h/.cpp, report_map.h
│   ├── net/                  net.h/.cpp
│   └── web/                  web.h/.cpp
├── data/                     LittleFS image: web UI + default config
│   ├── index.html, app.js, app.css, fonts/
│   └── config.json
├── test/                     native unit tests (config validation, keymap)
└── docs/
```
