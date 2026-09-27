// compile.js — the editor section → the pad section (docs/config-schema.md).
//
// This is the browser half of "thin device, smart browser": the firmware
// receives numbers only (RGB, icon indexes, HID codes, keystroke hex) and has
// none of these tables. Needs swatches.js, icons.js, keymap.js, layouts/*.js
// and unicode.js loaded first.
(function (TG) {
  'use strict';

  const hex2 = function (n) { return n.toString(16).toUpperCase().padStart(2, '0'); };

  function layoutOf(settings) { return TG.LAYOUTS[settings.hostLayout] || TG.LAYOUTS.us; }
  function osOf(settings) { return TG.HOST_OS[settings.hostOS] ? settings.hostOS : 'windows'; }
  TG.hostOS = osOf;

  // Typed text → keystrokes: the layout first, the OS's Unicode method for the
  // rest. Returns { a, strokes, unicode, hexNumpad } or { error }.
  TG.compileText = function (action, settings) {
    const map = layoutOf(settings).map, os = osOf(settings), chars = [...(action.text || '')];
    const strokes = [], unicode = [], bad = [];
    let np = false, hexNumpad = false;
    for (const ch of chars) {
      if (map[ch]) { strokes.push(map[ch]); continue; }
      const cp = ch.codePointAt(0);
      const isControl = cp < 0x20 || (cp >= 0x7F && cp < 0xA0);
      const seq = isControl ? null : TG.UNICODE_INPUT[os](ch, cp);
      if (!seq) { bad.push(ch); continue; }
      strokes.push.apply(strokes, seq.strokes);
      unicode.push(ch);
      np = np || !!seq.np;
      hexNumpad = hexNumpad || !!seq.hexNumpad;
    }
    const list = function (cs) { return [...new Set(cs)].map(function (c) { return '“' + c + '”'; }).join(', '); };
    if (bad.length) {
      return { error: os === 'other'
        ? 'Can\'t type ' + list(bad) + ': with the computer set to ' + TG.HOST_OS.other.name + ', only US-keyboard characters work.'
        : 'Can\'t type ' + list(bad) + ' (control characters).' };
    }
    if (chars.length > TG.LIMITS.text) return { error: 'Text is longer than 1024 characters.' };
    if (strokes.length > TG.LIMITS.strokes) return { error: 'This text needs ' + strokes.length + ' keystrokes; the limit is 4096.' };
    const delay = action.charDelayMs == null ? 10 : action.charDelayMs;
    if (!(delay >= 5 && delay <= 100)) return { error: 'The delay must be 5–100 ms.' };
    const a = { t: 's', d: delay, s: strokes.map(function (k) { return hex2(k[0]) + hex2(k[1]); }).join('') };
    if (np) a.np = true;   // the firmware turns NumLock on for numpad codes, then restores it
    return { a: a, strokes: strokes.length, unicode: [...new Set(unicode)], hexNumpad: hexNumpad };
  };

  // One tile's action → compiled action. Returns { a } or { error }.
  TG.compileAction = function (action, settings) {
    if (!action) return { error: 'No action.' };
    if (action.type === 'keys') {
      let m = 0;
      const k = [];
      for (const name of action.keys || []) {
        if (name in TG.MODIFIERS) m |= TG.MODIFIERS[name];
        else if (name in TG.USAGE) k.push(TG.USAGE[name]);
        else return { error: 'Unknown key “' + name + '”.' };
      }
      if (!m && !k.length) return { error: 'Add at least one key.' };
      if (k.length > 6) return { error: 'At most 6 non-modifier keys.' };
      return { a: { t: 'k', m: m, k: k } };
    }
    if (action.type === 'media') {
      return action.key in TG.CONSUMER
        ? { a: { t: 'c', u: TG.CONSUMER[action.key] } }
        : { error: 'Unknown media key “' + action.key + '”.' };
    }
    if (action.type === 'text') return TG.compileText(action, settings);
    return { error: 'Unknown action type “' + action.type + '”.' };
  };

  // One tile → its pad form. A tile whose action has an error gets `a: null`
  // (saveProblems() blocks the save before that could reach the device).
  TG.compileTile = function (t, settings) {
    const sw = TG.SWATCHES[t.color] || TG.SWATCHES.graphite;
    const swHex = sw[0], onHex = TG.TOKENS[sw[1]];
    const soft = t.style === 'soft';
    const compiled = TG.compileAction(t.action, settings);
    return {
      x: t.x, y: t.y, w: t.w, h: t.h, label: t.label, icon: TG.ICON_INDEX[t.icon] || 0,
      bg: soft ? TG.mixHex(swHex, TG.TOKENS.surface1, 0.16) : swHex,
      fg: soft ? TG.TOKENS.text : onHex,
      accent: soft ? swHex : onHex,
      bar: soft,
      a: compiled.a || null,
    };
  };

  TG.compilePad = function (editor) {
    const s = editor.settings;
    return {
      v: 1, name: s.deviceName, density: s.density === 'compact' ? 1 : 0,
      brightness: s.brightness, dim: s.dimAfterSec,
      pages: editor.pages.map(function (p) {
        return { name: p.name, tiles: p.tiles.map(function (t) { return TG.compileTile(t, s); }) };
      }),
    };
  };

  // The whole file as PUT /api/config sends it: compact JSON, pad regenerated.
  TG.savedFile = function (editor) {
    return JSON.stringify({ version: 1, editor: editor, pad: TG.compilePad(editor) });
  };
})(globalThis.TG = globalThis.TG || {});
