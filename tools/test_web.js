// test_web.js — run the web/lib tests without a browser: `node tools/test_web.js`.
'use strict';

const { loadTG } = require('./web_lib');

const TG = loadTG(['web/test.js']);
const result = TG.runTests();

for (const f of result.failed) {
  console.log(`FAIL  ${f.name}\n      got      ${f.actual}\n      expected ${f.expected}`);
}
console.log(`${result.passed} passed, ${result.failed.length} failed`);
process.exit(result.failed.length ? 1 : 0);
