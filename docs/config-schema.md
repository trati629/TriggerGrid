# Config Schema — `config.json`

The layout, tiles and actions live in one file, `/config.json` on LittleFS. The web page reads it
with `GET /api/config` and saves it with `PUT /api/config`.

The file has two sections (why is explained in [architecture.md](architecture.md#thin-device-smart-browser)):

| Section  | Written by | Read by | Contents |
|----------|------------|---------|----------|
| `editor` | the browser (the editor) | the browser only | The layout as people think about it: names, swatches, key names, text |
| `pad`    | the browser, **compiled from `editor`** on every save | the firmware | The same layout as the firmware needs it: RGB colours, icon numbers, HID codes, keystroke lists |

`editor` is the source of truth. The browser regenerates `pad` from it before every save and after
every import, so you never edit `pad` by hand. The firmware ignores `editor` completely.

Wi-Fi credentials and the web PIN are **not** in this file (they live in NVS). That means an
exported layout is safe to share.

## Example

```json
{
  "version": 1,
  "editor": {
    "settings": {
      "deviceName": "TriggerGrid",
      "density": "regular",
      "brightness": 180,
      "dimAfterSec": 120,
      "hostLayout": "us",
      "hostOS": "windows"
    },
    "pages": [
      {
        "id": "edit",
        "name": "Editing",
        "tiles": [
          {
            "id": "copy",
            "x": 0, "y": 0, "w": 1, "h": 1,
            "label": "Copy",
            "icon": "copy",
            "color": "azure",
            "style": "solid",
            "action": { "type": "keys", "keys": ["CTRL", "C"] }
          },
          {
            "id": "sig",
            "x": 1, "y": 0, "w": 2, "h": 1,
            "label": "Email sig",
            "icon": "mail",
            "color": "violet",
            "style": "soft",
            "action": { "type": "text", "text": "Cheers,\nAlex" }
          },
          {
            "id": "play",
            "x": 3, "y": 0, "w": 2, "h": 2,
            "label": "Play / Pause",
            "icon": "playpause",
            "color": "volt",
            "action": { "type": "media", "key": "PLAY_PAUSE" }
          }
        ]
      }
    ]
  },
  "pad": {
    "v": 1,
    "name": "TriggerGrid",
    "density": 0,
    "brightness": 180,
    "dim": 120,
    "pages": [
      {
        "name": "Editing",
        "tiles": [
          { "x": 0, "y": 0, "w": 1, "h": 1, "label": "Copy", "icon": 1,
            "bg": "3D8BFF", "fg": "0E0F10", "accent": "0E0F10", "bar": false,
            "a": { "t": "k", "m": 1, "k": [6] } },
          { "x": 1, "y": 0, "w": 2, "h": 1, "label": "Email sig", "icon": 3,
            "bg": "2F2942", "fg": "F2F3F4", "accent": "9B6BFF", "bar": true,
            "a": { "t": "s", "d": 10, "s": "0206000B0008000800150016003600280204000F0008001B" } },
          { "x": 3, "y": 0, "w": 2, "h": 2, "label": "Play / Pause", "icon": 5,
            "bg": "C6F432", "fg": "0E0F10", "accent": "0E0F10", "bar": false,
            "a": { "t": "c", "u": 205 } }
        ]
      }
    ]
  }
}
```

## The `editor` section

### Root

| Field      | Type    | Required | Notes |
|------------|---------|----------|-------|
| `version`  | int     | yes      | File version, at the top level of the file. Currently `1`. The browser migrates older files when it loads them |
| `editor.settings` | object  | yes      | See below |
| `editor.pages`    | array   | yes      | 1–12 pages |

### `settings`

| Field         | Type   | Default         | Notes |
|---------------|--------|-----------------|-------|
| `deviceName`  | string | `"TriggerGrid"` | BLE name and mDNS host (lower-cased, spaces → `-`). 1–24 chars |
| `density`     | enum   | `"regular"`     | `regular` (5×3) or `compact` (6×4). See style guide |
| `brightness`  | int    | `180`           | 10–255 |
| `dimAfterSec` | int    | `120`           | Dim to 20% after this many idle seconds. `0` = never |
| `hostLayout`  | enum   | `"us"`          | Keyboard layout set on the **computer**. Characters on it are typed as normal keys. Only `us` for now; others (e.g. `uk`, `de`) would be browser-only additions |
| `hostOS`      | enum   | `"windows"`     | The computer's OS: `windows`, `mac`, `linux` or `other`. Picks the Unicode input method for characters not on the layout (see *Characters beyond the keyboard*) |

### Page

| Field   | Type   | Required | Notes |
|---------|--------|----------|-------|
| `id`    | string | yes      | Unique, `[a-z0-9-]`, ≤ 16 chars |
| `name`  | string | yes      | Shown in the status bar, ≤ 20 chars |
| `tiles` | array  | yes      | 0 to 24 tiles |

### Tile

| Field    | Type   | Required | Notes |
|----------|--------|----------|-------|
| `id`     | string | yes      | Unique within the page |
| `x`, `y` | int    | yes      | Top-left grid cell, 0-based |
| `w`, `h` | int    | yes      | Span: 1 or 2 each |
| `label`  | string | yes      | ≤ 24 chars, may be empty if `icon` is set |
| `icon`   | string | no       | Icon name from the icon table (starts as the `LV_SYMBOL_*` set) |
| `color`  | enum   | no       | Swatch name from the style guide. Default `graphite` |
| `style`  | enum   | no       | `solid` (default) or `soft` |
| `action` | object | yes      | See below |

**Grid rules** (checked by the browser while you edit and again before saving):
- The tile must fit inside the grid for the current `density` (5×3 or 6×4).
- Tiles must not overlap.
- If you switch from `compact` to `regular`, some tiles may no longer fit. The editor lists them
  and asks you to move them. The firmware never silently drops a tile.

## Actions

### `keys`: key combo

```json
{ "type": "keys", "keys": ["CTRL", "SHIFT", "M"] }
```

All listed keys are pressed together, then released together. Up to 4 modifiers and 6 other keys
(the HID boot-keyboard limit).

- Modifiers: `CTRL`, `SHIFT`, `ALT`, `GUI` (Win / Cmd), optionally with a side prefix: `LCTRL`, `RALT`.
- Letters and digits: `A`–`Z`, `0`–`9`
- Function keys: `F1`–`F24` (F13–F24 are useful for binding in other apps without clashes)
- Named keys: `ENTER`, `ESC`, `TAB`, `SPACE`, `BACKSPACE`, `DELETE`, `INSERT`, `HOME`, `END`,
  `PAGE_UP`, `PAGE_DOWN`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `PRINT_SCREEN`, `CAPS_LOCK`
- Punctuation: `MINUS`, `EQUAL`, `LBRACKET`, `RBRACKET`, `BACKSLASH`, `SEMICOLON`, `QUOTE`,
  `GRAVE`, `COMMA`, `PERIOD`, `SLASH`

Key names are **US layout physical keys**. HID sends key positions, not characters, so on a
different OS keyboard layout the same position can produce a different character.

### `text`: type a string

```json
{ "type": "text", "text": "hello@example.com", "charDelayMs": 10 }
```

- ≤ 1024 characters of any Unicode text, including accented letters, other scripts and emoji,
  plus `\n` (Enter) and `\t` (Tab). Other control characters are rejected.
- `charDelayMs` is optional (default 10, range 5–100). Increase it if the computer drops characters.
- The browser compiles the text into keystrokes. Characters on the computer's keyboard layout
  (`settings.hostLayout`) become normal key presses. Everything else is typed with the OS's
  Unicode input method (`settings.hostOS`), described next. The editor shows which characters use
  Unicode input and warns where the method has limits.

#### Characters beyond the keyboard

A Bluetooth keyboard sends key *positions*, never characters, so `é`, `€` or `👋` can only be
typed through the computer's own Unicode input. The browser compiles each such character into
that OS's key sequence:

| `hostOS`  | Method | Example: `é` (U+00E9) | Covers | One-time setup on the computer |
|-----------|--------|------------------------|--------|--------------------------------|
| `mac`     | Unicode Hex Input: hold Option, type 4 hex digits per UTF-16 unit | Option + `0 0 e 9` | Everything, including emoji (typed as a surrogate pair, 8 digits) | System Settings › Keyboard › Input Sources: add **Unicode Hex Input** and switch to it. It types normal text like US English |
| `linux`   | IBus: Ctrl+Shift+U, hex code point, Space | Ctrl+Shift+U `e 9` Space | Everything, including emoji | None on GNOME/Ubuntu (IBus is the default). Some terminals and non-IBus desktops ignore it |
| `windows` | Alt codes: hold Alt, numpad `0` + decimal code | Alt + numpad `0 2 3 3` | The Windows-1252 set: Western accented letters, `€ £ © ° – — “ ” …` | None |
| `windows` | Hex numpad: hold Alt, numpad `+`, hex digits | (used for `ő`, `ł`, `→`, emoji…) | Many apps, **not all**; emoji are the least reliable | Registry: `HKCU\Control Panel\Input Method`, string `EnableHexNumpad` = `1`, then sign out and in |
| `other`   | None (iPad, Android, …) | — | US-keyboard characters only; anything else is an error | — |

Windows Alt codes use the system's "ANSI" code page, which is Windows-1252 on Western-language
installs. On other installs the same codes can produce different characters.

### `media`: consumer control key

```json
{ "type": "media", "key": "VOLUME_UP" }
```

`PLAY_PAUSE`, `NEXT_TRACK`, `PREV_TRACK`, `STOP`, `MUTE`, `VOLUME_UP`, `VOLUME_DOWN`,
`BRIGHTNESS_UP`, `BRIGHTNESS_DOWN`. Support varies by OS; brightness in particular is often
ignored on desktops.

### Reserved for later

`sequence` (several actions with delays) and `page` (jump to another page) are planned; see the
roadmap. The editor won't save a `type` it doesn't know, and the compiler has no output for one.

## The `pad` section (compiled)

This is everything the firmware reads. The browser writes it; you never edit it by hand. It uses
short keys and numbers so the firmware needs no lookup tables. All it does is range checks
(see [architecture.md](architecture.md#device-side-checks-cheap-no-tables)).

### Root

| Field        | Type   | Notes |
|--------------|--------|-------|
| `v`          | int    | `pad` format version, currently `1`. Firmware that doesn't support it falls back to its built-in default layout |
| `name`       | string | BLE name and mDNS host, 1–24 bytes |
| `density`    | int    | `0` = regular 5×3, `1` = compact 6×4 |
| `brightness` | int    | 10–255 |
| `dim`        | int    | Seconds before dimming, `0` = never |
| `pages`      | array  | 1–12 pages, each `{ "name", "tiles" }` |

### Tile

| Field    | Type   | Notes |
|----------|--------|-------|
| `x`, `y`, `w`, `h` | int | Grid cell and span, same as `editor` |
| `label`  | string | ≤ 24 bytes, UTF-8 |
| `icon`   | int    | Index into the device's icon font. `0` = no icon. The order comes from `web/lib/icons.js`, which the icon font is also built from |
| `bg`     | hex    | Tile fill, `RRGGBB`. For `soft` tiles the browser has already mixed the swatch into `surface-1` |
| `fg`     | hex    | Label colour |
| `accent` | hex    | Icon colour, and the colour of the bottom bar when `bar` is true |
| `bar`    | bool   | Draw the 3 px bottom bar (`soft` style) |
| `a`      | object | The compiled action, below |

The firmware converts each colour with `lv_color_hex(strtoul(s, nullptr, 16))`. It has no swatch
table and does no colour maths.

### Compiled actions

| `t`   | Meaning | Fields | Firmware does |
|-------|---------|--------|---------------|
| `"k"` | Key chord | `m`: modifier byte; `k`: up to 6 HID keyboard usage codes | Press all, release all |
| `"c"` | Consumer (media) key | `u`: HID consumer usage (e.g. `205` = `0xCD` Play/Pause) | Press, release |
| `"s"` | Keystroke list (typed text) | `d`: delay per keystroke in ms (5–100); `s`: hex string, 4 hex digits per keystroke = modifier byte + usage code; `np` (optional): `true` if the list uses numpad keys | See *Keystroke rules* below |

Modifier byte bits (standard HID): `0x01` L-Ctrl, `0x02` L-Shift, `0x04` L-Alt, `0x08` L-GUI,
`0x10` R-Ctrl, `0x20` R-Shift, `0x40` R-Alt (AltGr), `0x80` R-GUI.

Worked example: `"Cheers,\nAlex"` on a US layout compiles to
`0206 000B 0008 0008 0015 0016 0036 0028 0204 000F 0008 001B` (spaces added here for reading):
`C` is Shift (`02`) + usage `06`, `,` is `36`, the newline is Enter (`28`), and so on.
The hex string is up to 4096 keystrokes (16384 hex digits). Unicode characters take several
keystrokes each (an emoji is 7–9), and the whole file must still fit in 32 KB.

#### Keystroke rules

The firmware plays a keystroke list like this. These rules are all it needs for normal typing and
for every Unicode method above:

1. For each keystroke `MMKK`: send the report with modifiers `MM` and key `KK`, wait `d` ms, then
   send modifiers `MM` with no key (the key is released and the modifiers **stay held**).
2. If the next keystroke has a **different** modifier byte, first release everything, then wait `d`.
3. `0000` means "release everything". The compiler puts it after each Alt-code or Option
   sequence, so two sequences in a row don't merge into one.
4. At the end of the list, release everything.
5. If `np` is `true`: before typing, check the NumLock state the computer reports (the HID LED
   output report). If NumLock is off, tap NumLock (`0x53`) first and tap it again at the end to
   restore it.

Example, Windows, `é` then `!`: `0462 045A 045B 045B 0000 021E`. That is Alt held while typing numpad
`0 2 3 3`, then release, then Shift + `1`.

`POST /api/test` takes one of these action objects on its own, e.g. `{ "t": "c", "u": 205 }`.
