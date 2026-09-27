// unicode.js — typing characters that aren't on the computer's keyboard.
//
// A Bluetooth keyboard sends key *positions*, never characters. Anything the
// layout can't type (é, €, 👋) goes through the computer's own Unicode input,
// which differs per OS (docs/config-schema.md, "Characters beyond the
// keyboard"). Each method returns keystrokes as [modifier, usage] pairs.
// Keystroke rules the firmware follows:
//   - keystrokes with the same modifier keep that modifier held;
//   - [0, 0] releases everything (it ends an Alt or Option sequence).
(function (TG) {
  'use strict';

  TG.HOST_OS = {
    windows: { name: 'Windows', how: 'Accented letters and symbols such as é, ñ and € are typed with Alt codes (Alt + numpad) and need no setup. Other characters and emoji use hex numpad input: it needs a one-time registry setting and doesn’t work in every app.' },
    mac: { name: 'macOS', how: 'Characters that aren’t on the keyboard are typed with Unicode Hex Input. One-time setup: System Settings › Keyboard › Input Sources, add “Unicode Hex Input” and switch to it. It types normal text like a US keyboard.' },
    linux: { name: 'Linux', how: 'Characters that aren’t on the keyboard are typed with Ctrl+Shift+U (IBus, the default on GNOME and Ubuntu). Works in most apps; a few terminals and non-IBus desktops ignore it.' },
    other: { name: 'Other (iPad, Android, …)', how: 'These have no general way to type a character by its code, so only characters on the US keyboard can be used.' },
  };

  // Type hex digits: letters with the letter keys, digits with `digitKey`.
  function hexKeys(hex, modifier, digitKey) {
    return [...hex].map(function (c) {
      return /\d/.test(c) ? [modifier, digitKey(c)] : [modifier, TG.USAGE[c.toUpperCase()]];
    });
  }
  const rowDigit = function (d) { return TG.USAGE[d]; };

  // Windows code page 1252 (the Western "ANSI" set) → Alt code. 0xA0–0xFF match Unicode.
  const CP1252 = {
    '€': 128, '‚': 130, 'ƒ': 131, '„': 132, '…': 133, '†': 134, '‡': 135, 'ˆ': 136, '‰': 137, 'Š': 138,
    '‹': 139, 'Œ': 140, 'Ž': 142, '‘': 145, '’': 146, '“': 147, '”': 148, '•': 149, '–': 150, '—': 151,
    '˜': 152, '™': 153, 'š': 154, '›': 155, 'œ': 156, 'ž': 158, 'Ÿ': 159,
  };
  function cp1252(ch, cp) {
    if (ch in CP1252) return CP1252[ch];
    return cp >= 0xA0 && cp <= 0xFF ? cp : null;
  }

  // Each returns { strokes, np?, hexNumpad? } or null if the OS can't type it.
  TG.UNICODE_INPUT = {
    // Linux (IBus): Ctrl+Shift+U, the hex code point, Space.  é → Ctrl+Shift+U e 9 Space
    linux: function (ch, cp) {
      return { strokes: [[0x03, TG.USAGE.U]].concat(hexKeys(cp.toString(16), 0, rowDigit), [[0, TG.USAGE.SPACE]]) };
    },

    // macOS (Unicode Hex Input): hold Option, type 4 hex digits per UTF-16
    // unit. Emoji are two units (a surrogate pair), typed in one Option hold.
    mac: function (ch) {
      let hex = '';
      for (let i = 0; i < ch.length; i++) hex += ch.charCodeAt(i).toString(16).padStart(4, '0');
      return { strokes: hexKeys(hex, 0x04, rowDigit).concat([[0, 0]]) };
    },

    // Windows: Alt + numpad decimal code for the CP1252 set (no setup), else
    // Alt + numpad-plus + hex (needs EnableHexNumpad in the registry). Both
    // use the numpad, so NumLock must be on (`np`).
    windows: function (ch, cp) {
      const code = cp1252(ch, cp);
      if (code !== null) {
        const digits = '0' + String(code).padStart(3, '0');
        return { np: true, strokes: [...digits].map(function (d) { return [0x04, TG.NUMPAD.DIGIT(d)]; }).concat([[0, 0]]) };
      }
      return {
        np: true, hexNumpad: true,
        strokes: [[0x04, TG.NUMPAD.PLUS]].concat(hexKeys(cp.toString(16), 0x04, TG.NUMPAD.DIGIT), [[0, 0]]),
      };
    },

    other: function () { return null; },
  };
})(globalThis.TG = globalThis.TG || {});
