/* Local verification: native/WASM comparison, real controls and actual screenshots. */
const {chromium} = require('playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const {pathToFileURL} = require('node:url');
const {execFileSync} = require('node:child_process');
const root = path.resolve('dist');
const mime = {'.html':'text/html','.mjs':'text/javascript','.wasm':'application/wasm','.css':'text/css','.png':'image/png','.jpg':'image/jpeg'};
const server = http.createServer((req, res) => {
  const file = path.resolve(root, '.' + decodeURIComponent(new URL(req.url, 'http://localhost').pathname));
  const target = file === root ? path.join(root, 'index.html') : file;
  if (!target.startsWith(root + path.sep)) { res.writeHead(403).end(); return; }
  fs.readFile(target, (error, data) => {
    if (error) res.writeHead(404).end();
    else { res.setHeader('Content-Type', mime[path.extname(target)] || 'application/octet-stream'); res.end(data); }
  });
});
let browser, checks = 0;
const check = (value, message) => { assert.ok(value, message); checks++; };
const snapshot = page => page.evaluate(() => window.rrView.snapshot);
const capture = (page, name) => page.screenshot({path: `docs/images/${name}.jpg`, type: 'jpeg', quality: 86, fullPage: true});
async function ready(page, url) { await page.goto(url); await page.waitForFunction(() => window.rrView?.snapshot?.artReady); await page.clock.install(); }
// The demo is 800 model steps at 2 steps per 1/60 s tick = 6.7 s.
const DEMO_MS = 7200;

// Same scripted manual thrust as tests/test_rider.c --trace.
const manualThrust = step => step < 500 ? 1 : step < 700 ? 0 : step < 1200 ? -1 : 1;
async function compareWasm() {
  const createRider = (await import(pathToFileURL(path.resolve('dist/physics.mjs')).href)).default;
  const engine = await createRider({wasmBinary: fs.readFileSync('dist/physics.wasm')});
  const rows = fs.readFileSync('build/native-trace.csv', 'utf8').trim().split('\n').slice(1).map(line => line.split(',').map(Number));
  let row = 0, maxError = 0;
  for (let mode = 0; mode < 2; mode++) {
    engine._rr_reset(mode);
    for (let step = 0; step <= 1800; step++) {
      if (step % 100 === 0) {
        const native = rows[row++];
        assert.equal(native[0], mode); assert.equal(native[1], step);
        const n = engine._rr_value(13);
        const values = [...Array(17).keys()].map(f => engine._rr_value(f)).concat([engine._rr_trail_t(n - 1), engine._rr_trail_x(n - 1)]);
        values.forEach((value, i) => {
          const error = Math.abs(value - native[i + 2]);
          maxError = Math.max(maxError, error);
          assert.ok(Number.isFinite(error) && error < 1e-9, `WASM/native mode=${mode} step=${step} column=${i}: ${error}`);
        });
      }
      if (mode === 0) engine._rr_thrust(manualThrust(step));
      engine._rr_step(1);
    }
  }
  check(row === rows.length, 'all native samples checked');
  return {rows: row, columns: rows[0].length - 2, maxError, tolerance: 1e-9};
}

/* A pilot at the keyboard: hold W toward the marker until Oren calls "turn now" (or, without his call, until the brake
 * distance readout reaches the distance left), then hold S until the rocket stops; repeat from there. On a hold it waits. */
async function flyStory(page, choices, shoot) {
  await page.locator('#story').click();
  let key = null, phase = 'burn', s, guard = 0;
  const seen = [], heavy = {};
  const hold = async next => { if (key === next) return; if (key) await page.keyboard.up(key); key = next; if (key) await page.keyboard.down(key); };
  while (guard++ < 20000) {
    s = await snapshot(page);
    if (s.sceneId) {
      await hold(null);
      if (!seen.includes(s.sceneId)) {
        seen.push(s.sceneId);
        const box = page.locator('#story-scene');
        if (['dust', 'star', 'silence'].includes(s.sceneId)) {
          heavy[s.sceneId] = (await box.getAttribute('class')) === 'heavy' && await page.locator('#banner').isHidden() && !s.bannerShown;
        }
        if (shoot) await capture(page, `story-${s.sceneId}`);
        else if (s.sceneId === 'home') await capture(page, 'story-home-turned');
      }
      if (s.sceneId === 'home') break;
      const option = s.sceneId === 'gate' ? choices.gate : s.sceneId === 'star' ? choices.star : 0;
      await page.locator('#story-options button').nth(option).click();
      phase = 'burn';
      continue;
    }
    if (s.paused && !key) { await hold('w'); }
    let step = 17;
    if (s.holdNeed > 0 && Math.abs(s.range) < 0.049) {
      await hold(Math.abs(s.v) > 0.004 ? 's' : null);
      if (!key && s.paused) await page.keyboard.press('p');
      step = key ? 17 : 200;
    } else if (s.drift > 0) {
      await hold(null); step = 100;
      if (s.paused) await page.keyboard.press('p');
    } else if (phase === 'burn') {
      if (s.range * s.v < 0 && Math.abs(s.v) > 0.002) phase = 'brake';
      else if (s.call ? s.cue === 1 : s.range * s.v > 0 && s.brakeRead >= Math.abs(s.range)) phase = 'brake';
      await hold(phase === 'burn' ? 'w' : 's');
      if (phase === 'burn' && s.brake < 0.4 * Math.abs(s.range) && !s.call) step = 50;
    } else {
      if (Math.abs(s.v) < 1e-9) { phase = 'burn'; await hold('w'); } else await hold('s');
    }
    await page.clock.runFor(step);
  }
  await hold(null);
  check(s.sceneId === 'home' && s.done, `story reaches home (${seen.join(' > ')})`);
  check(seen.join(',') === 'pad,gate,depot,dust,star,silence,home', 'scenes in the designed order');
  check(heavy.dust && heavy.star && heavy.silence, 'heavy scenes: dark panel, no banner');
  check(s.heavyLog.every(row => !row.bannerShown), 'no banner on screen when a heavy scene opens');
  const ended = await page.locator('#story-text').textContent();
  check(ended.includes(choices.star === 0 ? 'The window lamp is not.' : 'You come in on fumes'), 'the ending of the chosen path');
  check(s.t > s.tau && Math.abs(s.gap - (s.t - s.tau)) < 1e-9, 'Oren has aged more than Vela');
  return {ending: s.ending, ended, tau: s.tau, t: s.t, failures: s.failures, red: s.red, steps: guard};
}

async function run() {
  const started = performance.now(), equivalence = await compareWasm();
  fs.mkdirSync('docs/images', {recursive: true});
  await new Promise((resolve, reject) => { server.once('error', reject); server.listen(0, '127.0.0.1', resolve); });
  const origin = `http://127.0.0.1:${server.address().port}`;
  browser = await chromium.launch({headless: true, args: ['--no-sandbox', '--enable-unsafe-swiftshader']});
  const errors = [], external = [];
  const context = await browser.newContext({viewport: {width: 1440, height: 1050}, deviceScaleFactor: 1});
  const track = page => {
    page.on('pageerror', error => errors.push(error.message));
    page.on('console', msg => { if (msg.type() === 'error') errors.push(msg.text()); });
    page.on('request', req => { if (!req.url().startsWith(origin) && !req.url().startsWith('data:')) external.push(req.url()); });
  };
  // Before: the original terminal frame, captured as text.
  const before = await context.newPage();
  const terminal = execFileSync('./relativity-rider', ['--snapshot'], {encoding: 'utf8'});
  await before.setContent('<html lang="en"><body style="margin:0;background:#080a18;color:#e6e6e6;padding:40px"><p style="font:16px monospace">Original terminal output / ./relativity-rider --snapshot</p><pre style="font:16px/1.3 monospace"></pre></body></html>');
  await before.locator('pre').evaluate((el, text) => el.textContent = text, terminal);
  await capture(before, 'before-terminal'); await before.close();

  const page = await context.newPage(); track(page); await ready(page, origin + '/');
  const initial = await snapshot(page);
  check(initial.paused && initial.steps === 0 && initial.x === 0 && initial.v === 0, 'starts paused at rest on Earth');
  check(initial.dest === 4.2 && Math.abs(initial.dtau - initial.halfTau / 400) < 1e-15, 'C reports the star and the step size');
  await capture(page, 'observatory');
  // Holding W accelerates in C; releasing coasts; S brakes; P freezes.
  await page.keyboard.down('w'); await page.clock.runFor(2000);
  let s = await snapshot(page);
  check(s.thrust === 1 && !s.paused && s.v > 0.3 && s.t > s.tau, `holding W burns at 1 g (v ${s.v})`);
  await page.keyboard.up('w'); const released = await snapshot(page);
  check(released.thrust === 0, 'releasing W cuts the engine');
  await page.clock.runFor(1000); s = await snapshot(page);
  check(Math.abs(s.v - released.v) < 1e-12 && s.x > released.x, 'coasting keeps the speed');
  check(s.gamma > 1 && Math.abs(s.gamma - 1 / Math.sqrt(1 - s.v * s.v)) < 1e-9, 'gamma = 1/sqrt(1 - v^2)');
  await page.keyboard.down('w'); await page.clock.runFor(1500); await page.keyboard.up('w');
  await page.keyboard.press('p'); await capture(page, 'burning'); await page.keyboard.press('p');
  await page.keyboard.press('p'); const frozenCheck = await snapshot(page); await page.clock.runFor(800);
  check(frozenCheck.paused && (await snapshot(page)).tau === frozenCheck.tau, 'pause freezes the C state');
  await page.keyboard.press('p');
  await page.keyboard.down('s'); await page.clock.runFor(1500); s = await snapshot(page); await page.keyboard.up('s');
  check(s.thrust === -1 && s.v < frozenCheck.v, 'holding S brakes');
  // Flying past the star is reported, not counted as arrival.
  await page.keyboard.down('w'); await page.clock.runFor(4000); await page.keyboard.up('w');
  s = await snapshot(page);
  check(s.overshot && !s.arrived && await page.locator('#goal').isHidden(), `overshoot is not arrival (x ${s.x})`);
  check(await page.locator('#status').textContent() === 'Past the star · decelerate to turn back', 'overshoot status shown');
  // R returns to Earth.
  await page.keyboard.press('r'); s = await snapshot(page);
  check(s.steps === 0 && s.x === 0 && s.paused, 'R resets to Earth');

  // The scripted 1 g flip-and-burn trip arrives at rest on the star.
  await page.getByRole('button', {name: '1 g flip-and-burn trip'}).click();
  check(await page.locator('#accel').isDisabled(), 'demo locks the engine');
  await page.clock.runFor(DEMO_MS / 2 - 800);
  await page.keyboard.press('p'); s = await snapshot(page);
  check(s.thrust === 1 && s.v > 0.9, `demo near the flip at ${s.v} c`);
  await capture(page, 'demo-flip'); await page.keyboard.press('p');
  await page.clock.runFor(DEMO_MS);
  const demo = await snapshot(page);
  check(demo.done && demo.arrived && demo.paused, 'demo arrives');
  check(Math.abs(demo.x - 4.2) < 1e-9 && Math.abs(demo.v) < 1e-9, `demo stops on the star (x ${demo.x}, v ${demo.v})`);
  check(demo.tau.toFixed(2) === '3.53' && demo.t.toFixed(2) === '5.82', `demo clocks ${demo.tau} / ${demo.t} yr`);
  check(demo.maxV < 1, 'top speed below c');
  check(await page.locator('#goal').isVisible(), 'arrival banner shown');
  check(await page.locator('#age-rider').textContent() === '3.53 yr' && await page.locator('#age-twin').textContent() === '5.82 yr', 'clock cards show both ages');
  await capture(page, 'arrived');
  check(await page.locator('body').evaluate(el => el.scrollWidth <= innerWidth), 'desktop has no horizontal overflow');
  await page.getByRole('link', {name: 'How it works'}).click();
  check(page.url().endsWith('#field-notes'), 'field notes link');

  // A hand-flown trip with the real buttons can also stop on the star.
  const hand = await context.newPage(); track(hand); await ready(hand, origin + '/');
  // Pilot with the real mouse buttons: burn to the half-way point, flip and burn back, then trim at low speed.
  const hold = async id => { await hand.locator(id).hover(); await hand.mouse.down(); };
  await hold('#accel');
  for (let i = 0; i < 600 && (await snapshot(hand)).x < 2.05; i++) await hand.clock.runFor(17);
  await hand.mouse.up(); s = await snapshot(hand);
  check(s.thrust === 0 && s.x > 2 && s.v > 0.9, `mouse hold on Accelerate burns to the half-way point (x ${s.x})`);
  await hold('#decel');
  for (let i = 0; i < 600 && !(await snapshot(hand)).arrived && (await snapshot(hand)).v > 0.03; i++) await hand.clock.runFor(17);
  await hand.mouse.up();
  let trims = 0;
  for (let i = 0; i < 1500 && !(s = await snapshot(hand)).arrived; i++) {
    const d = s.dest - s.x, want = Math.max(-0.04, Math.min(0.04, 0.8 * d));
    if (s.v < want - 0.01) { await hold('#accel'); await hand.clock.runFor(17); await hand.mouse.up(); trims++; }
    else if (s.v > want + 0.01) { await hold('#decel'); await hand.clock.runFor(17); await hand.mouse.up(); trims++; }
    else await hand.clock.runFor(Math.abs(d) > 0.1 ? 150 : 17);
  }
  check(s.arrived && Math.abs(s.x - 4.2) < 0.05 && Math.abs(s.v) < 0.05, `hand-flown trip arrives (x ${s.x}, v ${s.v}, trims ${trims})`);
  const handTrip = {tau: s.tau, t: s.t, x: s.x, v: s.v, trims};
  await hand.close();

  // The Long Echo: fly the whole story twice with the real keys (W burn, S brake), once down each path of the choices.
  const story = {};
  for (const [name, choices] of [['waited', {gate: 0, star: 0}], ['turned', {gate: 1, star: 1}]]) {
    const pg = await context.newPage(); track(pg); await ready(pg, origin + '/');
    story[name] = await flyStory(pg, choices, name === 'waited');
    await pg.close();
  }
  check(story.waited.ending === 0 && story.turned.ending === 1, 'both endings reached with the real controls');
  check(story.waited.ended !== story.turned.ended, 'the two endings read differently');
  // Japanese: the same scene in the other language, switched from the top bar.
  {
    const pg = await context.newPage(); track(pg); await ready(pg, origin + '/');
    await pg.locator('#story').click(); await pg.locator('#lang').click();
    s = await snapshot(pg);
    check(s.lang === 'ja' && s.sceneId === 'pad' && s.sceneText.includes('母さんの時計'), 'the pad scene in Japanese');
    check((await pg.locator('#story-options button').first().textContent()) === '発進する', 'the answer in Japanese');
    await capture(pg, 'story-ja'); await pg.close();
  }

  const mobileContext = await browser.newContext({viewport: {width: 390, height: 844}, deviceScaleFactor: 1, isMobile: true, hasTouch: true});
  const mobile = await mobileContext.newPage(); track(mobile); await ready(mobile, origin + '/');
  await mobile.locator('#accel').scrollIntoViewIfNeeded(); const box = await mobile.locator('#accel').boundingBox();
  const cdp = await mobileContext.newCDPSession(mobile);
  const touch = (type, points) => cdp.send('Input.dispatchTouchEvent', {type, touchPoints: points});
  await touch('touchStart', [{x: box.x + box.width / 2, y: box.y + box.height / 2}]);
  await mobile.clock.runFor(1500);
  s = await snapshot(mobile);
  check(s.thrust === 1 && s.v > 0.2, `touch hold reaches C (v ${s.v})`);
  await touch('touchEnd', []);
  await mobile.clock.runFor(100);
  check((await snapshot(mobile)).thrust === 0, 'lifting the finger cuts the engine');
  await mobile.getByRole('button', {name: 'Pause'}).tap();
  check(await mobile.locator('body').evaluate(el => el.scrollWidth <= innerWidth), '390px has no overflow');
  for (const el of await mobile.locator('button').all()) {
    const rect = await el.boundingBox(); check(rect.width >= 44 && rect.height >= 44, 'buttons have 44px touch target');
  }
  // Phone legibility: every visible text at least 14 CSS px (DOM and canvas), in portrait and landscape.
  const sizes = () => {
    const out = {dom: 99, where: '', canvas: 99, gate: Boolean(window.__tmGate)};
    const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
    for (let node; (node = walker.nextNode());) {
      if (!node.textContent.trim()) continue;
      const el = node.parentElement, style = getComputedStyle(el), box = el.getBoundingClientRect();
      if (style.display === 'none' || style.visibility === 'hidden' || !box.width || !box.height || el.closest('[hidden]')) continue;
      const px = parseFloat(style.fontSize);
      if (px < out.dom) { out.dom = px; out.where = el.className || el.id || el.tagName; }
    }
    for (const row of window.__tmGate.texts()) out.canvas = Math.min(out.canvas, row.px);
    return out;
  };
  let size = await mobile.evaluate(sizes);
  check(size.dom >= 14, `390x844: smallest DOM text ${size.dom}px (${size.where})`);
  check(size.canvas >= 14, `390x844: smallest canvas text ${size.canvas}px`);
  check(await mobile.evaluate(() => window.__tmGate.goal()), 'the hook reports the first burn');
  const thrustBox = await mobile.locator('#accel').boundingBox(), sceneBox = await mobile.locator('#scene').boundingBox();
  await mobile.evaluate(() => document.getElementById('scene').scrollIntoView({block: 'start'}));
  const pinned = await mobile.locator('#accel').boundingBox();
  check(pinned.y + pinned.height <= 844 && thrustBox.width >= 150 && sceneBox.width > 300, 'engine buttons stay on screen beside the diagram');
  await mobile.evaluate(() => scrollTo(0, 0)); await capture(mobile, 'mobile');
  await mobile.setViewportSize({width: 844, height: 390}); await mobile.clock.runFor(100);
  size = await mobile.evaluate(sizes);
  check(size.dom >= 14 && size.canvas >= 14, `844x390: smallest text DOM ${size.dom}px (${size.where}), canvas ${size.canvas}px`);
  check(await mobile.locator('body').evaluate(el => el.scrollWidth <= innerWidth), '844x390 has no overflow');
  for (const el of await mobile.locator('button').all()) {
    const rect = await el.boundingBox(); check(rect.width >= 44 && rect.height >= 44, 'landscape buttons have 44px touch target');
  }
  // A story scene on the phone: text at least 14 px, answers at least 44 px, in both orientations.
  await mobile.setViewportSize({width: 390, height: 844}); await mobile.clock.runFor(100);
  await mobile.locator('#story').scrollIntoViewIfNeeded(); await mobile.locator('#story').tap();
  await mobile.locator('#story-scene').scrollIntoViewIfNeeded();
  for (const [w, h] of [[390, 844], [844, 390]]) {
    await mobile.setViewportSize({width: w, height: h}); await mobile.clock.runFor(100);
    size = await mobile.evaluate(sizes);
    check(size.dom >= 14 && size.canvas >= 14, `${w}x${h} story: smallest text DOM ${size.dom}px (${size.where}), canvas ${size.canvas}px`);
    for (const el of await mobile.locator('#story-options button, #accel, #decel, #lang').all()) {
      const rect = await el.boundingBox(); check(rect.width >= 44 && rect.height >= 44, `${w}x${h} story controls have 44px targets`);
    }
    check(await mobile.locator('body').evaluate(el => el.scrollWidth <= innerWidth), `${w}x${h} story has no overflow`);
    if (w === 390) { await mobile.locator('#story-scene').scrollIntoViewIfNeeded(); await capture(mobile, 'story-mobile'); }
  }
  await mobileContext.close();

  // Missing art must not block the physics.
  const fallback = await context.newPage(); track(fallback);
  await fallback.route('**/art/**', route => route.fulfill({status: 404, body: ''}));
  await fallback.goto(origin + '/'); await fallback.waitForFunction(() => window.rrView?.snapshot);
  await fallback.clock.install(); await fallback.getByRole('button', {name: '1 g flip-and-burn trip'}).click(); await fallback.clock.runFor(600);
  check((await snapshot(fallback)).steps > 0 && !(await snapshot(fallback)).artReady, 'art failure leaves the model usable');
  await fallback.close();
  const normalErrors = errors.filter(e => !/404/.test(e));
  // Model failure is a visible, noninteractive error state.
  const broken = await context.newPage();
  await broken.route('**/physics.wasm', route => route.fulfill({status: 200, body: ''}));
  await broken.goto(origin + '/'); await broken.getByText('Rocket model unavailable — reload to retry').waitFor();
  check(await broken.locator('#accel').isDisabled() && await broken.locator('#demo').isDisabled(), 'WASM failure disables controls');
  await broken.close();
  check(normalErrors.length === 0, `console/page errors ${normalErrors.join(';')}`);
  check(external.length === 0, 'no external requests');
  const result = {checks, seconds: (performance.now() - started) / 1000, equivalence, errors: normalErrors.length, externalRequests: external.length,
    handTrip, story, demo: {x: demo.x, v: demo.v, tau: demo.tau, t: demo.t, gap: demo.gap, maxV: demo.maxV},
    screenshots: fs.readdirSync('docs/images').filter(file => file.endsWith('.jpg')).sort()};
  fs.writeFileSync('docs/validation.json', JSON.stringify(result, null, 2) + '\n'); console.log(JSON.stringify(result, null, 2));
}
run().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => { if (browser) await browser.close(); server.close(); });
