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

// The black-hole panel (src/orbit.c): the same scripted runs as `./test_schwarzschild --trace`.
const ORBIT_RUNS = [[0, 20, 4.2], [0, 7, 3.5], [0, 40, 3.2], [1, 0, 6], [1, 0, 5.2]];
async function compareOrbits() {
  const createRider = (await import(pathToFileURL(path.resolve('dist/physics.mjs')).href)).default;
  const engine = await createRider({wasmBinary: fs.readFileSync('dist/physics.wasm')});
  const lines = fs.readFileSync('build/orbit-trace.csv', 'utf8').trim().split('\n');
  const rows = lines.slice(1).map(line => line.split(',').map(Number)), fields = lines[0].split(',').filter(c => /^f\d+$/.test(c)).length;
  let row = 0, maxError = 0;
  ORBIT_RUNS.forEach(([kind, r0, L], run) => {
    engine._so_reset(kind, r0, L);
    for (let step = 0; step <= 3000; step += 300) {
      const native = rows[row++];
      assert.equal(native[0], run); assert.equal(native[1], step);
      const n = engine._so_value(37), a = engine._so_value(18);
      const values = [...Array(fields).keys()].map(f => engine._so_value(f)).concat([engine._so_trail_x(n - 1), engine._so_trail_y(n - 1),
        engine._so_ntrail_x(n - 1), engine._so_ntrail_y(n - 1), engine._so_curve(step % 240), engine._so_ncurve(step % 240), engine._so_apsis(a > 0 ? a - 1 : 0)]);
      values.forEach((value, i) => {
        const error = Math.abs(value - native[i + 2]) / Math.max(1, Math.abs(native[i + 2]));
        maxError = Math.max(maxError, error);
        assert.ok(Number.isFinite(error) && error < 1e-9, `orbit WASM/native run=${run} step=${step} column=${i}: ${error}`);
      });
      engine._so_step(300);
    }
  });
  check(row === rows.length, 'all native orbit samples checked');
  return {rows: row, columns: rows[0].length - 2, maxError, tolerance: 1e-9};
}

// The view from the window (src/aberration.c, src/skyview.c): the same samples as `./test_aberration --trace`.
async function compareSky() {
  const createRider = (await import(pathToFileURL(path.resolve('dist/physics.mjs')).href)).default;
  const engine = await createRider({wasmBinary: fs.readFileSync('dist/physics.wasm')});
  const lines = fs.readFileSync('build/sky-trace.csv', 'utf8').trim().split('\n'), head = lines[0].split(',');
  let maxError = 0, values = 0;
  for (const line of lines.slice(1)) {
    const native = line.split(',').map(Number);
    engine._sv_reset(native[0]);
    head.forEach((name, c) => {
      if (c === 0) return;
      let m;
      const value = (m = /^f(\d+)$/.exec(name)) ? engine._sv_value(+m[1]) : (m = /^s(\d+)_(\d+)$/.exec(name)) ? engine._sv_star(+m[1], +m[2])
        : (m = /^c(\d+)_(\d+)$/.exec(name)) ? engine._sv_curve(+m[1], +m[2]) : NaN;
      const error = Math.abs(value - native[c]) / Math.max(1, Math.abs(native[c]));
      maxError = Math.max(maxError, error); values++;
      assert.ok(Number.isFinite(error) && error < 1e-9, `sky WASM/native beta=${native[0]} ${name}: ${error}`);
    });
  }
  check(values === (lines.length - 1) * (head.length - 1), 'all native sky samples checked');
  return {rows: lines.length - 1, columns: head.length - 1, maxError, tolerance: 1e-9};
}

async function skyWindow(page) {
  const sv = () => page.evaluate(() => window.skyView.snapshot);
  const setRange = (id, value) => page.locator(id).evaluate((el, v) => { el.value = String(v); el.dispatchEvent(new Event('input', {bubbles: true})); }, value);
  await page.locator('#sky').scrollIntoViewIfNeeded();
  let v = await sv();
  check(v.beta === 0.9 && v.stars === 600 && v.starsDrawn === 600, 'sky: default 0.9 c with 600 stars read from C');
  check(Math.abs(v.halfCone - Math.acos(0.9)) < 1e-12 && Math.abs(v.dAhead - Math.sqrt(19)) < 1e-12 && Math.abs(v.dBehind - 1 / Math.sqrt(19)) < 1e-12, 'half cone and D ahead/behind from C');
  check(Math.abs(v.forward - v.forwardExpected) < 4 * Math.sqrt(600 * 0.95 * 0.05), `stars ahead ${v.forward} vs expected ${v.forwardExpected}`);
  check((await page.locator('#sky-cone').textContent()) === '25.8' && (await page.locator('#sky-gamma').textContent()) === '2.294', 'readouts show the half cone and gamma');
  await page.locator('#sky').evaluate(el => el.scrollIntoView({block: 'start'}));
  await page.screenshot({path: 'docs/images/sky.jpg', type: 'jpeg', quality: 86});
  await setRange('#sky-beta', 0.3); v = await sv();
  check(v.beta === 0.3 && Math.abs(v.gamma - 1 / Math.sqrt(1 - 0.09)) < 1e-12, 'the slider reaches C');
  const at09 = (await page.locator('#sky-09').click(), await sv()).bandAhead;
  await page.locator('#sky-099').click(); v = await sv();
  check(v.beta === 0.99 && v.bandAhead < at09 && v.tAhead > 80000 && (await page.locator('#sky-099').getAttribute('aria-pressed')) === 'true',
    `0.99 c: a 5800 K star ahead is ${v.tAhead} K and dimmer in the visible band than at 0.9 c (x${v.bandAhead} < x${at09})`);
  await page.locator('#sky-sweep').click(); await page.clock.runFor(4000); v = await sv();
  check(v.sweeping && v.beta > 0.2 && v.beta < 0.8, `the sweep runs (beta ${v.beta})`);
  await page.clock.runFor(5000); v = await sv();
  check(!v.sweeping && v.beta === 0.99, 'the sweep ends at 0.99 c');
  await page.locator('#sky-09').click();
  for (const width of [1440, 1024]) {
    await page.setViewportSize({width, height: 1050}); await page.clock.runFor(100);
    check(await page.locator('body').evaluate(el => el.scrollWidth <= innerWidth), `${width}px with the sky panel has no horizontal overflow`);
  }
  await page.setViewportSize({width: 1440, height: 1050});
  v = await sv();
  return {beta: v.beta, forward: v.forward, expected: v.forwardExpected, halfConeDeg: v.halfCone * 180 / Math.PI, dAhead: v.dAhead, bandAhead: v.bandAhead};
}

async function blackHole(page) {
  const bh = () => page.evaluate(() => window.bhView.snapshot);
  const setRange = (id, value) => page.locator(id).evaluate((el, v) => { el.value = String(v); el.dispatchEvent(new Event('input', {bubbles: true})); }, value);
  await page.locator('#black-hole').scrollIntoViewIfNeeded();
  let b = await bh();
  check(b.kind === 0 && b.r0 === 20 && Math.abs(b.L - 4.2) < 1e-12 && b.paused && b.steps === 0, 'black hole: default start read from C');
  check(b.bound === 0 && Math.abs(b.ra - 20) < 1e-9 && b.rp > 10.3 && b.rp < 10.4, `bound orbit between ${b.rp} and ${b.ra} M`);
  check(b.isco === 6 && b.photon === 3 && Math.abs(b.bcrit - 3 * Math.sqrt(3)) < 1e-12, 'ISCO, photon sphere and b_c from C');
  await page.locator('#bh-launch').click();
  await page.clock.runFor(30000);
  b = await bh();
  check(!b.paused && b.state === 0 && b.apses >= 6, `the orbit runs (${b.apses} periapses)`);
  check(Math.abs(b.prec - b.precExact) < 1e-6 && b.prec > 2 && b.prec < 2.3, `turn per lap ${b.prec} vs elliptic integral ${b.precExact}`);
  check(b.precWeak < b.precExact, 'the weak-field formula is short in the strong field');
  check(b.trailPoints > 1000 && b.drift < 1e-10 && b.apsesDrawn === b.apses, `trail ${b.trailPoints} points, energy error ${b.drift}`);
  check(b.nbound === 0 && Math.abs(b.nra - 20) < 1e-9, "Newton's orbit from the same start");
  check(b.clockRate < b.staticRate && b.staticRate < 1, 'the orbiting clock runs slower than a static one');
  const rosette = {apses: b.apses, precession: b.prec, precExact: b.precExact, precWeak: b.precWeak, drift: b.drift, trailPoints: b.trailPoints};
  check(b.labelInfo && b.labelInfo.hits === 0 && b.labelInfo.outside === 0 && b.labelInfo.labels > 0, `ring labels clear of the periapsis ticks (${JSON.stringify(b.labelInfo)})`);
  await page.locator('#bh-freeze').click(); const frozen = await bh(); await page.clock.runFor(1000);
  check(frozen.paused && (await bh()).steps === frozen.steps, 'Freeze stops the C integration');
  await page.locator('#black-hole').evaluate(el => el.scrollIntoView({block: 'start'}));
  await page.screenshot({path: 'docs/images/black-hole.jpg', type: 'jpeg', quality: 86});
  // Too little angular momentum: falls in, while Newton's orbit with the same L does not.
  await setRange('#bh-l', 3.3);
  b = await bh();
  check(Math.abs(b.L - 3.3) < 1e-12 && b.bound === 2 && b.preset === null, 'the L slider reaches C: L = 3.3 has no turning point');
  await page.locator('#bh-launch').click(); await page.clock.runFor(20000); b = await bh();
  check(b.state === 1 && b.nstate === 0, 'captured by the hole; the Newtonian twin keeps orbiting');
  await setRange('#bh-r0', 12); b = await bh();
  check(b.r0 === 12 && b.steps === 0, 'the r0 slider reaches C');
  // Light grazing the photon sphere: winds round and escapes, bent by more than a full turn.
  await page.locator('#bh-graze').click(); await page.clock.runFor(15000); b = await bh();
  check(b.kind === 1 && Math.abs(b.L - 5.21) < 1e-12 && b.state === 2, `grazing light escapes (state ${b.state})`);
  check(Math.abs(b.deflect - b.deflectInner) < 1e-3 && b.deflect > Math.PI, `bent by ${b.deflect} rad (quadrature ${b.deflectInner})`);
  const graze = {b: b.L, deflect: b.deflect, deflectInner: b.deflectInner, deflectTotal: b.deflectTotal};
  check(await page.locator('#bh-r0').isDisabled() && (await page.locator('#bh-l-name').textContent()) === 'Impact parameter b', 'light mode relabels the slider');
  await page.locator('#black-hole').evaluate(el => el.scrollIntoView({block: 'start'}));
  await page.screenshot({path: 'docs/images/black-hole-light.jpg', type: 'jpeg', quality: 86});
  await page.locator('#bh-capture').click(); await page.clock.runFor(10000); b = await bh();
  check(b.state === 1 && b.nstate !== 1, 'light with b = 5 is captured; the straight line is not');
  await page.locator('#bh-isco').click(); await page.clock.runFor(5000); b = await bh();
  check(b.kind === 0 && Math.abs(b.rp - 7) < 1e-6 && Math.abs(b.ra - 7) < 1e-6 && Math.abs(b.r - 7) < 1e-6, `circular orbit at 7 M stays (r ${b.r})`);
  await page.locator('#bh-rosette').click(); await page.clock.runFor(8000);
  for (const width of [1440, 1024]) {
    await page.setViewportSize({width, height: 1050}); await page.clock.runFor(100);
    check(await page.locator('body').evaluate(el => el.scrollWidth <= innerWidth), `${width}px with the black-hole panel has no horizontal overflow`);
  }
  await page.setViewportSize({width: 1440, height: 1050});
  return {rosette, grazingLight: graze};
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
  const started = performance.now(), equivalence = await compareWasm(), orbitEquivalence = await compareOrbits(), skyEquivalence = await compareSky();
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
  const blackHoleResult = await blackHole(page);
  const skyResult = await skyWindow(page);

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
  // The black-hole panel on the phone: sliders and buttons 44 px, the orbit drawn, a screenshot.
  await mobile.locator('#bh-launch').scrollIntoViewIfNeeded(); await mobile.locator('#bh-launch').tap(); await mobile.clock.runFor(8000);
  check((await mobile.evaluate(() => window.bhView.snapshot)).apses >= 2, 'black hole runs from a tap');
  for (const id of ['#bh-r0', '#bh-l', '#bh-launch', '#bh-particle', '#bh-light']) {
    const rect = await mobile.locator(id).boundingBox(); check(rect.height >= 44 && rect.width >= 44, `390x844 ${id} is at least 44 px (${rect.width}x${rect.height})`);
  }
  await mobile.locator('#black-hole').evaluate(el => el.scrollIntoView({block: 'start'}));
  await mobile.screenshot({path: 'docs/images/black-hole-mobile.jpg', type: 'jpeg', quality: 86});
  check((await mobile.evaluate(() => window.bhView.snapshot.labelInfo.hits)) === 0, '390x844 ring labels clear of the ticks');
  // The sky panel on the phone: controls 44 px, both discs drawn, a screenshot.
  await mobile.locator('#sky-099').scrollIntoViewIfNeeded(); await mobile.locator('#sky-099').tap();
  check((await mobile.evaluate(() => window.skyView.snapshot.beta)) === 0.99, 'sky preset from a tap');
  for (const id of ['#sky-beta', '#sky-05', '#sky-09', '#sky-099', '#sky-sweep']) {
    const rect = await mobile.locator(id).boundingBox(); check(rect.height >= 44 && rect.width >= 44, `390x844 ${id} is at least 44 px (${rect.width}x${rect.height})`);
  }
  await mobile.locator('#sky-09').tap();
  await mobile.locator('#sky').evaluate(el => el.scrollIntoView({block: 'start'}));
  await mobile.screenshot({path: 'docs/images/sky-mobile.jpg', type: 'jpeg', quality: 86});
  await mobile.setViewportSize({width: 844, height: 390}); await mobile.clock.runFor(100);
  size = await mobile.evaluate(sizes);
  check(size.dom >= 14 && size.canvas >= 14, `844x390: smallest text DOM ${size.dom}px (${size.where}), canvas ${size.canvas}px`);
  for (const id of ['#bh-r0', '#bh-l', '#sky-beta']) {
    const rect = await mobile.locator(id).boundingBox(); check(rect.height >= 44, `844x390 ${id} is at least 44 px high`);
  }
  check(await mobile.locator('body').evaluate(el => el.scrollWidth <= innerWidth), '844x390 has no overflow');
  for (const el of await mobile.locator('button').all()) {
    const rect = await el.boundingBox(); check(rect.width >= 44 && rect.height >= 44, `landscape buttons have 44px touch target (${await el.getAttribute('id')} ${rect.width}x${rect.height})`);
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
  const result = {checks, seconds: (performance.now() - started) / 1000, equivalence, orbitEquivalence, skyEquivalence, blackHole: blackHoleResult, sky: skyResult, errors: normalErrors.length, externalRequests: external.length,
    handTrip, story, demo: {x: demo.x, v: demo.v, tau: demo.tau, t: demo.t, gap: demo.gap, maxV: demo.maxV},
    screenshots: fs.readdirSync('docs/images').filter(file => file.endsWith('.jpg')).sort()};
  fs.writeFileSync('docs/validation.json', JSON.stringify(result, null, 2) + '\n'); console.log(JSON.stringify(result, null, 2));
}
run().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => { if (browser) await browser.close(); server.close(); });
