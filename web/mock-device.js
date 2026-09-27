// mock-device.js — a pretend pad for developing the page from disk.
//
// When web/index.html is opened as a file:// URL there is no device to talk
// to, so app.js loads this and sends its API calls here instead. State lives
// in memory (and localStorage, so a reload keeps your layout). This file is
// never embedded in the firmware (tools/embed_web.py skips it).
(function (TG) {
  'use strict';

  const KEY = 'tg-mock-config';
  let config = null;
  try { config = localStorage.getItem(KEY); } catch (e) { /* private mode */ }
  let pin = '';
  const started = Date.now();

  const json = function (status, body) { return { status: status, body: body }; };

  TG.mockDevice = function (method, path, body, headers) {
    if (pin && headers['X-TG-Pin'] !== pin) return json(401, { error: 'PIN required' });

    if (method === 'GET' && path === '/api/status') {
      return json(200, {
        fw: 'mock', padV: 1, uptime: Math.round((Date.now() - started) / 1000),
        wifi: { mode: 'station', ssid: 'Home-Net', ip: '192.168.1.42', host: 'triggergrid', rssi: -58 },
        ble: { connected: true }, heap: 142000, psram: 7400000, pin: !!pin, layoutWarning: null,
      });
    }
    if (method === 'GET' && path === '/api/config') {
      return config ? json(200, JSON.parse(config)) : json(404, { error: 'no config.json yet' });
    }
    if (method === 'PUT' && path === '/api/config') {
      if (body.length > TG.MAX_FILE) return json(413, { error: 'layout larger than 32 KB' });
      const pad = JSON.parse(body).pad;
      if (!pad || pad.v !== 1) return json(400, { error: 'no pad section' });
      config = body;
      try { localStorage.setItem(KEY, body); } catch (e) { /* private mode */ }
      return json(204, null);
    }
    if (method === 'POST' && path === '/api/test') {
      console.log('mock device: would send', body);
      return json(204, null);
    }
    if (method === 'POST' && path === '/api/wifi/scan') return json(202, { scanning: true });
    if (method === 'GET' && path === '/api/wifi/scan') {
      return json(200, { scanning: false, networks: [
        { ssid: 'Home-Net', rssi: -52, secure: true }, { ssid: 'Neighbour', rssi: -80, secure: true } ] });
    }
    if (method === 'POST' && path === '/api/pin') {
      const b = JSON.parse(body);
      if (pin && b.old !== pin) return json(403, { error: 'current PIN is wrong' });
      pin = b.new;
      return json(204, null);
    }
    if (method === 'POST') return json(204, null);   // wifi, ble/forget, reboot, ota
    return json(404, { error: 'not found' });
  };
})(globalThis.TG = globalThis.TG || {});
