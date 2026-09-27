# Mockups

## Web config page: `web-config.html`

A clickable mockup of the page the device serves at `http://triggergrid.local` (roadmap M8/M9).
Open the file in any browser. It is a single file with no build step and no internet access
needed. In VS Code, right-click the file and choose *Reveal in File Explorer* / *Finder*, then
double-click it. Add `#settings` to the URL to open the settings view directly.

![Tile editor](../images/web-config-editor.png)

![Dragging a preset from the tray onto the screen](../images/web-config-drag.png)

| The config.json panel: editor vs compiled pad | Emoji in a text tile, computer set to Windows |
|---|---|
| ![config.json panel showing the editor and pad sections side by side](../images/web-config-json.png) | ![Text box with an emoji and an amber note that it will be typed with Windows Unicode input](../images/web-config-text-unicode.png) |

| Settings view | Phone layout |
|---|---|
| ![Device settings](../images/web-config-settings.png) | <img src="../images/web-config-phone.png" width="260" alt="Phone layout: page tabs across the top, scaled preview, tile editor as a bottom sheet"> |

**What works in the mockup:**
- **Drag and drop on the preview:**
  - drag a blank tile or a preset from the *Add tiles* tray onto the screen;
  - drag a tile to move it, and drag the white corner handle of the selected tile to resize it;
  - drag a tile back onto the tray to delete it;
  - a dashed outline shows where the tile will land. It turns red if the spot is taken or off the
    grid, and dropping there does nothing;
  - arrow keys move the focused tile one cell.
- Switch pages, add a page, rename the page (the name updates in the preview's status bar).
- Tap a tile to edit its label, colour, style (solid / soft), size, icon and action. The preview
  updates as you go.
- Tap an empty `+` cell to add a blank tile there, or delete a tile from the editor.
- All placement follows the grid rules from the config schema (must fit the grid, no overlaps).
- **The layout compiler runs in the page**, exactly as the real one will. The *config.json*
  panel shows the human-readable `editor` section next to the compiled `pad` section, the only
  part the device reads, and updates as you edit. Watch `CTRL` + `C` become `"m": 1, "k": [6]`.
  Below them, a meter shows the whole file's size against the device's 32 KB limit.
- **Typed text can use any character, including accents, other scripts and emoji.** Characters on
  the US layout are typed as normal keys. Everything else is compiled into the computer's
  Unicode input method, chosen by Settings → Bluetooth → *Computer* (Windows, macOS, Linux,
  Other). The Settings note explains the one-time setup for each.
  - Under the text box, the editor shows how many keystrokes the text needs and which characters
    use Unicode input.
  - It warns in amber where the method is unreliable (emoji on Windows).
  - It blocks *Try it* and *Save* only when a character can't be typed at all (anything beyond
    the US keyboard when *Computer* is set to Other).
- *Try it* shows the compiled action it would send to `POST /api/test`, and *Save* shows the
  size of the file it would upload.
- **Record combo:** press a key combination and it's converted to config key names (`CTRL`, `SHIFT`, `M`).
- Switch density between Regular 5×3 and Compact 6×4.
- *config.json for this page* shows the live JSON in the [config schema](../config-schema.md) format.

**What's faked:** nothing is sent to a device. *Save*, *Try it* and the Wi-Fi, Bluetooth and backup
buttons only show a message. Status values (IP, signal, heap) are placeholders.

**How it maps to the real page:** the real `web/index.html` (M8/M9) can start from this file. The
data model, grid maths, swatch table and key-name mapping are already the ones the firmware will
use, and so is the compiler (the *compiler* section of the script moves to `web/lib/` in M6). What
changes is loading and saving through `GET`/`PUT /api/config`.

**Design notes:**
- The device preview is drawn at 1:1 (480×320 plus bezel) using the style-guide grid numbers, and
  scales down only when the screen is narrower than that.
- The interface is neutral graphite. The only colour is in the tiles and the status dots, and the
  primary button is white, as the style guide specifies.
- Fonts fall back to system fonts here. The real page will self-host Space Grotesk and JetBrains
  Mono subsets in `web/fonts/`, embedded in the firmware.

To refresh the screenshots after changing the mockup, render it in a browser (desktop at
1360×860, phone at 390 px wide) and save the images over the files in `docs/images/`. The
drag screenshot is taken while dragging the *Copy* preset over an empty cell on the *Code* page.
