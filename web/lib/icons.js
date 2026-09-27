// icons.js — tile icons, drawn as 20×20 stroke SVG.
// The ORDER matters: an icon is sent to the pad as its position in this list
// (1 = copy, 2 = paste, …, 0 = none). src/ui/icons.h maps the same numbers to
// glyphs on the device. Only ever add new icons at the end.
(function (TG) {
  'use strict';

  TG.ICONS = [
    ['copy', '<rect x="6" y="6" width="11" height="11" rx="2"/><path d="M3 13V5a2 2 0 0 1 2-2h8"/>'],
    ['paste', '<rect x="4" y="4" width="12" height="14" rx="2"/><rect x="7" y="2" width="6" height="4" rx="1"/>'],
    ['mail', '<rect x="2" y="4" width="16" height="12" rx="2"/><path d="M2.5 5.5 10 11l7.5-5.5"/>'],
    ['undo', '<path d="M7 5 3 9l4 4"/><path d="M3 9h9a5 5 0 0 1 0 10h-2"/>'],
    ['playpause', '<path d="M3 4v12l8-6z" fill="currentColor"/><path d="M14 4v12M18 4v12"/>'],
    ['next', '<path d="M3 4v12l9-6z" fill="currentColor"/><path d="M16 4v12"/>'],
    ['prev', '<path d="M17 4v12L8 10z" fill="currentColor"/><path d="M4 4v12"/>'],
    ['volup', '<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="M13 10h6M16 7v6"/>'],
    ['voldown', '<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="M13 10h6"/>'],
    ['mute', '<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="m13 7 6 6M19 7l-6 6"/>'],
    ['shot', '<path d="M2 7V4a2 2 0 0 1 2-2h3M13 2h3a2 2 0 0 1 2 2v3M18 13v3a2 2 0 0 1-2 2h-3M7 18H4a2 2 0 0 1-2-2v-3"/><circle cx="10" cy="10" r="3"/>'],
    ['lock', '<rect x="3" y="9" width="14" height="10" rx="2"/><path d="M6 9V6a4 4 0 0 1 8 0v3"/>'],
    ['code', '<path d="m7 5-5 5 5 5M13 5l5 5-5 5"/>'],
    ['save', '<path d="M4 2h9l4 4v11a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V3a1 1 0 0 1 1-1z"/><path d="M6 2v5h7V2M6 18v-6h8v6"/>'],
    ['search', '<circle cx="9" cy="9" r="6"/><path d="m13.5 13.5 4.5 4.5"/>'],
    ['mic', '<rect x="7" y="2" width="6" height="10" rx="3"/><path d="M4 10a6 6 0 0 0 12 0M10 16v3"/>'],
    ['terminal', '<rect x="2" y="3" width="16" height="14" rx="2"/><path d="m5 8 3 2-3 2M10 13h5"/>'],
    ['keyboard', '<rect x="2" y="5" width="16" height="10" rx="2"/><path d="M5.5 8.5h.01M8.5 8.5h.01M11.5 8.5h.01M14.5 8.5h.01M6 12h8"/>'],
    ['type', '<path d="M4 5V3.5h12V5M10 3.5v13M7.5 16.5h5"/>'],
  ];

  // name → SVG body, and name → number sent to the pad.
  TG.ICON_SVG = { none: '' };
  TG.ICON_INDEX = { none: 0 };
  TG.ICONS.forEach(function (entry, i) {
    TG.ICON_SVG[entry[0]] = entry[1];
    TG.ICON_INDEX[entry[0]] = i + 1;
  });
})(globalThis.TG = globalThis.TG || {});
