# web/fonts

Latin subsets of the page's fonts, self-hosted because the page has to work on the pad's setup
hotspot, with no internet. `tools/embed_web.py` builds them into the firmware as they are
(WOFF2 is already compressed).

| File | Font | Licence |
|------|------|---------|
| `space-grotesk-latin.woff2` | Space Grotesk (variable, all weights) | SIL OFL 1.1, `OFL-SpaceGrotesk.txt` |
| `jetbrains-mono-500-latin.woff2` | JetBrains Mono Medium | SIL OFL 1.1, `OFL-JetBrainsMono.txt` |

Both come from Google Fonts' `latin` subset.
