# TriggerGrid Style Guide

This guide covers the on-device UI (LVGL, 480×320 landscape) and the web config page. Both follow
the same rules so a button looks the same in the editor as it does on the pad.

## Principles

1. **The buttons carry the colour.** The interface itself is neutral graphite. Colour comes from
   what the user assigns to each tile, so a page reads as *their* layout and not as a theme.
2. **Dense but hittable.** Fit as many actions as possible, but never make a tile smaller than
   about 9 mm (64 px on this panel).
3. **Rounded bento tiles.** Tiles have rounded corners, sit on a strict grid with even gutters, and
   come in a few sizes so important actions can be bigger.
4. **One glance, one action.** A label is 1–2 words; an icon is optional. The pad is not the place
   for descriptions. Those go in the web editor.
5. **Dark only.** A single dark theme, tuned for a screen sitting next to a monitor.

### What to avoid

The goal is to avoid the look of stock AI-generated UIs:

- **No sand, cream, beige or parchment.** No warm off-white backgrounds or "paper" textures.
- **No ink.** No deep navy or blue-black backgrounds (`#0B1020`-type colours), and no
  indigo-to-purple gradients.
- No glassmorphism, background blur, glow halos or soft-drop-shadow stacks.
- No gradient fills on tiles. Tiles use flat colour; depth comes from surface steps, not shadows.
- No emoji as icons.

## Colour

### UI tokens

The base is a **neutral graphite** with no blue or warm tint. Names are the same in C and CSS.

| Token          | Hex       | Use                                                    |
|----------------|-----------|--------------------------------------------------------|
| `bg`           | `#101112` | Screen / page background                               |
| `surface-1`    | `#1A1C1E` | Empty grid cells, cards, input fields                  |
| `surface-2`    | `#24272A` | Raised panels: sheets, menus, the status bar           |
| `surface-3`    | `#2E3236` | Hover (web), pressed neutral controls                  |
| `line`         | `#383C41` | 1 px borders and dividers                              |
| `text`         | `#F2F3F4` | Primary text                                           |
| `text-muted`   | `#A0A6AC` | Secondary text, captions                               |
| `text-faint`   | `#62696F` | Disabled text, placeholder, inactive page dots         |
| `on-bright`    | `#0E0F10` | Label colour on light tile colours                     |
| `focus`        | `#F2F3F4` | Focus ring / selection outline (white, 2 px)           |

Status colours are the only fixed accents in the interface, and they only show state:

| Token          | Hex       | Meaning                                                 |
|----------------|-----------|---------------------------------------------------------|
| `ok`           | `#3DDC84` | BLE connected, Wi-Fi connected, saved                   |
| `warn`         | `#FFB224` | Advertising / pairing, AP fallback active               |
| `error`        | `#FF4D4F` | Disconnected, save failed, invalid config               |
| `link`         | `#5AA9FF` | Web page links only                                     |

### Tile palette

Users choose a tile colour from a fixed set of named swatches. Using named swatches instead of
free hex values keeps every layout legible and survives RGB565 quantisation. The config file stores
the **name**, not the hex value.

| Name        | Hex       | Label colour | Notes                       |
|-------------|-----------|--------------|-----------------------------|
| `graphite`  | `#2C3034` | `text`       | Default. Neutral tile       |
| `steel`     | `#8A96A3` | `on-bright`  | Quiet, secondary actions    |
| `volt`      | `#C6F432` | `on-bright`  | Electric lime               |
| `mint`      | `#3EE6A8` | `on-bright`  |                             |
| `aqua`      | `#2BD4E6` | `on-bright`  |                             |
| `azure`     | `#3D8BFF` | `on-bright`  |                             |
| `violet`    | `#9B6BFF` | `on-bright`  |                             |
| `magenta`   | `#F24FD1` | `on-bright`  |                             |
| `coral`     | `#FF5C5C` | `on-bright`  |                             |
| `tangerine` | `#FF8A1F` | `on-bright`  |                             |
| `sun`       | `#FFD60A` | `on-bright`  |                             |
| `crimson`   | `#C8233B` | `text`       | Use for destructive actions |

Each swatch has been checked for at least 4.5:1 contrast with its label colour.

### Tile styles

Each tile has a `style` setting as well as a colour:

- **`solid`** (default): the tile is filled with the swatch colour, and the label uses the
  swatch's label colour.
- **`soft`**: the tile is filled with `surface-1` mixed with 16% of the swatch colour. The label is
  `text`, and the icon and a 3 px bar along the bottom edge are in the full swatch colour. Use it
  for a calmer page where only some tiles are solid.

`soft` fill in LVGL: `lv_color_mix(swatch, surface_1, 41)` (41/255 ≈ 16%).
In CSS: `color-mix(in srgb, var(--swatch) 16%, var(--surface-1))`.

### RGB565 note

The panel is 16-bit colour, so each channel loses its low bits: red and blue keep 5 bits, green
keeps 6. Colours close together in the hex table can become the same colour on the device. Do not
add a swatch without viewing it on real hardware. Use `lv_color_hex(0xRRGGBB)`; LVGL converts it.

## Layout (device)

The logical screen is **480 × 320** landscape. All numbers are logical pixels. The panel is about
180 ppi, so 1 mm ≈ 7 px.

```
 0                                                     480
 ┌──────────────────────────────────────────────────────┐  0
 │ Editing                                 wifi  ble    │  status bar, 22 px
 ├──────────────────────────────────────────────────────┤  22
 │  ┌──────┐  ┌──────┐  ┌─────────────┐  ┌──────┐       │
 │  │ 1×1  │  │ 1×1  │  │     2×1     │  │ 1×1  │       │
 │  └──────┘  └──────┘  └─────────────┘  └──────┘       │
 │  ┌──────┐  ┌─────────────┐  ┌──────┐  ┌──────┐       │  tile grid, 288 px
 │  │ 1×2  │  │             │  │ 1×1  │  │ 1×1  │       │  (one page;
 │  │      │  │     2×2     │  └──────┘  └──────┘       │   swipe ← → for
 │  │      │  │             │  ┌──────┐  ┌──────┐       │   more pages)
 │  └──────┘  └─────────────┘  └──────┘  └──────┘       │
 ├──────────────────────────────────────────────────────┤  310
 │                        ● ○ ○                         │  page dots, 10 px
 └──────────────────────────────────────────────────────┘  320
```
(Sketch not to scale. The `regular` grid is 5 columns; exact numbers are below.)

### Grid densities

Density is a global setting.

| Density   | Grid  | Tile (1×1) | Gutter | Side pad | Top/bottom pad | Tiles/page |
|-----------|-------|------------|--------|----------|----------------|------------|
| `regular` | 5 × 3 | 88 × 88    | 6      | 8        | 6              | 15         |
| `compact` | 6 × 4 | 73 × 64    | 6      | 6        | 7              | 24         |

Check: `regular` → 8 + 5·88 + 4·6 + 8 = 480 wide, 6 + 3·88 + 2·6 + 6 = 288 tall.
`compact` → 6 + 6·73 + 5·6 + 6 = 480 wide, 7 + 4·64 + 3·6 + 7 = 288 tall.

### Tile spans

Tiles occupy whole grid cells: **1×1, 2×1, 1×2, 2×2**. Width of a span of *n* cells is
`n·tile + (n−1)·gutter`. For example, a `regular` 2×1 tile is 182 × 88.

The corner radius is **14 px** (`regular`) or **12 px** (`compact`) on all spans. Don't scale
the radius with tile size; equal corners are what make the grid read as bento.

### Pages and scrolling

- Each page is exactly one screen of grid. You swipe horizontally between pages, and pages snap
  into place. There is no vertical scrolling.
- Use an `lv_tileview` or a horizontal scroll container with `LV_SCROLL_SNAP_CENTER` and one child
  per page. Turn off elastic overscroll on the last page.
- Page dots: 6 px circles, 8 px apart, centred. The current page is `text` and the others are
  `text-faint`. When there is a single page, hide the dots.
- The page name is shown in the status bar, not on the page.

### Status bar (22 px, `surface-2`)

From left to right: page name (`text`, 12 px), then a spacer, then Wi-Fi state, BLE state and
the host name if connected. The state icons use the status colours. Tapping the status bar opens
the device menu (brightness, Wi-Fi info with QR code, re-pair Bluetooth, about).

## Typography

| Where  | Role            | Font                          | Size / weight |
|--------|-----------------|-------------------------------|---------------|
| Device | Tile label      | Space Grotesk¹ (Montserrat until converted) | 16 medium (`regular`), 14 (`compact`) |
| Device | Status / dots   | Space Grotesk¹                | 12 regular    |
| Device | Menu titles     | Space Grotesk¹                | 20 medium     |
| Web    | UI text         | Space Grotesk                 | 14–15 / 400–500 |
| Web    | Key combos      | JetBrains Mono, in keycaps    | 13 / 500      |

¹ Convert with `lv_font_conv` (or the LVGL online font converter) at 4 bpp, and include only
the glyphs you need: ASCII plus `LV_SYMBOL_*`. Until then the built-in Montserrat is used
(enabled in `include/lv_conf.h`).

Label rules: up to 2 lines. Labels are left-aligned at the tile's top-left with 10 px padding, and
the icon sits at the bottom-left. A 1×1 `compact` tile shows either the icon or the label, not
both. Text that doesn't fit is truncated with `…`, never scaled down.

## Motion and feedback

- **Press:** the tile scales to 96% and darkens by 20% (`lv_color_darken(c, 51)`) for 80 ms.
- **Action sent:** a 150 ms outline flash in `ok`. If BLE is not connected, the flash is in
  `error` and the status bar BLE icon pulses once.
- **Page change:** the default LVGL scroll snap animation. Nothing custom.
- Don't use idle animations. The pad sits in your peripheral vision, so it should stay still.

## Icons

- Device: start with the built-in `LV_SYMBOL_*` set (play, pause, volume, copy, etc.). Later,
  a small custom icon font made from one open set (e.g. Lucide or Tabler), converted like the text font.
- Icons are 20 px (`regular`) or 18 px (`compact`) and take the label colour.
- The web editor uses the same icon set as SVG, so the preview matches the device.

## Web config page

The web page applies the same rules to a browser layout.

- Background `bg`. The editor shows a **true-scale preview** of the current page (480×320 at 1×,
  or scaled to fit on phones), using the same tile sizes, radii and colours.
- Tap a tile in the preview to edit it in a side sheet (a bottom sheet on phones) on `surface-2`:
  label, icon, colour swatch picker, style, span, action.
- **Drag and drop** places tiles on the preview. It uses pointer events so mouse and touch both work:
  - **Add:** drag a blank tile or a preset from the *Add tiles* tray (below the preview) onto the screen.
  - **Move:** drag a tile. A press that moves less than 5 px is a tap and opens the editor instead.
  - **Resize:** drag the corner handle of the selected tile: a 20 px `text` square with a
    2 px `bg` border. Spans stay within 1–2 cells.
  - **Delete:** drag a tile back onto the tray. While a tile is dragged the tray turns into a dashed
    "Drop here to delete" zone, which goes `error` when the pointer is over it.
  - While dragging, the tile follows the pointer at 85% opacity with a 2 px `text` outline, and a
    dashed outline snaps to the grid cell it would land in. Both turn `error` when the spot is
    taken or off the grid. Dropping there does nothing and shows why.
  - Keyboard: arrow keys move the focused tile one cell.
- The key combo editor shows keys as keycaps: `surface-3` fill, 1 px `line` border, 6 px
  radius, JetBrains Mono.
- Buttons: the primary action (Save to device) uses a `text` background with `on-bright` text.
  The primary button is neutral because the tiles carry the colour. Secondary buttons have a 1 px
  `line` outline.
- Radius: 12 px for panels, 8 px for inputs and buttons.
- Spacing scale: 4, 8, 12, 16, 24, 32.
- The page is plain HTML/CSS/JS in `web/`, with no framework and no CDN (it must work in AP mode
  with no internet). It is gzipped into the firmware at build time. Fonts are self-hosted WOFF2
  subsets in `web/fonts/`. Keep them small (Latin subset, 2 weights): every byte ships in the firmware.

### CSS tokens

```css
:root {
  --bg: #101112; --surface-1: #1A1C1E; --surface-2: #24272A; --surface-3: #2E3236;
  --line: #383C41; --text: #F2F3F4; --text-muted: #A0A6AC; --text-faint: #62696F;
  --on-bright: #0E0F10; --ok: #3DDC84; --warn: #FFB224; --error: #FF4D4F; --link: #5AA9FF;
  --radius-tile: 14px; --radius-panel: 12px; --radius-control: 8px;
  color-scheme: dark;
}
```

### LVGL tokens

Define the interface (chrome) tokens once in `src/ui/theme.h` and never write a hex value anywhere
else in UI code. Tile colours are **not** in firmware: the browser compiles swatch names and the
solid/soft style into final RGB values in the `pad` section (see the config schema), and the
firmware draws them as given.

```cpp
namespace theme {
inline lv_color_t bg()        { return lv_color_hex(0x101112); }
inline lv_color_t surface1()  { return lv_color_hex(0x1A1C1E); }
inline lv_color_t surface2()  { return lv_color_hex(0x24272A); }
inline lv_color_t text()      { return lv_color_hex(0xF2F3F4); }
// ...one function per chrome token. No swatch table here: tile colours arrive compiled.
}
```
