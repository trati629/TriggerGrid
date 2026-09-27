// test.js — known-answer tests for web/lib. Run them by opening
// web/test.html in a browser, or with `node tools/test_web.js`.
(function (TG) {
  'use strict';

  function settings(os) {
    return { deviceName: 'T', density: 'regular', brightness: 180, dimAfterSec: 0, hostLayout: 'us', hostOS: os };
  }
  function strokes(text, os) {
    const r = TG.compileText({ type: 'text', text: text }, settings(os));
    return r.error ? 'error' : r.a.s;
  }

  // [name, actual, expected]
  TG.tests = function () {
    const def = TG.compilePad(TG.defaultEditor());
    return [
      ['CTRL + C → modifier 0x01, usage 0x06',
        JSON.stringify(TG.compileAction({ type: 'keys', keys: ['CTRL', 'C'] }, settings('windows')).a),
        JSON.stringify({ t: 'k', m: 1, k: [6] })],
      ['F13 reaches usage 0x68', TG.compileAction({ type: 'keys', keys: ['F13'] }, settings('windows')).a.k[0], 0x68],
      ['F24 reaches usage 0x73', TG.compileAction({ type: 'keys', keys: ['F24'] }, settings('windows')).a.k[0], 0x73],
      ['Unknown key is an error', !!TG.compileAction({ type: 'keys', keys: ['NOPE'] }, settings('windows')).error, true],
      ['PLAY_PAUSE → consumer 205', TG.compileAction({ type: 'media', key: 'PLAY_PAUSE' }, settings('windows')).a.u, 205],
      ['"Cheers,\\nAlex" on US', strokes('Cheers,\nAlex', 'windows'), '0206000B0008000800150016003600280204000F0008001B'],
      ['Soft violet tile fill', TG.compileTile({ x: 0, y: 0, w: 1, h: 1, label: '', color: 'violet', style: 'soft', action: { type: 'media', key: 'MUTE' } }, settings('windows')).bg, '2F2942'],
      ['Soft coral tile fill', TG.compileTile({ x: 0, y: 0, w: 1, h: 1, label: '', color: 'coral', style: 'soft', action: { type: 'media', key: 'MUTE' } }, settings('windows')).bg, '3F2628'],
      ['Solid azure tile', JSON.stringify(TG.compileTile({ x: 0, y: 0, w: 1, h: 1, label: '', color: 'azure', style: 'solid', action: { type: 'media', key: 'MUTE' } }, settings('windows')).fg), '"0E0F10"'],
      ['é on Windows (Alt 0233)', strokes('é', 'windows'), '0462045A045B045B0000'],
      ['é then ! on Windows (schema example)', strokes('é!', 'windows'), '0462045A045B045B0000021E'],
      ['é on Windows sets np', !!TG.compileText({ text: 'é' }, settings('windows')).a.np, true],
      ['é on macOS (Option 00e9)', strokes('é', 'mac'), '04270427040804260000'],
      ['é on Linux (Ctrl+Shift+U e9 Space)', strokes('é', 'linux'), '031800080026002C'],
      ['é on Other is an error', strokes('é', 'other'), 'error'],
      ['👋 on macOS (surrogate pair d83d dc4b)', strokes('👋', 'mac'), '040704250420040704070406042104050000'],
      ['👋 on Linux (1f44b)', strokes('👋', 'linux'), '0318001E0009002100210005002C'],
      ['👋 on Windows (hex numpad)', strokes('👋', 'windows'), '045704590409045C045C04050000'],
      ['👋 on Windows flags hexNumpad', TG.compileText({ text: '👋' }, settings('windows')).hexNumpad, true],
      ['Control characters are rejected', strokes('a\u0007', 'windows'), 'error'],
      ['Overlap is caught', TG.placeError({ x: 0, y: 0, w: 2, h: 1 }, { tiles: [{ x: 1, y: 0, w: 1, h: 1, label: 'B' }] }, 'regular', null), 'Overlaps “B”.'],
      ['Off-grid is caught', TG.placeError({ x: 4, y: 0, w: 2, h: 1 }, { tiles: [] }, 'regular', null) !== '', true],
      ['Compact grid has 6 columns', TG.placeError({ x: 4, y: 0, w: 2, h: 1 }, { tiles: [] }, 'compact', null), ''],
      ['Default layout saves cleanly', JSON.stringify(TG.saveProblems(TG.defaultEditor())), '[]'],
      ['Default layout: 3 pages', def.pages.length, 3],
      ['Default layout: Copy icon is 1', def.pages[0].tiles[0].icon, 1],
      ['Default layout: Email sig icon is 3', def.pages[0].tiles[2].icon, 3],
      ['Default layout: Play / Pause icon is 5', def.pages[0].tiles[4].icon, 5],
      ['sanitizeEditor turns a hostile x into a number',
        TG.sanitizeEditor({ settings: {}, pages: [{ name: 'P', tiles: [{ x: '"><img src=x>', label: 'A' }] }] }).pages[0].tiles[0].x, 0],
      ['sanitizeEditor keeps a valid layout intact',
        JSON.stringify(TG.sanitizeEditor(TG.defaultEditor())), JSON.stringify(TG.defaultEditor())],
      ['Default file is under 32 KB', TG.savedFile(TG.defaultEditor()).length < TG.MAX_FILE, true],
    ];
  };

  // Run all tests; returns { passed, failed: [ {name, actual, expected} ] }.
  TG.runTests = function () {
    const failed = [];
    let passed = 0;
    for (const t of TG.tests()) {
      if (String(t[1]) === String(t[2])) passed++;
      else failed.push({ name: t[0], actual: t[1], expected: t[2] });
    }
    return { passed: passed, failed: failed };
  };
})(globalThis.TG = globalThis.TG || {});
