// editor.js — the visual layout editor: a true-scale preview of the pad,
// tap-to-edit tiles, drag and drop, and "Try it". Ported from
// docs/mockups/web-config.html; app.js provides saving, the API and the
// other views. Load this before app.js.
(function (TG) {
  'use strict';

  TG.defaultView = 'layout';

  const $ = (id) => document.getElementById(id);
  const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
  const svg = (name, cls = 'i') => `<svg class="${cls}" viewBox="0 0 20 20">${TG.ICON_SVG[name] || ''}</svg>`;
  const ICON_NAMES = ['none'].concat(TG.ICONS.map((e) => e[0]));
  const TOKEN_CSS = { text: 'var(--text)', onBright: 'var(--on-bright)' };

  const st = () => TG.state;
  const ed = () => TG.state.editor;
  const density = () => ed().settings.density;
  const page = () => ed().pages[st().page] || ed().pages[0];
  const tile = () => page().tiles.find((t) => t.id === st().tile);
  const newId = (prefix) => prefix + Date.now().toString(36) + Math.floor(Math.random() * 1000);
  let recording = false;
  let jsonOpen = false;

  // ---- preview ---------------------------------------------------------------------

  function tileRect(t, g) {
    return { left: g.padX + t.x * (g.w + g.gap), top: TG.STATUS_H + g.padY + t.y * (g.h + g.gap),
             width: t.w * g.w + (t.w - 1) * g.gap, height: t.h * g.h + (t.h - 1) * g.gap };
  }
  const px = (r) => Object.entries(r).map(([k, v]) => `${k}:${v}px`).join(';');

  // CSS colours for a tile, matching what compile.js sends to the pad.
  function tileColors(t) {
    const sw = TG.SWATCHES[t.color] || TG.SWATCHES.graphite;
    const hex = '#' + sw[0], on = TOKEN_CSS[sw[1]], soft = t.style === 'soft';
    return { sw: hex, soft, bg: soft ? `color-mix(in srgb, ${hex} 16%, var(--surface-1))` : hex,
             fg: soft ? 'var(--text)' : on, ic: soft ? hex : on };
  }

  function tileHTML(t, rect, extra = '') {
    const c = tileColors(t);
    const iconOnly = density() === 'compact' && t.w === 1 && t.h === 1 && t.icon && t.icon !== 'none';
    return `<button class="tile ${c.soft ? 'soft' : ''} ${extra}" data-tile="${esc(t.id)}"
      aria-label="${esc(t.label)}, ${t.w}×${t.h} at column ${t.x + 1}, row ${t.y + 1}"
      style="${px(rect)};background:${c.bg};color:${c.fg};--sw:${c.sw}">
      ${iconOnly ? '<span></span>' : `<span class="label">${esc(t.label)}</span>`}
      <span style="color:${c.ic}">${svg(t.icon)}</span></button>`;
  }

  function renderPreview() {
    const g = TG.GRID[density()], pg = page(), s = TG.state.status;
    const used = new Set();
    pg.tiles.forEach((t) => { for (let x = t.x; x < t.x + t.w; x++) for (let y = t.y; y < t.y + t.h; y++) used.add(x + ',' + y); });
    let cells = '';
    for (let y = 0; y < g.rows; y++) {
      for (let x = 0; x < g.cols; x++) {
        if (!used.has(x + ',' + y)) cells += `<button class="cell" data-cell="${x},${y}" title="Add tile" style="${px(tileRect({ x, y, w: 1, h: 1 }, g))}">+</button>`;
      }
    }
    const tiles = pg.tiles.map((t) => tileHTML(t, tileRect(t, g), t.id === st().tile ? 'sel' : '')).join('');
    const sel = tile(), sr = sel && tileRect(sel, g);
    const handle = sel ? `<div class="resize" data-resize="${esc(sel.id)}" title="Drag to resize"
        style="left:${sr.left + sr.width - 13}px;top:${sr.top + sr.height - 13}px">
        <svg class="i" viewBox="0 0 20 20"><path d="M7 17h10V7M3 13l10-10"/></svg></div>` : '';
    const wifiColor = s && s.wifi.mode === 'station' && s.wifi.ip ? 'var(--ok)' : 'var(--warn)';
    const bleColor = s && s.ble.connected ? 'var(--ok)' : 'var(--warn)';
    return `
      <div class="device-wrap" id="device-wrap"><div class="device" id="device">
        <div class="screen ${density()}">
          <div class="sbar"><span>${esc(pg.name)}</span><span class="spacer"></span>
            <svg class="i" viewBox="0 0 20 20" style="color:${wifiColor}"><path d="M3 8a10 10 0 0 1 14 0M6 11a6 6 0 0 1 8 0"/><circle cx="10" cy="14.5" r="1" fill="currentColor"/></svg>
            <svg class="i" viewBox="0 0 20 20" style="color:${bleColor}"><path d="m5 6 9 8-4 3.5v-15L14 6l-9 8"/></svg></div>
          ${cells}${tiles}${handle}
          ${ed().pages.length > 1 ? `<div class="pdots">${ed().pages.map((_, i) => `<i class="${i === st().page ? 'on' : ''}"></i>`).join('')}</div>` : ''}
        </div></div></div>`;
  }

  // Scale the 1:1 preview down only when the stage is narrower than it.
  function fitDevice() {
    const wrap = $('device-wrap'), dev = $('device');
    if (!wrap) return;
    const s = Math.min(1, wrap.parentElement.clientWidth / 516);
    dev.style.transform = `scale(${s})`;
    wrap.style.width = 516 * s + 'px';
    wrap.style.height = 356 * s + 'px';
    const note = $('scale-note');
    if (note) note.textContent = s < 1 ? `Scaled to ${Math.round(s * 100)}%.` : 'Actual size.';
  }

  function updateJson() {
    if (!$('json-editor')) return;
    $('json-editor').textContent = JSON.stringify(page(), null, 2);
    $('json-pad').textContent = JSON.stringify(TG.compilePad(ed()).pages[st().page], null, 2);
    const size = new TextEncoder().encode(TG.savedFile(ed())).length, pct = Math.min(100, size / TG.MAX_FILE * 100);
    $('json-foot').innerHTML = `<span class="meter"><i style="width:${pct}%;${size > TG.MAX_FILE ? 'background:var(--error)' : ''}"></i></span>
      Whole file ${(size / 1024).toFixed(1)} KB of the 32 KB device limit`;
  }
  document.addEventListener('toggle', (e) => { if (e.target.id === 'json') jsonOpen = e.target.open; }, true);

  // Tiles that don't fit the current density, on any page (after a density switch).
  function misfitNote() {
    const bad = [];
    ed().pages.forEach((p) => p.tiles.forEach((t) => { if (TG.fitError(t, p, density())) bad.push(`${p.name} › ${t.label}`); }));
    return bad.length ? `<div class="err" style="max-width:560px">These tiles don't fit the ${density()} grid: ${esc(bad.join(', '))}. Move or resize them before saving.</div>` : '';
  }

  // ---- tile tray: templates you drag onto the screen ----------------------------------

  const TEMPLATES = [
    { group: 'Blank' },
    { label: 'Key combo', icon: 'keyboard', color: 'graphite', style: 'solid', action: { type: 'keys', keys: ['F13'] } },
    { label: 'Type text', icon: 'type', color: 'graphite', style: 'solid', action: { type: 'text', text: '', charDelayMs: 10 } },
    { label: 'Media key', icon: 'playpause', color: 'graphite', style: 'solid', action: { type: 'media', key: 'PLAY_PAUSE' } },
    { group: 'Presets' },
    { label: 'Copy', icon: 'copy', color: 'azure', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'C'] } },
    { label: 'Paste', icon: 'paste', color: 'azure', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'V'] } },
    { label: 'Undo', icon: 'undo', color: 'graphite', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'Z'] } },
    { label: 'Save', icon: 'save', color: 'violet', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'S'] } },
    { label: 'Play / Pause', icon: 'playpause', color: 'volt', style: 'solid', action: { type: 'media', key: 'PLAY_PAUSE' } },
    { label: 'Vol +', icon: 'volup', color: 'mint', style: 'solid', action: { type: 'media', key: 'VOLUME_UP' } },
    { label: 'Vol -', icon: 'voldown', color: 'mint', style: 'solid', action: { type: 'media', key: 'VOLUME_DOWN' } },
    { label: 'Mute', icon: 'mute', color: 'coral', style: 'soft', action: { type: 'media', key: 'MUTE' } },
    { label: 'Lock', icon: 'lock', color: 'crimson', style: 'solid', action: { type: 'keys', keys: ['GUI', 'L'] } },
    { label: 'Screen shot', icon: 'shot', color: 'sun', style: 'solid', action: { type: 'keys', keys: ['GUI', 'SHIFT', 'S'] } },
  ];

  function renderTray() {
    // One row of items per group; a group entry closes the previous row and opens a new one.
    const items = TEMPLATES.map((tp, i) => {
      if (tp.group) return `${i ? '</div>' : ''}<div class="tray-group">${tp.group}</div><div class="tray-items">`;
      const c = tileColors(tp);
      return `<button class="tray-item" data-template="${i}" title="Drag onto the screen"
        style="background:${c.bg};color:${c.fg}"><span>${esc(tp.label)}</span><span style="color:${c.ic}">${svg(tp.icon)}</span></button>`;
    }).join('') + '</div>';
    return `<div class="tray" id="tray">
      <div class="tray-head">Add tiles <span class="note">Drag onto the screen. Tiles start at 1×1.</span></div>
      ${items}
      <div class="tray-trash"><span><svg class="i" viewBox="0 0 20 20"><path d="M3 5h14M8 5V3h4v2M5 5l1 12h8l1-12"/></svg></span>Drop here to delete</div>
    </div>`;
  }

  function newTile(tp, spot) {
    return { id: newId('t'), ...spot, label: tp.label, icon: tp.icon, color: tp.color, style: tp.style,
             action: JSON.parse(JSON.stringify(tp.action)) };
  }

  // ---- layout view ---------------------------------------------------------------------

  TG.renderLayout = function () {
    const d = density();
    $('stage').innerHTML = `
      <div class="stage-head">
        <input class="page-name" id="page-name" value="${esc(page().name)}" maxlength="20" aria-label="Page name">
        <div class="seg" role="group" aria-label="Grid density">
          <button data-density="regular" class="${d === 'regular' ? 'on' : ''}">Regular 5×3</button>
          <button data-density="compact" class="${d === 'compact' ? 'on' : ''}">Compact 6×4</button>
        </div>
        ${ed().pages.length > 1 ? '<button class="btn small danger" id="delete-page">Delete page</button>' : ''}
      </div>
      ${renderPreview()}
      <div class="hint"><span id="scale-note">Actual size.</span> Drag tiles to move them, drag the white corner to resize,
        or drag a new tile in from the tray. Arrow keys nudge the selected tile.</div>
      ${misfitNote()}
      ${renderTray()}
      <details class="json" id="json" ${jsonOpen ? 'open' : ''}><summary>config.json: what Save sends to the device</summary>
        <div class="json-panes">
          <div><span class="json-title">editor <span class="note">· this page, for the browser</span></span><pre id="json-editor"></pre></div>
          <div><span class="json-title">pad <span class="note">· this page compiled, the only part the device reads</span></span><pre id="json-pad"></pre></div>
        </div>
        <div class="json-foot" id="json-foot"></div>
      </details>`;
    updateJson();
    fitDevice();
    renderEditor();
  };

  function textNote(a) {
    const s = ed().settings, r = TG.compileText(a, s), os = TG.HOST_OS[TG.hostOS(s)];
    if (r.error) return `<span class="err">${esc(r.error)} Remove them, or change the computer in Settings.</span>`;
    const count = `${[...(a.text || '')].length} / 1024 characters · ${r.strokes} keystrokes`;
    if (!r.unicode.length) return `<span class="note">${count}</span>`;
    const which = r.unicode.slice(0, 6).map((c) => `“${esc(c)}”`).join(', ') + (r.unicode.length > 6 ? '…' : '');
    const winNote = r.hexNumpad ? ' Emoji and characters outside the Alt-code set use Windows hex numpad input, which needs a one-time registry setting and may not work in every app.' : '';
    return `<span class="note" style="color:${r.hexNumpad ? 'var(--warn)' : 'var(--text-muted)'}">${count}<br>
      ${which} will be typed with ${os.name} Unicode input.${winNote}</span>`;
  }

  function renderEditor() {
    const t = tile();
    if (!t) {
      $('editor').innerHTML = '<h2>No tile selected</h2><p class="note">Tap a tile in the preview to edit it, or tap an empty <b>+</b> cell to add a tile there.</p>';
      return;
    }
    const a = t.action, err = TG.fitError(t, page(), density()), labelWarn = TG.labelWarning(t.label);
    const actionFields = {
      keys: () => `
        <div class="field"><span class="lbl">Key combo</span>
          <div class="keys">${a.keys.map((k, i) => `${i ? '<span class="plus">+</span>' : ''}<span class="keycap">${esc(k)}<button data-delkey="${i}" aria-label="Remove ${esc(k)}">×</button></span>`).join('')}</div>
          <div class="row"><button class="btn small ${recording ? 'recording' : ''}" id="record">
            <svg class="i" viewBox="0 0 20 20"><circle cx="10" cy="10" r="4" fill="currentColor" stroke="none"/></svg>
            ${recording ? 'Press a key combo…' : 'Record combo'}</button>
            <span class="note">US layout key positions</span></div></div>`,
      text: () => `
        <div class="field"><label for="f-text">Text to type</label>
          <textarea id="f-text" maxlength="1024">${esc(a.text || '')}</textarea>
          <span id="text-note">${textNote(a)}</span></div>
        <div class="field"><label for="f-delay">Delay between characters (ms)</label>
          <input type="number" id="f-delay" min="5" max="100" value="${a.charDelayMs == null ? 10 : a.charDelayMs}"></div>`,
      media: () => `
        <div class="field"><label for="f-media">Media key</label>
          <select id="f-media">${TG.MEDIA_KEYS.map((k) => `<option ${k === a.key ? 'selected' : ''}>${k}</option>`).join('')}</select></div>`,
    }[a.type];

    const sw = TG.SWATCHES[t.color] || TG.SWATCHES.graphite;
    $('editor').innerHTML = `
      <h2><span class="sw-dot" style="background:#${sw[0]}"></span>Edit tile</h2>
      <div class="field"><label for="f-label">Label</label>
        <input type="text" id="f-label" maxlength="24" value="${esc(t.label)}">
        <span class="note" id="label-note" style="color:var(--warn)">${esc(labelWarn)}</span></div>
      <div class="field"><span class="lbl">Colour <span class="swatch-name">· ${esc(t.color || 'graphite')}</span></span>
        <div class="swatches">${Object.entries(TG.SWATCHES).map(([n, [c]]) =>
          `<button class="swatch ${n === t.color ? 'on' : ''}" data-color="${n}" title="${n}" aria-label="${n}" style="background:#${c}"></button>`).join('')}</div></div>
      <div class="field"><span class="lbl">Style</span>
        <div class="seg full"><button data-style="solid" class="${t.style !== 'soft' ? 'on' : ''}">Solid</button><button data-style="soft" class="${t.style === 'soft' ? 'on' : ''}">Soft</button></div></div>
      <div class="field"><span class="lbl">Size</span>
        <div class="seg full">${[[1, 1], [2, 1], [1, 2], [2, 2]].map(([w, h]) => `<button data-size="${w},${h}" class="${t.w === w && t.h === h ? 'on' : ''}">${w}×${h}</button>`).join('')}</div>
        ${err ? `<span class="err">${esc(err)}</span>` : ''}</div>
      <div class="field"><span class="lbl">Icon</span>
        <div class="icons">${ICON_NAMES.map((n) => `<button data-icon="${n}" class="${(t.icon || 'none') === n ? 'on' : ''}" title="${n}">${n === 'none' ? '<span class="note">none</span>' : svg(n)}</button>`).join('')}</div></div>
      <div class="divider"></div>
      <div class="field"><span class="lbl">Action</span>
        <div class="seg full">${['keys', 'text', 'media'].map((k) => `<button data-atype="${k}" class="${a.type === k ? 'on' : ''}">${{ keys: 'Key combo', text: 'Type text', media: 'Media key' }[k]}</button>`).join('')}</div></div>
      ${actionFields ? actionFields() : ''}
      <div class="editor-actions">
        <button class="btn" id="try"><svg class="i" viewBox="0 0 20 20"><path d="M5 3v14l11-7z"/></svg>Try it</button>
        <button class="btn danger" id="delete">Delete tile</button>
      </div>`;
  }

  // Redraw the preview without touching the field being typed in.
  function refreshPreview() {
    TG.renderSidebar();
    const wrap = $('stage').querySelector('.device-wrap');
    if (wrap) { wrap.outerHTML = renderPreview(); fitDevice(); }
    updateJson();
  }

  // ---- clicks and typing (called by app.js for buttons it doesn't handle) ---------------

  TG.onClick = async function (e, el) {
    const d = el.dataset, t = tile(), s = st();
    const change = () => { TG.markDirty(); TG.render(); };
    if (d.page) { s.view = 'layout'; s.page = +d.page; s.tile = page().tiles[0] && page().tiles[0].id; recording = false; return TG.render(); }
    if (el.id === 'add-page') {
      if (ed().pages.length >= TG.LIMITS.pages) return TG.toast('A layout has at most 12 pages.', 'var(--error)');
      ed().pages.push({ id: newId('p'), name: 'New page', tiles: [] });
      s.view = 'layout'; s.page = ed().pages.length - 1; s.tile = null;
      return change();
    }
    if (el.id === 'delete-page') {
      if (!window.confirm(`Delete the page “${page().name}” and its tiles?`)) return;
      ed().pages.splice(s.page, 1);
      s.page = Math.max(0, s.page - 1); s.tile = null;
      return change();
    }
    if (d.tile) { s.tile = d.tile; recording = false; return TG.render(); }
    if (d.cell) {
      const [x, y] = d.cell.split(',').map(Number), id = newId('t');
      page().tiles.push({ id, x, y, w: 1, h: 1, label: 'New', icon: 'none', color: 'graphite', style: 'solid', action: { type: 'keys', keys: ['F13'] } });
      s.tile = id;
      return change();
    }
    if (!t) return;
    if (d.color) { t.color = d.color; return change(); }
    if (d.style) { t.style = d.style; return change(); }
    if (d.icon) { t.icon = d.icon; return change(); }
    if (d.size) { [t.w, t.h] = d.size.split(',').map(Number); return change(); }
    if (d.atype && d.atype !== t.action.type) {
      t.action = { keys: { type: 'keys', keys: ['F13'] }, text: { type: 'text', text: '', charDelayMs: 10 }, media: { type: 'media', key: 'PLAY_PAUSE' } }[d.atype];
      return change();
    }
    if (d.delkey) { t.action.keys.splice(+d.delkey, 1); return change(); }
    if (el.id === 'record') { recording = !recording; return renderEditor(); }
    if (el.id === 'delete') { page().tiles = page().tiles.filter((o) => o !== t); s.tile = null; return change(); }
    if (el.id === 'try') {
      const c = TG.compileAction(t.action, ed().settings);
      if (c.error) return TG.toast(c.error, 'var(--error)');
      const res = await TG.api('POST', '/api/test', JSON.stringify(c.a));   // runs on the pad, nothing saved
      if (res.status === 204) return TG.toast(`Sent “${t.label}” to the computer.`);
      return TG.toast((res.body && res.body.error) || 'HTTP ' + res.status, 'var(--error)');
    }
  };

  TG.onInput = function (e) {
    const t = tile(), v = e.target.value;
    switch (e.target.id) {
      case 'f-label': t.label = v; $('label-note').textContent = TG.labelWarning(v); break;
      case 'f-text': t.action.text = v; break;
      case 'f-delay': t.action.charDelayMs = +v; break;
      case 'f-media': t.action.key = v; break;
      case 'page-name': page().name = v; break;
      default: return;
    }
    TG.markDirty();
    refreshPreview();
    if (e.target.id === 'f-text' || e.target.id === 'f-delay') $('text-note').innerHTML = textNote(t.action);
  };

  // "Record combo": modifiers + one key, converted to config key names.
  document.addEventListener('keydown', (e) => {
    if (!recording) return;
    if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) return;   // wait for the non-modifier key
    e.preventDefault();
    const name = TG.keyNameFromCode(e.code);
    if (!name) return TG.toast(`${e.code} isn't supported`, 'var(--error)');
    const keys = [];
    if (e.ctrlKey) keys.push('CTRL');
    if (e.shiftKey) keys.push('SHIFT');
    if (e.altKey) keys.push('ALT');
    if (e.metaKey) keys.push('GUI');
    tile().action.keys = keys.concat([name]);
    recording = false;
    TG.markDirty();
    TG.render();
  });

  // Keyboard: arrow keys move the focused tile one cell, if the new spot is free.
  document.addEventListener('keydown', (e) => {
    if (recording) return;
    const el = document.activeElement && document.activeElement.closest && document.activeElement.closest('.screen .tile');
    const step = { ArrowLeft: [-1, 0], ArrowRight: [1, 0], ArrowUp: [0, -1], ArrowDown: [0, 1] }[e.key];
    if (!el || !step) return;
    e.preventDefault();
    const t = page().tiles.find((o) => o.id === el.dataset.tile);
    const spot = { x: t.x + step[0], y: t.y + step[1], w: t.w, h: t.h };
    const err = TG.placeError(spot, page(), density(), t);
    if (err) return TG.toast(err, 'var(--error)');
    Object.assign(t, spot);
    st().tile = t.id;
    TG.markDirty();
    TG.render();
    const moved = document.querySelector(`.screen .tile[data-tile="${CSS.escape(t.id)}"]`);
    if (moved) moved.focus();
  });

  // ---- drag and drop -------------------------------------------------------------------
  // Three kinds of drag, all snapping to the grid of the current density:
  //   move   — pick up a tile on the screen; drop it on the tray to delete it
  //   new    — drag a template out of the tray
  //   resize — drag the white corner handle of the selected tile (1×1 … 2×2)
  // A press that moves less than 5 px is a normal click (select / edit).
  // Pointer events (not HTML5 drag) so touch works too.
  let drag = null, suppressClick = false;

  function screenGeom() {
    const scr = document.querySelector('.screen'), r = scr.getBoundingClientRect();
    return { scr, r, s: r.width / 480, g: TG.GRID[density()] };
  }
  const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

  document.addEventListener('pointerdown', (e) => {
    if (e.button !== 0 || st().view !== 'layout') return;
    const el = e.target.closest('.screen .tile, .tray-item, .resize');
    if (!el) return;
    drag = { el, x0: e.clientX, y0: e.clientY, started: false,
             kind: el.dataset.resize ? 'resize' : el.dataset.template ? 'new' : 'move' };
    if (drag.kind === 'move') drag.tile = page().tiles.find((t) => t.id === el.dataset.tile);
    if (drag.kind === 'resize') drag.tile = page().tiles.find((t) => t.id === el.dataset.resize);
    if (drag.kind === 'new') drag.tile = { ...TEMPLATES[+el.dataset.template], id: 'ghost', x: 0, y: 0, w: 1, h: 1 };
  });

  function startDrag() {
    const { scr, s, g } = screenGeom();
    drag.started = true;
    drag.spot = null;
    document.body.classList.add('dragging');
    drag.ind = document.createElement('div');
    drag.ind.className = 'drop-ind';
    drag.ind.hidden = true;
    scr.appendChild(drag.ind);
    if (drag.kind === 'resize') return;
    // The ghost is the real tile, drawn at the preview's scale, following the pointer.
    const t = drag.tile, size = tileRect({ ...t, x: 0, y: 0 }, g);
    const box = drag.kind === 'move' ? drag.el.getBoundingClientRect() : null;
    drag.off = box ? { x: drag.x0 - box.left, y: drag.y0 - box.top } : { x: size.width * s / 2, y: size.height * s / 2 };
    drag.ghost = document.createElement('div');
    drag.ghost.className = 'ghost ' + density();
    drag.ghost.innerHTML = tileHTML(t, { left: 0, top: 0, width: size.width, height: size.height });
    document.body.appendChild(drag.ghost);
    if (drag.kind === 'move') { drag.el.classList.add('drag-src'); $('tray').classList.add('trash'); }
  }

  document.addEventListener('pointermove', (e) => {
    if (!drag) return;
    if (!drag.started) {
      if (Math.hypot(e.clientX - drag.x0, e.clientY - drag.y0) < 5) return;
      startDrag();
    }
    e.preventDefault();
    const { r, s, g } = screenGeom(), t = drag.tile;
    const over = e.clientX >= r.left && e.clientX <= r.right && e.clientY >= r.top && e.clientY <= r.bottom;
    let spot = null;
    if (drag.kind === 'resize') {
      // The cell under the pointer sets the far corner; spans are limited to 1–2 cells.
      const col = Math.floor(((e.clientX - r.left) / s - g.padX) / (g.w + g.gap));
      const row = Math.floor(((e.clientY - r.top) / s - TG.STATUS_H - g.padY) / (g.h + g.gap));
      spot = { x: t.x, y: t.y, w: clamp(col - t.x + 1, 1, 2), h: clamp(row - t.y + 1, 1, 2) };
    } else {
      const gx = e.clientX - drag.off.x, gy = e.clientY - drag.off.y;
      drag.ghost.style.transform = `translate(${gx}px, ${gy}px) scale(${s})`;
      if (over) {
        // Snap the ghost's top-left corner to the nearest cell, kept inside the grid.
        const lx = (gx - r.left) / s, ly = (gy - r.top) / s;
        spot = { w: t.w, h: t.h,
                 x: clamp(Math.round((lx - g.padX) / (g.w + g.gap)), 0, g.cols - t.w),
                 y: clamp(Math.round((ly - TG.STATUS_H - g.padY) / (g.h + g.gap)), 0, g.rows - t.h) };
      }
      if (drag.kind === 'move') {
        const tr = $('tray').getBoundingClientRect();
        drag.overTrash = e.clientX >= tr.left && e.clientX <= tr.right && e.clientY >= tr.top && e.clientY <= tr.bottom;
        $('tray').classList.toggle('hot', drag.overTrash);
      }
    }
    drag.spot = spot;
    drag.ind.hidden = !spot;
    if (spot) {
      drag.err = TG.placeError(spot, page(), density(), drag.kind === 'new' ? null : t);
      drag.ind.classList.toggle('bad', !!drag.err);
      if (drag.ghost) drag.ghost.classList.toggle('bad', !!drag.err);
      drag.ind.style.cssText = px(tileRect(spot, g));
    }
  });

  function endDrag(commit) {
    const d = drag;
    drag = null;
    if (!d || !d.started) return;
    suppressClick = true;
    setTimeout(() => { suppressClick = false; }, 0);
    document.body.classList.remove('dragging');
    if (d.ghost) d.ghost.remove();
    if (d.ind) d.ind.remove();
    if (commit) {
      if (d.kind === 'move' && d.overTrash) {
        page().tiles = page().tiles.filter((t) => t !== d.tile);
        if (st().tile === d.tile.id) st().tile = null;
        TG.markDirty();
        TG.toast(`Deleted “${d.tile.label}”`, 'var(--warn)');
      } else if (d.spot && d.err) {
        TG.toast(`Can't place it there. ${d.err}`, 'var(--error)');
      } else if (d.spot) {
        if (d.kind === 'new') {
          if (page().tiles.length >= TG.LIMITS.tiles) {
            TG.toast('A page has at most 24 tiles.', 'var(--error)');
          } else {
            const t = newTile(d.tile, d.spot);
            page().tiles.push(t);
            st().tile = t.id;
          }
        } else {
          Object.assign(d.tile, d.spot);
          st().tile = d.tile.id;
        }
        TG.markDirty();
      }
    }
    TG.render();
  }
  document.addEventListener('pointerup', () => endDrag(true));
  document.addEventListener('pointercancel', () => endDrag(false));
  // A drag ends with a click on whatever is under the pointer; swallow it.
  document.addEventListener('click', (e) => { if (suppressClick) { e.stopImmediatePropagation(); e.preventDefault(); } }, true);

  window.addEventListener('resize', fitDevice);
})(globalThis.TG = globalThis.TG || {});
