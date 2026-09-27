// keymap.js — key names used in the editor → HID codes sent by the pad.
// USB HID Usage Tables: keyboard page 0x07, consumer page 0x0C. Key names are
// US-layout physical keys (docs/config-schema.md, "keys: key combo").
(function (TG) {
  'use strict';

  // Modifier byte bits.
  TG.MODIFIERS = {
    CTRL: 0x01, LCTRL: 0x01, SHIFT: 0x02, LSHIFT: 0x02, ALT: 0x04, LALT: 0x04, GUI: 0x08, LGUI: 0x08,
    RCTRL: 0x10, RSHIFT: 0x20, RALT: 0x40, RGUI: 0x80,
  };

  // Keyboard usage codes.
  const USAGE = {
    ENTER: 0x28, ESC: 0x29, BACKSPACE: 0x2A, TAB: 0x2B, SPACE: 0x2C, MINUS: 0x2D, EQUAL: 0x2E,
    LBRACKET: 0x2F, RBRACKET: 0x30, BACKSLASH: 0x31, SEMICOLON: 0x33, QUOTE: 0x34, GRAVE: 0x35, COMMA: 0x36,
    PERIOD: 0x37, SLASH: 0x38, CAPS_LOCK: 0x39, PRINT_SCREEN: 0x46, INSERT: 0x49, HOME: 0x4A, PAGE_UP: 0x4B,
    DELETE: 0x4C, END: 0x4D, PAGE_DOWN: 0x4E, RIGHT: 0x4F, LEFT: 0x50, DOWN: 0x51, UP: 0x52,
  };
  for (let i = 0; i < 26; i++) USAGE[String.fromCharCode(65 + i)] = 0x04 + i;   // A–Z
  for (let i = 1; i <= 9; i++) USAGE[String(i)] = 0x1E + i - 1;                 // 1–9
  USAGE['0'] = 0x27;
  for (let i = 1; i <= 12; i++) USAGE['F' + i] = 0x3A + i - 1;                  // F1–F12
  for (let i = 13; i <= 24; i++) USAGE['F' + i] = 0x68 + i - 13;                // F13–F24
  TG.USAGE = USAGE;

  // Numpad keys, used by the Windows Unicode methods.
  TG.NUMPAD = { PLUS: 0x57, DIGIT: function (d) { return d === '0' ? 0x62 : 0x59 + (+d - 1); } };

  // Consumer (media) usage codes.
  TG.CONSUMER = {
    PLAY_PAUSE: 0xCD, NEXT_TRACK: 0xB5, PREV_TRACK: 0xB6, STOP: 0xB7, MUTE: 0xE2,
    VOLUME_UP: 0xE9, VOLUME_DOWN: 0xEA, BRIGHTNESS_UP: 0x6F, BRIGHTNESS_DOWN: 0x70,
  };
  TG.MEDIA_KEYS = Object.keys(TG.CONSUMER);

  // KeyboardEvent.code → key name, for "Record combo" in the editor.
  TG.CODE_NAMES = {
    Enter: 'ENTER', Escape: 'ESC', Tab: 'TAB', Space: 'SPACE', Backspace: 'BACKSPACE', Delete: 'DELETE',
    Insert: 'INSERT', Home: 'HOME', End: 'END', PageUp: 'PAGE_UP', PageDown: 'PAGE_DOWN', ArrowUp: 'UP',
    ArrowDown: 'DOWN', ArrowLeft: 'LEFT', ArrowRight: 'RIGHT', Minus: 'MINUS', Equal: 'EQUAL',
    BracketLeft: 'LBRACKET', BracketRight: 'RBRACKET', Backslash: 'BACKSLASH', Semicolon: 'SEMICOLON',
    Quote: 'QUOTE', Backquote: 'GRAVE', Comma: 'COMMA', Period: 'PERIOD', Slash: 'SLASH',
    PrintScreen: 'PRINT_SCREEN', CapsLock: 'CAPS_LOCK',
  };
  TG.keyNameFromCode = function (code) {
    if (/^Key[A-Z]$/.test(code)) return code.slice(3);
    if (/^Digit\d$/.test(code)) return code.slice(5);
    if (/^F\d{1,2}$/.test(code)) return code;
    return TG.CODE_NAMES[code] || null;
  };
})(globalThis.TG = globalThis.TG || {});
