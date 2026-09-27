# Config Schema — `config.json`

The layout, tiles and actions live in `/config.json` on LittleFS. The web editor reads and writes
this file through `GET`/`PUT /api/config`. You can also edit it by hand and flash it with
`pio run -t uploadfs`.

Wi-Fi credentials and the web PIN are **not** in this file (they live in NVS). That means an
exported layout is safe to share.

## Example

```json
{
  "version": 1,
  "settings": {
    "deviceName": "TriggerGrid",
    "density": "regular",
    "brightness": 180,
    "dimAfterSec": 120
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
          "color": "violet",
          "style": "soft",
          "action": { "type": "text", "text": "Cheers,\nAlex" }
        },
        {
          "id": "play",
          "x": 3, "y": 0, "w": 2, "h": 2,
          "label": "Play / Pause",
          "icon": "play",
          "color": "volt",
          "action": { "type": "media", "key": "PLAY_PAUSE" }
        }
      ]
    }
  ]
}
```

## Fields

### Root

| Field      | Type    | Required | Notes |
|------------|---------|----------|-------|
| `version`  | int     | yes      | Schema version. Currently `1`. Firmware migrates older versions on load |
| `settings` | object  | yes      | See below |
| `pages`    | array   | yes      | 1–12 pages |

### `settings`

| Field         | Type   | Default         | Notes |
|---------------|--------|-----------------|-------|
| `deviceName`  | string | `"TriggerGrid"` | BLE name and mDNS host (lower-cased, spaces → `-`). 1–24 chars |
| `density`     | enum   | `"regular"`     | `regular` (5×3) or `compact` (6×4). See style guide |
| `brightness`  | int    | `180`           | 10–255 |
| `dimAfterSec` | int    | `120`           | Dim to 20% after this many idle seconds. `0` = never |

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

**Grid rules** (checked on save; errors are returned to the editor):
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

- ≤ 1024 characters. Printable ASCII, `\n` (Enter) and `\t` (Tab).
- `charDelayMs` is optional (default 10, range 5–100). Increase it if the computer drops characters.
- Characters are mapped to keys assuming the computer uses a **US keyboard layout**. Other
  layouts are a roadmap item.

### `media`: consumer control key

```json
{ "type": "media", "key": "VOLUME_UP" }
```

`PLAY_PAUSE`, `NEXT_TRACK`, `PREV_TRACK`, `STOP`, `MUTE`, `VOLUME_UP`, `VOLUME_DOWN`,
`BRIGHTNESS_UP`, `BRIGHTNESS_DOWN`. Support varies by OS; brightness in particular is often
ignored on desktops.

### Reserved for later

`sequence` (several actions with delays) and `page` (jump to another page) are planned; see the
roadmap. Firmware for schema version 1 rejects unknown `type` values with a clear error, rather
than ignoring them.
