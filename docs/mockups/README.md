# Mockups

## Web config page: `web-config.html`

A clickable mockup of the page the device serves at `http://triggergrid.local` (roadmap M8/M9).
Open the file in any browser. It is a single file with no build step and no internet access
needed. In VS Code, right-click the file and choose *Reveal in File Explorer* / *Finder*, then
double-click it. Add `#settings` to the URL to open the settings view directly.

![Tile editor](../images/web-config-editor.png)

| Settings view | Phone layout |
|---|---|
| ![Device settings](../images/web-config-settings.png) | <img src="../images/web-config-phone.png" width="260" alt="Phone layout: page tabs across the top, scaled preview, tile editor as a bottom sheet"> |

**What works in the mockup:**
- Switch pages, add a page, rename the page (the name updates in the preview's status bar).
- Tap a tile to edit its label, colour, style (solid / soft), size, icon and action. The preview
  updates as you go.
- Tap an empty `+` cell to add a tile there. Delete a tile.
- Size changes are checked against the grid rules from the config schema (must fit the grid,
  no overlaps), and the error is shown under *Size*.
- **Record combo:** press a key combination and it's converted to config key names (`CTRL`, `SHIFT`, `M`).
- Switch density between Regular 5×3 and Compact 6×4.
- *config.json for this page* shows the live JSON in the [config schema](../config-schema.md) format.

**What's faked:** nothing is sent to a device. *Save*, *Try it* and the Wi-Fi, Bluetooth and backup
buttons only show a message. Status values (IP, signal, heap) are placeholders.

**How it maps to the real page:** the real `data/index.html` (M8) can start from this file. The
data model, grid maths, swatch table and key-name mapping are already the ones the firmware will
use. What changes is loading and saving through `GET`/`PUT /api/config`.

**Design notes:**
- The device preview is drawn at 1:1 (480×320 plus bezel) using the style-guide grid numbers, and
  scales down only when the screen is narrower than that.
- The interface is neutral graphite. The only colour is in the tiles and the status dots, and the
  primary button is white, as the style guide specifies.
- Fonts fall back to system fonts here. The real page will self-host Space Grotesk and JetBrains
  Mono subsets in `data/fonts/`.

To refresh the screenshots after changing the mockup, render with the pre-installed Chromium in
headless mode (desktop at 1360×860, phone at 390 px wide) and save them over the files in `docs/images/`.
