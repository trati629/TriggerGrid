// default-layout.js — the layout a new pad starts with (the editor section).
// After changing it: run `node tools/make_default_config.js` to rewrite
// data/config.json, and update src/config/defaults.cpp (the same layout,
// compiled, built into the firmware). `pio test -e native` checks that the
// two still match.
(function (TG) {
  'use strict';

  TG.defaultEditor = function () {
    return {
      settings: { deviceName: 'TriggerGrid', density: 'regular', brightness: 180, dimAfterSec: 120, hostLayout: 'us', hostOS: 'windows' },
      pages: [
        { id: 'edit', name: 'Editing', tiles: [
          { id: 'copy', x: 0, y: 0, w: 1, h: 1, label: 'Copy', icon: 'copy', color: 'azure', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'C'] } },
          { id: 'paste', x: 1, y: 0, w: 1, h: 1, label: 'Paste', icon: 'paste', color: 'azure', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'V'] } },
          { id: 'sig', x: 2, y: 0, w: 2, h: 1, label: 'Email sig', icon: 'mail', color: 'violet', style: 'soft', action: { type: 'text', text: 'Cheers,\nAlex', charDelayMs: 10 } },
          { id: 'undo', x: 4, y: 0, w: 1, h: 1, label: 'Undo', icon: 'undo', color: 'graphite', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'Z'] } },
          { id: 'play', x: 0, y: 1, w: 2, h: 2, label: 'Play / Pause', icon: 'playpause', color: 'volt', style: 'solid', action: { type: 'media', key: 'PLAY_PAUSE' } },
          { id: 'volup', x: 2, y: 1, w: 1, h: 1, label: 'Vol +', icon: 'volup', color: 'mint', style: 'solid', action: { type: 'media', key: 'VOLUME_UP' } },
          { id: 'mute', x: 3, y: 1, w: 1, h: 1, label: 'Mute', icon: 'mute', color: 'coral', style: 'soft', action: { type: 'media', key: 'MUTE' } },
          { id: 'shot', x: 4, y: 1, w: 1, h: 2, label: 'Screen shot', icon: 'shot', color: 'sun', style: 'solid', action: { type: 'keys', keys: ['GUI', 'SHIFT', 'S'] } },
          { id: 'voldn', x: 2, y: 2, w: 1, h: 1, label: 'Vol -', icon: 'voldown', color: 'mint', style: 'solid', action: { type: 'media', key: 'VOLUME_DOWN' } },
          { id: 'lock', x: 3, y: 2, w: 1, h: 1, label: 'Lock', icon: 'lock', color: 'crimson', style: 'solid', action: { type: 'keys', keys: ['GUI', 'L'] } },
        ] },
        { id: 'media', name: 'Media', tiles: [
          { id: 'prev', x: 0, y: 0, w: 1, h: 1, label: 'Prev', icon: 'prev', color: 'aqua', style: 'soft', action: { type: 'media', key: 'PREV_TRACK' } },
          { id: 'pp', x: 1, y: 0, w: 2, h: 2, label: 'Play / Pause', icon: 'playpause', color: 'magenta', style: 'solid', action: { type: 'media', key: 'PLAY_PAUSE' } },
          { id: 'next', x: 3, y: 0, w: 1, h: 1, label: 'Next', icon: 'next', color: 'aqua', style: 'soft', action: { type: 'media', key: 'NEXT_TRACK' } },
          { id: 'mic', x: 4, y: 0, w: 1, h: 1, label: 'Mic mute', icon: 'mic', color: 'coral', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'SHIFT', 'M'] } },
          { id: 'vdn', x: 0, y: 1, w: 1, h: 1, label: 'Vol -', icon: 'voldown', color: 'graphite', style: 'solid', action: { type: 'media', key: 'VOLUME_DOWN' } },
          { id: 'vup', x: 3, y: 1, w: 1, h: 1, label: 'Vol +', icon: 'volup', color: 'graphite', style: 'solid', action: { type: 'media', key: 'VOLUME_UP' } },
        ] },
        { id: 'code', name: 'Code', tiles: [
          { id: 'term', x: 0, y: 0, w: 2, h: 1, label: 'Terminal', icon: 'terminal', color: 'steel', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'GRAVE'] } },
          { id: 'find', x: 2, y: 0, w: 1, h: 1, label: 'Find', icon: 'search', color: 'tangerine', style: 'soft', action: { type: 'keys', keys: ['CTRL', 'SHIFT', 'F'] } },
          { id: 'build', x: 3, y: 0, w: 2, h: 1, label: 'PIO build', icon: 'code', color: 'volt', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'ALT', 'B'] } },
          { id: 'save', x: 0, y: 1, w: 1, h: 1, label: 'Save', icon: 'save', color: 'violet', style: 'solid', action: { type: 'keys', keys: ['CTRL', 'S'] } },
        ] },
      ],
    };
  };
})(globalThis.TG = globalThis.TG || {});
