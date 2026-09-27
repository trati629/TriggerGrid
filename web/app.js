// app.js — the web config page served by the pad.
//
// Thin device, smart browser: this page edits the human-readable `editor`
// section, compiles it into the `pad` section with web/lib/compile.js, and
// uploads the whole file. The device only stores it and checks bounds.
// Opened from disk (file://), it talks to an in-browser mock device instead.
(function (TG) {
  'use strict';

  const $ = (id) => document.getElementById(id);
  const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
  const svg = (name, cls = 'i') => `<svg class="${cls}" viewBox="0 0 20 20">${TG.ICON_SVG[name] || ''}</svg>`;

  // ---- talking to the device ----------------------------------------------------

  const MOCK = location.protocol === 'file:';
  let pin = '';
  try { pin = sessionStorage.getItem('tg-pin') || ''; } catch (e) { /* private mode */ }

  // One API call. Resolves to { status, body } (body parsed as JSON if any).
  // On 401 it asks for the PIN once and retries.
  async function api(method, path, body, retried) {
    const headers = pin ? { 'X-TG-Pin': pin } : {};
    let res;
    if (MOCK) {
      res = TG.mockDevice(method, path, body, headers);
    } else {
      const r = await fetch(path, { method, headers: body ? { ...headers, 'Content-Type': 'application/json' } : headers, body });
      const text = await r.text();
      let parsed = null;
      try { parsed = text ? JSON.parse(text) : null; } catch (e) { parsed = { error: text }; }
      res = { status: r.status, body: parsed };
    }
    if (res.status === 401 && !retried) {
      const entered = window.prompt('This pad has a web PIN. Enter it to continue:');
      if (entered) {
        pin = entered;
        try { sessionStorage.setItem('tg-pin', pin); } catch (e) { /* private mode */ }
        return api(method, path, body, true);
      }
    }
    return res;
  }
  const errorOf = (res) => (res.body && res.body.error) || ('HTTP ' + res.status);

  // ---- state ----------------------------------------------------------------------

  const state = {
    editor: TG.defaultEditor(),   // replaced by the device's file once loaded
    status: null,                 // last /api/status
    view: location.hash === '#settings' ? 'device' : TG.defaultView || 'json',
    page: 0,
    tile: null,
    dirty: false,
    networks: null,
    jsonError: '',
  };
  TG.state = state;
  TG.api = api;

  function markDirty() {
    state.dirty = true;
    $('dirty').classList.add('on');
    $('save').disabled = false;
  }
  TG.markDirty = markDirty;

  let toastTimer;
  function toast(msg, color = 'var(--ok)') {
    $('toast-msg').textContent = msg;
    $('toast').querySelector('.dot').style.background = color;
    $('toast').classList.add('on');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => $('toast').classList.remove('on'), 2600);
  }
  TG.toast = toast;

  // ---- load, save, status -------------------------------------------------------

  async function loadConfig() {
    const res = await api('GET', '/api/config');
    if (res.status === 200 && res.body && res.body.editor) {
      state.editor = res.body.editor;
    } else if (res.status !== 404) {
      toast('Could not read the layout: ' + errorOf(res), 'var(--error)');
    }
    // 404: a new pad; start from the default layout.
    render();
  }

  async function save() {
    const problems = TG.saveProblems(state.editor);
    if (problems.length) {
      toast(`Can't save. ${problems[0]}${problems.length > 1 ? ` (+${problems.length - 1} more)` : ''}`, 'var(--error)');
      return;
    }
    const file = TG.savedFile(state.editor);
    const res = await api('PUT', '/api/config', file);
    if (res.status === 204) {
      state.dirty = false;
      $('dirty').classList.remove('on');
      $('save').disabled = true;
      toast(`Saved ${(file.length / 1024).toFixed(1)} KB. The pad has updated.`);
    } else {
      toast('The pad refused the layout: ' + errorOf(res), 'var(--error)');
    }
  }

  function renderChips() {
    const s = state.status;
    if (!s) { $('chips').innerHTML = ''; return; }
    const wifiDot = s.wifi.mode === 'station' && s.wifi.ip ? '' : ' warn';
    const wifi = s.wifi.mode === 'hotspot' ? `Setup hotspot <b>${esc(s.wifi.ssid)}</b>`
      : `Wi-Fi <b>${esc(s.wifi.ssid)}</b> · ${esc(s.wifi.host)}.local`;
    const ble = s.ble.connected ? 'Bluetooth <b>connected</b>' : 'Bluetooth <b>waiting</b>';
    $('chips').innerHTML = `<span class="chip"><span class="dot${wifiDot}"></span>${wifi}</span>` +
      `<span class="chip"><span class="dot${s.ble.connected ? '' : ' warn'}"></span>${ble}</span>` +
      (s.layoutWarning ? `<span class="chip"><span class="dot error"></span>Layout error: ${esc(s.layoutWarning)}</span>` : '');
  }

  // Poll every 5 s while the tab is visible; cheaper than a socket.
  async function pollStatus() {
    if (document.visibilityState === 'visible') {
      const res = await api('GET', '/api/status');
      if (res.status === 200) {
        state.status = res.body;
        renderChips();
        if (state.view === 'device') renderDeviceInfo();
      }
    }
    setTimeout(pollStatus, 5000);
  }

  // ---- sidebar ------------------------------------------------------------------

  function renderSidebar() {
    const pages = state.editor.pages.map((p, i) => `
      <button class="nav-item ${state.view === 'layout' && i === state.page ? 'active' : ''}" data-page="${i}">
        ${esc(p.name)}<span class="count">${p.tiles.length}</span></button>`).join('');
    $('sidebar').innerHTML = `
      ${TG.renderLayout ? `<div><div class="side-title">Pages</div><div class="pages-list">${pages}
        <button class="nav-item add" id="add-page">${svg('none')}+ Add page</button></div></div>` : ''}
      <div><div class="side-title">Device</div>
        <button class="nav-item ${state.view === 'device' ? 'active' : ''}" data-view="device">Settings</button>
        <button class="nav-item ${state.view === 'json' ? 'active' : ''}" data-view="json">config.json</button>
      </div>`;
  }

  // ---- config.json view: edit the editor section as text ---------------------------

  function renderJson() {
    const problems = TG.saveProblems(state.editor);
    const size = new TextEncoder().encode(TG.savedFile(state.editor)).length;
    $('stage').innerHTML = `
      <div class="stage-head" style="max-width:760px"><h1 class="page-name" style="border:0;margin:0;padding:0">config.json</h1></div>
      <p class="note" style="max-width:760px;margin:0">This is the <b>editor</b> section. Saving compiles it into the <b>pad</b>
        section, the only part the device reads. ${(size / 1024).toFixed(1)} KB of the 32 KB limit.</p>
      <textarea class="json-edit" id="json-edit" spellcheck="false">${esc(JSON.stringify(state.editor, null, 2))}</textarea>
      <ul class="problems" id="problems">${state.jsonError ? `<li>${esc(state.jsonError)}</li>` : ''}
        ${problems.map((p) => `<li>${esc(p)}</li>`).join('')}</ul>
      <div class="row"><button class="btn small" id="reset-default">Start from the default layout</button></div>`;
  }

  // ---- device settings view ----------------------------------------------------------

  function renderDeviceInfo() {
    const s = state.status, el = $('device-info');
    if (!s || !el) return;
    const up = s.uptime, h = Math.floor(up / 3600), m = Math.floor(up / 60) % 60;
    el.innerHTML = `<dt>Firmware</dt><dd>${esc(s.fw)}</dd><dt>Uptime</dt><dd>${h} h ${m} m</dd>
      <dt>Free heap</dt><dd>${Math.round(s.heap / 1024)} KB</dd><dt>Free PSRAM</dt><dd>${(s.psram / 1048576).toFixed(1)} MB</dd>`;
    const w = $('wifi-info');
    if (w) {
      w.innerHTML = `<dt>Mode</dt><dd>${esc(s.wifi.mode)}</dd><dt>Network</dt><dd>${esc(s.wifi.ssid)}</dd>
        <dt>Address</dt><dd>${esc(s.wifi.host)}.local</dd><dt>IP</dt><dd>${esc(s.wifi.ip || '—')}</dd>
        ${s.wifi.rssi ? `<dt>Signal</dt><dd>${s.wifi.rssi} dBm</dd>` : ''}`;
    }
  }

  function renderNetworks() {
    const el = $('networks');
    if (!el) return;
    if (state.networks === 'scanning') { el.innerHTML = '<span class="note">Scanning…</span>'; return; }
    if (!state.networks) { el.innerHTML = ''; return; }
    el.innerHTML = state.networks.map((n) =>
      `<button data-ssid="${esc(n.ssid)}"><span>${esc(n.ssid)}${n.secure ? '' : ' (open)'}</span><span class="note">${n.rssi} dBm</span></button>`).join('')
      || '<span class="note">No networks found.</span>';
  }

  function renderDevice() {
    const s = state.editor.settings, os = TG.hostOS(s);
    $('stage').innerHTML = `
      <div class="stage-head" style="max-width:760px"><h1 class="page-name" style="border:0;margin:0;padding:0">Device settings</h1></div>
      <div class="cards">
        <div class="card"><h3>Display</h3>
          <div class="field"><span class="lbl">Grid density</span>
            <div class="seg full"><button data-density="regular" class="${s.density === 'regular' ? 'on' : ''}">Regular 5×3</button><button data-density="compact" class="${s.density === 'compact' ? 'on' : ''}">Compact 6×4</button></div></div>
          <div class="field"><label for="f-bright">Brightness <span class="swatch-name" id="bright-val">· ${s.brightness}</span></label>
            <input type="range" id="f-bright" min="10" max="255" value="${s.brightness}"></div>
          <div class="field"><label for="f-dim">Dim after</label>
            <select id="f-dim">${[[0, 'Never'], [30, '30 seconds'], [60, '1 minute'], [120, '2 minutes'], [300, '5 minutes']].map(([v, l]) => `<option value="${v}" ${v === s.dimAfterSec ? 'selected' : ''}>${l}</option>`).join('')}</select></div>
          <span class="note">Display settings take effect when you save.</span>
        </div>
        <div class="card"><h3>Wi-Fi</h3>
          <dl class="kv" id="wifi-info"></dl>
          <div class="row"><button class="btn small" id="scan">Find networks</button></div>
          <div class="networks" id="networks"></div>
          <div class="field"><label for="f-ssid">Network name</label><input type="text" id="f-ssid" maxlength="32"></div>
          <div class="field"><label for="f-pass">Password</label><input type="password" id="f-pass" maxlength="64"></div>
          <div class="row"><button class="btn small" id="join">Join and restart</button></div>
          <span class="note">The pad restarts and joins this network. If it can't, it starts its setup hotspot again; the password and QR code are in the device menu on the pad.</span>
        </div>
        <div class="card"><h3>Bluetooth</h3>
          <div class="field"><label for="f-name">Device name</label><input type="text" id="f-name" maxlength="24" value="${esc(s.deviceName)}"></div>
          <div class="field"><label for="f-os">Computer</label>
            <select id="f-os">${Object.entries(TG.HOST_OS).map(([k, o]) => `<option value="${k}" ${k === os ? 'selected' : ''}>${o.name}</option>`).join('')}</select>
            <span class="note">${TG.HOST_OS[os].how}</span></div>
          <div class="field"><label for="f-layout">Computer's keyboard layout</label>
            <select id="f-layout">${Object.entries(TG.LAYOUTS).map(([k, l]) => `<option value="${k}" ${k === s.hostLayout ? 'selected' : ''}>${l.name}</option>`).join('')}</select>
            <span class="note">Characters on this layout are typed as normal keys; anything else uses the computer's Unicode input above.</span></div>
          <div class="row"><button class="btn small danger" id="forget-ble">Forget paired computer</button></div>
          <span class="note">A new device name is used after the pad restarts.</span>
        </div>
        <div class="card"><h3>Security<span class="state" style="color:var(--${state.status && state.status.pin ? 'ok' : 'warn'})">${state.status && state.status.pin ? 'PIN set' : 'No PIN'}</span></h3>
          <span class="note">Anyone on your network can change what this pad types. A PIN protects this page.</span>
          <div class="field"><label for="f-pin-old">Current PIN (if set)</label><input type="password" id="f-pin-old" inputmode="numeric"></div>
          <div class="field"><label for="f-pin-new">New PIN (4–8 digits, empty to remove)</label><input type="password" id="f-pin-new" inputmode="numeric"></div>
          <div class="row"><button class="btn small" id="set-pin">Set PIN</button></div>
        </div>
        <div class="card"><h3>Backup</h3>
          <span class="note">The layout file holds pages, tiles and settings. Wi-Fi passwords and the PIN are never included.</span>
          <div class="row"><button class="btn small" id="export">Export config.json</button><button class="btn small" id="import">Import</button></div>
          <input type="file" id="import-file" accept=".json,application/json" hidden>
        </div>
        <div class="card"><h3>About</h3>
          <dl class="kv" id="device-info"></dl>
          <div class="row"><button class="btn small" id="reboot">Restart the pad</button>${TG.renderUpdateButton ? TG.renderUpdateButton() : ''}</div>
        </div>
      </div>`;
    renderDeviceInfo();
    renderNetworks();
  }

  // ---- render ---------------------------------------------------------------------

  function render() {
    renderSidebar();
    $('main').classList.toggle('no-editor', state.view !== 'layout');
    if (state.view === 'layout' && TG.renderLayout) TG.renderLayout();
    else if (state.view === 'device') renderDevice();
    else renderJson();
  }
  TG.render = render;
  TG.renderSidebar = renderSidebar;

  // ---- events -----------------------------------------------------------------------

  async function scanNetworks() {
    state.networks = 'scanning';
    renderNetworks();
    await api('POST', '/api/wifi/scan');
    for (let i = 0; i < 20; i++) {
      await new Promise((r) => setTimeout(r, 1000));
      const res = await api('GET', '/api/wifi/scan');
      if (res.status === 200 && !res.body.scanning) {
        state.networks = res.body.networks;
        renderNetworks();
        return;
      }
    }
    state.networks = [];
    renderNetworks();
  }

  function exportConfig() {
    const blob = new Blob([TG.savedFile(state.editor)], { type: 'application/json' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'triggergrid-config.json';
    a.click();
    URL.revokeObjectURL(a.href);
  }

  // Import: take the editor section and recompile pad from it (never trust an
  // imported pad section, it may be stale or hand-edited).
  async function importConfig(file) {
    try {
      const parsed = JSON.parse(await file.text());
      if (!parsed.editor || !Array.isArray(parsed.editor.pages)) throw new Error('no editor section');
      state.editor = parsed.editor;
      state.page = 0;
      state.tile = null;
      markDirty();
      render();
      toast('Imported. Check it, then Save to device.');
    } catch (e) {
      toast('Not a TriggerGrid config: ' + e.message, 'var(--error)');
    }
  }

  document.addEventListener('click', async (e) => {
    const el = e.target.closest('button');
    if (!el) return;
    const d = el.dataset, s = state.editor.settings;
    if (el.id === 'save') return save();
    if (d.view) { state.view = d.view; return render(); }
    if (d.density) { s.density = d.density; markDirty(); return render(); }
    if (d.ssid) { $('f-ssid').value = d.ssid; $('f-pass').focus(); return; }
    if (el.id === 'scan') return scanNetworks();
    if (el.id === 'join') {
      const ssid = $('f-ssid').value.trim();
      if (!ssid) return toast('Enter a network name.', 'var(--error)');
      const res = await api('POST', '/api/wifi', JSON.stringify({ ssid, password: $('f-pass').value }));
      return res.status === 204
        ? toast(`Restarting to join ${ssid}. Reconnect to that network, then open http://${state.status ? state.status.wifi.host : 'triggergrid'}.local`, 'var(--warn)')
        : toast(errorOf(res), 'var(--error)');
    }
    if (el.id === 'forget-ble') {
      const res = await api('POST', '/api/ble/forget');
      return toast(res.status === 204 ? 'Bluetooth pairing cleared.' : errorOf(res), res.status === 204 ? 'var(--warn)' : 'var(--error)');
    }
    if (el.id === 'set-pin') {
      const res = await api('POST', '/api/pin', JSON.stringify({ old: $('f-pin-old').value, new: $('f-pin-new').value }));
      if (res.status === 204) {
        pin = $('f-pin-new').value;
        try { sessionStorage.setItem('tg-pin', pin); } catch (err) { /* private mode */ }
        toast(pin ? 'PIN set.' : 'PIN removed.');
      } else {
        toast(errorOf(res), 'var(--error)');
      }
      return;
    }
    if (el.id === 'reboot') {
      await api('POST', '/api/reboot');
      return toast('Restarting…', 'var(--warn)');
    }
    if (el.id === 'export') return exportConfig();
    if (el.id === 'import') return $('import-file').click();
    if (el.id === 'reset-default') {
      state.editor = TG.defaultEditor();
      state.jsonError = '';
      markDirty();
      return render();
    }
    if (TG.onClick) TG.onClick(e, el);
  });

  document.addEventListener('change', (e) => {
    if (e.target.id === 'import-file' && e.target.files[0]) importConfig(e.target.files[0]);
  });

  document.addEventListener('input', (e) => {
    const s = state.editor.settings, v = e.target.value;
    switch (e.target.id) {
      case 'f-bright': s.brightness = +v; $('bright-val').textContent = '· ' + v; break;
      case 'f-dim': s.dimAfterSec = +v; break;
      case 'f-name': s.deviceName = v; break;
      case 'f-layout': s.hostLayout = v; break;
      case 'f-os': s.hostOS = v; markDirty(); return render();
      case 'json-edit':
        try {
          const parsed = JSON.parse(v);
          if (!parsed.settings || !Array.isArray(parsed.pages)) throw new Error('needs "settings" and "pages"');
          state.editor = parsed;
          state.jsonError = '';
        } catch (err) {
          state.jsonError = 'Not valid yet: ' + err.message;
        }
        markDirty();
        $('problems').innerHTML = (state.jsonError ? `<li>${esc(state.jsonError)}</li>` : '') +
          TG.saveProblems(state.editor).map((p) => `<li>${esc(p)}</li>`).join('');
        return;
      default:
        if (TG.onInput) TG.onInput(e);
        return;
    }
    markDirty();
  });

  window.addEventListener('beforeunload', (e) => { if (state.dirty) e.preventDefault(); });

  // ---- start --------------------------------------------------------------------------

  function start() {
    render();
    loadConfig();
    pollStatus();
  }
  if (MOCK) {
    // Load the pretend device first (development only; not in the firmware).
    const script = document.createElement('script');
    script.src = 'mock-device.js';
    script.onload = start;
    document.head.appendChild(script);
  } else {
    start();
  }
})(globalThis.TG = globalThis.TG || {});
