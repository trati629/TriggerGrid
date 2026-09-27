// make_fonts.js — regenerate src/ui/fonts/*.c from Space Grotesk.
//   node tools/make_fonts.js
// Downloads the font (SIL Open Font License) into .pio/fonts-src/ if needed,
// then converts it with lv_font_conv at 4 bpp. Glyphs: Latin-1 plus common
// typographic punctuation (– — ‘ ’ “ ” • … − €), so labels typed in the web
// editor show on the pad. Icons still come from LVGL's Montserrat symbols.
'use strict';

const fs = require('fs');
const path = require('path');
const https = require('https');
const { execFileSync } = require('child_process');

const ROOT = path.join(__dirname, '..');
const SRC = path.join(ROOT, '.pio', 'fonts-src');
const OUT = path.join(ROOT, 'src', 'ui', 'fonts');
const BASE = 'https://github.com/floriankarsten/space-grotesk/raw/master/fonts/ttf/static/';
const RANGES = '0x20-0x7E,0xA0-0xFF,0x2013-0x2014,0x2018-0x201D,0x2022,0x2026,0x2212,0x20AC';

// [weight, size] pairs used by src/ui (see fonts.h).
const FONTS = [['Medium', 14], ['Medium', 16], ['Medium', 20], ['Regular', 12], ['Regular', 14]];

function download(url, file) {
  return new Promise((resolve, reject) => {
    https.get(url, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        return download(res.headers.location, file).then(resolve, reject);
      }
      if (res.statusCode !== 200) return reject(new Error(`${url}: HTTP ${res.statusCode}`));
      const out = fs.createWriteStream(file);
      res.pipe(out);
      out.on('finish', () => out.close(resolve));
    }).on('error', reject);
  });
}

async function main() {
  fs.mkdirSync(SRC, { recursive: true });
  fs.mkdirSync(OUT, { recursive: true });
  for (const weight of ['Medium', 'Regular']) {
    const ttf = path.join(SRC, `SpaceGrotesk-${weight}.ttf`);
    if (!fs.existsSync(ttf)) {
      console.log(`downloading SpaceGrotesk-${weight}.ttf`);
      await download(`${BASE}SpaceGrotesk-${weight}.ttf`, ttf);
    }
  }
  for (const [weight, size] of FONTS) {
    const name = `space_grotesk_${weight.toLowerCase()}_${size}`;
    const out = path.join(OUT, `${name}.c`);
    execFileSync('npx', ['--yes', 'lv_font_conv', '--bpp', '4', '--size', String(size), '--no-compress',
      '--format', 'lvgl', '--font', path.join(SRC, `SpaceGrotesk-${weight}.ttf`), '-r', RANGES,
      '--lv-font-name', name, '-o', out], { stdio: 'inherit', shell: process.platform === 'win32' });
    // Keep the header free of local paths.
    const text = fs.readFileSync(out, 'utf8').replace(/--font \S+SpaceGrotesk-/, '--font SpaceGrotesk-')
      .replace(/ -o \S+\.c/, ` -o src/ui/fonts/${name}.c`);
    fs.writeFileSync(out, text);
    console.log(`wrote src/ui/fonts/${name}.c`);
  }
}

main().catch((e) => { console.error(e.message); process.exit(1); });
