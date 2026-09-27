// swatches.js — tile colours and the interface tokens the compiler needs.
// Values from docs/style-guide.md. This is the only place swatch colours live:
// the firmware receives final RGB values and has no swatch table.
(function (TG) {
  'use strict';

  // Interface tokens used when compiling a tile.
  TG.TOKENS = {
    text: 'F2F3F4',       // label on dark tiles
    onBright: '0E0F10',   // label on light tiles
    surface1: '1A1C1E',   // what a soft tile is mixed into
  };

  // name → [fill, label token]. Each pair has at least 4.5:1 contrast.
  TG.SWATCHES = {
    graphite:  ['2C3034', 'text'],
    steel:     ['8A96A3', 'onBright'],
    volt:      ['C6F432', 'onBright'],
    mint:      ['3EE6A8', 'onBright'],
    aqua:      ['2BD4E6', 'onBright'],
    azure:     ['3D8BFF', 'onBright'],
    violet:    ['9B6BFF', 'onBright'],
    magenta:   ['F24FD1', 'onBright'],
    coral:     ['FF5C5C', 'onBright'],
    tangerine: ['FF8A1F', 'onBright'],
    sun:       ['FFD60A', 'onBright'],
    crimson:   ['C8233B', 'text'],
  };

  // Share `t` of colour a over colour b, both 'RRGGBB'. Same maths as a soft
  // tile in the style guide: color-mix(in srgb, swatch 16%, surface-1).
  TG.mixHex = function (a, b, t) {
    return [0, 2, 4].map(function (i) {
      const v = Math.round(parseInt(a.substr(i, 2), 16) * t + parseInt(b.substr(i, 2), 16) * (1 - t));
      return v.toString(16).toUpperCase().padStart(2, '0');
    }).join('');
  };
})(globalThis.TG = globalThis.TG || {});
