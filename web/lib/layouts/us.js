// layouts/us.js — characters typed on a computer set to US English.
// character → [modifier byte, key usage]. Adding a layout (uk, de, …) means
// adding a file like this one; the firmware never sees characters.
(function (TG) {
  'use strict';

  const map = { ' ': [0, 0x2C], '\n': [0, 0x28], '\t': [0, 0x2B] };
  for (let i = 0; i < 26; i++) {
    map[String.fromCharCode(97 + i)] = [0, 0x04 + i];      // a–z
    map[String.fromCharCode(65 + i)] = [0x02, 0x04 + i];   // A–Z (Shift)
  }
  '1234567890'.split('').forEach(function (c, i) { map[c] = [0, 0x1E + i]; });
  '!@#$%^&*()'.split('').forEach(function (c, i) { map[c] = [0x02, 0x1E + i]; });
  [['-', '_', 0x2D], ['=', '+', 0x2E], ['[', '{', 0x2F], [']', '}', 0x30], ['\\', '|', 0x31],
   [';', ':', 0x33], ["'", '"', 0x34], ['`', '~', 0x35], [',', '<', 0x36], ['.', '>', 0x37], ['/', '?', 0x38]]
    .forEach(function (k) { map[k[0]] = [0, k[2]]; map[k[1]] = [0x02, k[2]]; });

  TG.LAYOUTS = TG.LAYOUTS || {};
  TG.LAYOUTS.us = { name: 'US English', map: map };
})(globalThis.TG = globalThis.TG || {});
