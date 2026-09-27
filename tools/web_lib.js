// web_lib.js — loads web/lib/*.js into Node, the same way the browser does
// (plain scripts that attach to globalThis.TG). Used by the other tools.
'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const ROOT = path.join(__dirname, '..');

// Script order matters: compile.js uses the tables from the earlier files.
const LIB_FILES = [
  'web/lib/swatches.js',
  'web/lib/icons.js',
  'web/lib/keymap.js',
  'web/lib/layouts/us.js',
  'web/lib/unicode.js',
  'web/lib/validate.js',
  'web/lib/compile.js',
  'web/lib/default-layout.js',
];

function loadTG(extraFiles = []) {
  const context = { TextEncoder, console };
  context.globalThis = context;
  vm.createContext(context);
  for (const file of LIB_FILES.concat(extraFiles)) {
    vm.runInContext(fs.readFileSync(path.join(ROOT, file), 'utf8'), context, { filename: file });
  }
  return context.TG;
}

module.exports = { ROOT, LIB_FILES, loadTG };
