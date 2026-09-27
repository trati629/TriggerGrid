// make_default_config.js — write data/config.json from web/lib/default-layout.js.
//   node tools/make_default_config.js
// Then `pio run -t uploadfs` puts it on the pad. web/test.html has a button
// that downloads the same file.
'use strict';

const fs = require('fs');
const path = require('path');
const { ROOT, loadTG } = require('./web_lib');

const TG = loadTG();
const editor = TG.defaultEditor();
const problems = TG.saveProblems(editor);
if (problems.length) {
  console.error('The default layout has problems:\n  ' + problems.join('\n  '));
  process.exit(1);
}

const out = path.join(ROOT, 'data', 'config.json');
fs.writeFileSync(out, TG.savedFile(editor) + '\n');
console.log(`wrote ${path.relative(ROOT, out)} (${fs.statSync(out).size} bytes)`);
