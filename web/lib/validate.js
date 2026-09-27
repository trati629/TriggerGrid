// validate.js — the layout rules the browser enforces before saving.
// The device only does cheap bounds checks; everything meaningful is here
// (docs/architecture.md, "Thin device, smart browser").
(function (TG) {
  'use strict';

  // Grid geometry per density (style guide, "Grid densities").
  TG.GRID = {
    regular: { cols: 5, rows: 3, w: 88, h: 88, gap: 6, padX: 8, padY: 6 },
    compact: { cols: 6, rows: 4, w: 73, h: 64, gap: 6, padX: 6, padY: 7 },
  };
  TG.STATUS_H = 22;
  TG.MAX_FILE = 32 * 1024;
  TG.LIMITS = { pages: 12, tiles: 24, label: 24, pageName: 20, deviceName: 24, text: 1024, strokes: 4096 };

  // A spot {x, y, w, h} must fit the grid and not overlap another tile.
  // `ignore` is the tile being moved or resized. Returns '' when it's fine.
  TG.placeError = function (spot, page, density, ignore) {
    const g = TG.GRID[density];
    if (spot.x < 0 || spot.y < 0 || spot.x + spot.w > g.cols || spot.y + spot.h > g.rows) {
      return 'Doesn\'t fit: the ' + density + ' grid is ' + g.cols + '×' + g.rows + '.';
    }
    const hit = page.tiles.find(function (o) {
      return o !== ignore && spot.x < o.x + o.w && o.x < spot.x + spot.w && spot.y < o.y + o.h && o.y < spot.y + spot.h;
    });
    return hit ? 'Overlaps “' + hit.label + '”.' : '';
  };
  TG.fitError = function (tile, page, density) { return TG.placeError(tile, page, density, tile); };

  // The pad's font covers printable ASCII only (until the custom font lands),
  // so other characters in a label show as nothing. Not an error, a warning.
  TG.labelWarning = function (label) {
    const odd = [...new Set([...(label || '')].filter(function (c) { return c < ' ' || c > '~'; }))];
    return odd.length ? 'The pad can’t show ' + odd.map(function (c) { return '“' + c + '”'; }).join(', ') + ' yet.' : '';
  };

  // Everything that blocks a save, across all pages. Uses the compiler, so
  // load compile.js too.
  TG.saveProblems = function (editor) {
    const out = [];
    const s = editor.settings, L = TG.LIMITS;
    if (!s.deviceName || [...s.deviceName].length > L.deviceName) out.push('Device name must be 1–24 characters.');
    if (!editor.pages.length || editor.pages.length > L.pages) out.push('A layout has 1–12 pages.');
    editor.pages.forEach(function (p) {
      if ([...p.name].length > L.pageName) out.push(p.name + ': page names are at most 20 characters.');
      if (p.tiles.length > L.tiles) out.push(p.name + ': at most 24 tiles per page.');
      p.tiles.forEach(function (t) {
        const where = p.name + ' › ' + (t.label || '(no label)') + ': ';
        const fit = TG.fitError(t, p, s.density);
        const act = TG.compileAction(t.action, s).error;
        if (fit) out.push(where + fit);
        if (act) out.push(where + act);
        if (new TextEncoder().encode(t.label).length > L.label) out.push(where + 'the label is too long.');
      });
    });
    const size = new TextEncoder().encode(TG.savedFile(editor)).length;
    if (size > TG.MAX_FILE) out.push('The layout is ' + (size / 1024).toFixed(1) + ' KB; the device limit is 32 KB.');
    return out;
  };
})(globalThis.TG = globalThis.TG || {});
