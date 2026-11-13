import createRider from './physics.mjs';
import {SCENES, UI, NAMES} from './story.mjs';
import {startOrbits} from './orbits.mjs';

const $ = id => document.getElementById(id);
const names = ['tau','t','x','v','gamma','phi','thrust','mode','steps','dest','arrived','halfTau','dtau','trail','done','gap','maxV'];
const scene = $('scene'), plot = $('plot');
const sky = new Image(), rider = new Image(), twin = new Image();
sky.src = 'art/deep-sky.jpg'; rider.src = 'art/rider-portrait.png'; twin.src = 'art/twin-portrait.png';
const STEPS_PER_TICK = 2, TICK = 1 / 60;
const C = {world: '#e69f00', cone: '#56b4e9', star: '#f0e442', now: '#cc79a7', earth: '#009e73', ink: '#c9cfe3'};
const modeNames = ['MANUAL FLIGHT', '1 g FLIP-AND-BURN', 'THE LONG ECHO'];
const notes = [
  'Your engine, your trip. Burn toward the star, flip half way and burn back to stop on it.',
  'Full 1 g toward the star for 1.76 yr of ship time, then full 1 g back. The model is not told where to stop; it arrives because the two halves are symmetric. Controls are locked during the experiment.'
];
let engine, state, paused = true, last = 0, accumulator = 0, samples = [], trail = [], held = 0;
// The story (mode 2): the C trip of src/courier.c. Field names follow enum CourierValue in src/courier.h.
const storyNames = ['x','t','tau','v','gamma','phi','fuel','red','leg','scene','banner','failures','from','target','range','brake',
  'brakeRead','wobble','cue','echo','gate','calibrated','call','turned','ending','loud','heavy','command','redLine','held','holdNeed','leak','drift','gap'];
const MARKERS = [[0.8, {en: 'Gate', ja: 'ゲート'}], [2.0, {en: 'Depot · Midway', ja: '補給所 · 中間ブイ'}]];
const goalHTML = $('control-goal').innerHTML;
let story = false, cs = null, shownScene = -2, storyTrail = [], heavyLog = [];
let lang = new URLSearchParams(location.search).get('lang') === 'ja' ? 'ja' : 'en';
const t$ = key => UI[lang][key];
const readStory = () => Object.fromEntries(storyNames.map((name, i) => [name, engine._cw_value(i)]));
/* The view of the story trip in the observatory's terms: position, both clocks and the engine's direction. */
function storyView() {
  const dir = cs.command === 1 ? Math.sign(cs.range) || 1 : cs.command === 2 && cs.phi ? -Math.sign(cs.phi) : 0;
  return {...cs, mode: 2, dest: 4.2, thrust: cs.drift > 0 || cs.held > 0 || cs.fuel <= 0 ? 0 : dir, done: cs.leg >= 5,
    arrived: cs.leg >= 5, steps: storyTrail.length, trail: storyTrail.length, halfTau: 0, dtau: 0.001, maxV: 0};
}
const read = () => Object.fromEntries(names.map((name, i) => [name, engine._rr_value(i)]));
const overshot = () => state.x > state.dest + 0.05;

function pull() {
  if (story) { cs = readStory(); state = storyView(); trail = storyTrail; return; }
  state = read();
  trail = Array.from({length: state.trail}, (_, i) => [engine._rr_trail_x(i), engine._rr_trail_t(i)]);
}
function sample() { samples.push({tau: state.tau, t: state.t}); }
function reset(mode = 0, run = false) {
  story = mode === 2; held = 0; storyTrail = []; heavyLog = []; shownScene = -2;
  document.body.dataset.mode = story ? 'story' : 'flight';
  if (story) { engine._cw_reset(); cs = readStory(); storyTrail.push([cs.x, cs.t]); }
  else engine._rr_reset(mode);
  pull(); paused = !run; accumulator = 0; samples = []; sample();
  showScene(); render();
}
function setThrust(sign) {
  if (!engine || (state.mode !== 0 && !story) || (story && cs.scene >= 0)) return;
  held = sign; if (!story) engine._rr_thrust(sign); if (sign) burned = true;
  if (sign && paused && !state.done) { paused = false; accumulator = 0; }
  pull(); render();
}
function togglePause() {
  if (!engine || state.done || (story && cs.scene >= 0)) return;
  paused = !paused; accumulator = 0; render();
}
$('pause').onclick = togglePause;
$('reset').onclick = () => reset(0);
$('demo').onclick = () => reset(1, true);
$('story').onclick = () => reset(2, false);
$('lang').onclick = () => { lang = lang === 'en' ? 'ja' : 'en'; shownScene = -2; if (engine) { showScene(); render(); } else applyLang(); };
for (const [id, sign] of [['accel', 1], ['decel', -1]]) {
  const button = $(id);
  button.addEventListener('pointerdown', event => { event.preventDefault(); button.setPointerCapture?.(event.pointerId); setThrust(sign); });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) button.addEventListener(type, () => { if (held === sign) setThrust(0); });
  button.addEventListener('contextmenu', event => event.preventDefault());
}
document.addEventListener('keydown', event => {
  if (!engine || event.ctrlKey || event.metaKey || event.altKey || event.target.matches('input,textarea,select')) return;
  const key = event.key.toLowerCase();
  if (!['w', 's', 'p', 'r'].includes(key)) return;
  event.preventDefault();
  if (event.repeat) return;
  if (key === 'p') return togglePause();
  if (key === 'r') return reset(story ? 2 : 0);
  setThrust(key === 'w' ? 1 : -1);
});
document.addEventListener('keyup', event => {
  const key = event.key.toLowerCase();
  if ((key === 'w' && held === 1) || (key === 's' && held === -1)) setThrust(0);
});
const stop = () => { if (held) setThrust(0); paused = true; accumulator = 0; if (engine) render(); };
document.addEventListener('visibilitychange', () => { if (document.hidden) stop(); });
window.addEventListener('blur', stop);

// On a phone (narrow, short or touch-first screen) every canvas label is at least 14 CSS px.
const phone = matchMedia('(max-width:720px),(max-height:500px),(pointer:coarse)');
const textPx = desktop => phone.matches ? Math.max(14, desktop) : desktop;
let canvasTexts = [];
function canvasSize(canvas) {
  const bounds = canvas.getBoundingClientRect(), dpr = Math.min(devicePixelRatio || 1, 2);
  const w = Math.round(bounds.width * dpr), h = Math.round(bounds.height * dpr);
  if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
  const ctx = canvas.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  return {ctx, w: bounds.width, h: bounds.height};
}
function cover(ctx, img, w, h) {
  const scale = Math.max(w / img.naturalWidth, h / img.naturalHeight);
  ctx.drawImage(img, (w - img.naturalWidth * scale) / 2, (h - img.naturalHeight * scale) / 2, img.naturalWidth * scale, img.naturalHeight * scale);
}
function portrait(ctx, img, x, y, r, ring) {
  ctx.save(); ctx.beginPath(); ctx.arc(x, y, r, 0, 2 * Math.PI); ctx.closePath();
  ctx.fillStyle = '#f4efe2'; ctx.fill();
  if (img.complete && img.naturalWidth) { ctx.clip(); ctx.drawImage(img, x - r, y - r, 2 * r, 2 * r); }
  ctx.restore();
  ctx.strokeStyle = ring; ctx.lineWidth = 3; ctx.beginPath(); ctx.arc(x, y, r, 0, 2 * Math.PI); ctx.stroke();
}
function line(ctx, x0, y0, x1, y1) { ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke(); }

// ------------------------------------------------------------------ the story: scenes and readouts
function applyLang() {
  document.documentElement.lang = lang;
  $('lang').textContent = t$('lang'); $('lang').lang = lang === 'en' ? 'ja' : 'en'; $('lang').setAttribute('aria-label', t$('langLabel'));
  $('story-label').textContent = t$('storyButton'); $('story-eyebrow').textContent = t$('storyEyebrow'); $('story-note').textContent = t$('storyNote');
  for (const [id, key] of [['l-tank', 'tank'], ['l-red', 'red'], ['l-failures', 'failures'], ['l-marker', 'marker'], ['l-brake', 'brakeRead'], ['l-echo', 'echo']]) $(id).textContent = t$(key);
  $('u-red').textContent = t$('yr'); $('u-range').textContent = $('u-brake').textContent = t$('ly');
}
function storyLines(s) {
  if (s.endings) return s.endings[cs.ending === 0 ? 0 : 1].map(line => ['', line]);
  return s.lines.filter(([, , when]) => !when || (when === 'calibrated') === Boolean(cs.calibrated));
}
function showScene() {
  const box = $('story-scene'), index = story && cs ? cs.scene : -1;
  if (index === shownScene) return;
  shownScene = index;
  if (index < 0) { box.hidden = true; return; }
  const s = SCENES[index];
  if (held) setThrust(0);
  box.hidden = false;
  box.className = s.heavy ? 'heavy' : s.endings ? 'ending' : '';
  box.dataset.scene = s.id;
  if (s.heavy) heavyLog.push({id: s.id, banner: cs.banner, loud: cs.loud, bannerShown: !$('banner').hidden});
  $('story-title').textContent = s.title[lang];
  const text = $('story-text');
  text.replaceChildren();
  for (const [who, words] of storyLines(s)) {
    const p = document.createElement('p');
    if (who) { const b = document.createElement('b'); b.textContent = NAMES[lang][who]; b.className = who; p.append(b, ' '); }
    else p.className = 'narration';
    p.append(words[lang]);
    text.append(p);
  }
  if (s.endings) { const p = document.createElement('p'); p.className = 'clocks'; p.textContent = t$('clocks')(cs.tau, cs.t); text.append(p); }
  if (s.note) { const p = document.createElement('p'); p.className = 'note'; p.textContent = s.note[lang]; text.append(p); }
  const options = $('story-options');
  options.replaceChildren();
  s.options.forEach((label, option) => {
    const button = document.createElement('button');
    button.textContent = label[lang];
    if (!option) button.className = 'primary';
    button.onclick = () => answer(option);
    options.append(button);
  });
  if (s.endings) {
    const back = document.createElement('button');
    back.textContent = t$('toObservatory'); back.onclick = () => reset(0);
    options.append(back);
  }
  $('banner').hidden = true;
  box.scrollTop = 0;
  options.firstChild.focus({preventScroll: true});
}
function answer(option) {
  if (!story || cs.scene < 0) return;
  if (SCENES[cs.scene].endings) { reset(2, false); return; }
  engine._cw_answer(option);
  pull(); showScene();
  // A hold on the marker (the Gate's calibration, Kestrel's tanker) runs by itself; otherwise the clock waits for your first burn.
  paused = cs.scene >= 0 || !(cs.holdNeed > 0); accumulator = 0;
  render();
}
function storyStatus() {
  if (cs.leg >= 5) return SCENES[6].title[lang];
  const name = t$('leg')(cs.leg + 1, t$('legs')[cs.leg]);
  if (cs.drift > 0) return `${name} · ${t$('drifting')}`;
  if (cs.held > 0) return `${name} · ${t$('holding')(Math.max(0, cs.holdNeed - cs.held))}`;
  if (cs.cue) return t$('turnNow');
  if (cs.command === 1) return `${name} · ${t$('burning')}`;
  if (cs.command === 2) return `${name} · ${t$('braking')}`;
  if (Math.abs(cs.range) < 0.05 && Math.abs(cs.v) < 0.05) return `${name} · ${t$('onMark')}`;
  return `${name} · ${t$('coasting')(cs.v)}`;
}
function renderStory() {
  $('s-fuel').textContent = cs.fuel.toFixed(2);
  $('fuel-row').dataset.red = String(cs.fuel < cs.redLine);
  $('fuel-bar').style.width = `${Math.min(100, cs.fuel / 8 * 100)}%`;
  $('s-red').textContent = cs.red.toFixed(2);
  $('s-failures').textContent = String(cs.failures);
  $('s-range').textContent = Math.abs(cs.range).toFixed(2);
  $('s-brake').textContent = cs.brakeRead.toFixed(2);
  $('s-echo').textContent = !cs.echo ? (cs.leg < 4 ? '—' : t$('echoOff')) : cs.call ? t$('echoOn') : t$('echoDust');
  const banner = $('banner');
  banner.textContent = t$('banners')[cs.banner] || '';
  banner.dataset.kind = cs.banner >= 2 ? 'fail' : 'ok';
  // A scene on screen always hides the banner; the C trip also clears it when a heavy scene opens.
  banner.hidden = !cs.banner || cs.scene >= 0;
  document.querySelector('.scene-label').hidden = cs.scene >= 0;
}

/* Spacetime diagram in the Earth frame: x in light-years across, Earth time t in years upwards. */
function drawScene() {
  const {ctx, w, h} = canvasSize(scene);
  ctx.clearRect(0, 0, w, h);
  if (sky.complete && sky.naturalWidth) cover(ctx, sky, w, h);
  else { ctx.fillStyle = '#141838'; ctx.fillRect(0, 0, w, h); }
  ctx.fillStyle = '#0a0d20b0'; ctx.fillRect(0, 0, w, h);
  const narrow = w < 560, big = phone.matches, left = big ? 40 : narrow ? 34 : 54, right = narrow ? 14 : 26, top = narrow ? 112 : 104, base = h - (big ? 52 : narrow ? 40 : 44);
  const axis = textPx(10), mark = textPx(11); canvasTexts = [{text: 'axis labels', px: axis}, {text: 'clock labels', px: mark}];
  const xMin = Math.min(-0.6, state.x - 0.6), xMax = Math.max(5.2, state.x + 0.6), tMax = Math.max(7, Math.ceil(state.t + 0.8));
  const X = x => left + (x - xMin) / (xMax - xMin) * (w - left - right);
  const Y = t => base - t / tMax * (base - top);
  // Grid, axes and units.
  ctx.font = `${axis}px system-ui`; ctx.lineWidth = 1; ctx.strokeStyle = '#ffffff1c';
  const xStep = xMax - xMin > 9 ? 2 : 1, tStep = tMax > 14 ? 2 : 1;
  for (let x = Math.ceil(xMin / xStep) * xStep; x <= xMax; x += xStep) line(ctx, X(x), top, X(x), base);
  for (let t = 0; t <= tMax; t += tStep) line(ctx, left, Y(t), w - right, Y(t));
  ctx.strokeStyle = '#ffffff66'; line(ctx, left, base, w - right, base); line(ctx, left, base, left, top);
  ctx.fillStyle = C.ink; ctx.textAlign = 'center';
  for (let x = Math.ceil(xMin / xStep) * xStep; x <= xMax; x += xStep) ctx.fillText(String(x), X(x), base + axis + 4);
  ctx.textAlign = 'right'; ctx.fillText('position x [ly], Earth frame', w - right, base + 2 * axis + 10);
  for (let t = 0; t <= tMax; t += tStep) ctx.fillText(String(t), left - 6, Y(t) + axis / 3);
  ctx.textAlign = 'left'; ctx.fillText('Earth time t [yr]', left + 4, top - 8);
  // Light cone of the departure event: light leaves Earth at t = 0 along x = t (and x = -t).
  ctx.strokeStyle = C.cone; ctx.setLineDash([7, 5]); ctx.lineWidth = 1.5;
  for (const sign of [1, -1]) {
    const tEnd = Math.min(tMax, sign > 0 ? xMax : -xMin);
    line(ctx, X(0), Y(0), X(sign * tEnd), Y(tEnd));
  }
  ctx.setLineDash([]);
  // Worldlines of Earth and the star (both at rest in this frame).
  ctx.lineWidth = 3;
  ctx.strokeStyle = C.earth; line(ctx, X(0), Y(0), X(0), top);
  ctx.strokeStyle = C.star; line(ctx, X(state.dest), Y(0), X(state.dest), top);
  ctx.fillStyle = C.star; ctx.beginPath(); ctx.arc(X(state.dest), base, 6, 0, 2 * Math.PI); ctx.fill();
  ctx.fillStyle = '#f6f0a8'; ctx.textAlign = 'center'; ctx.fillText(`star · ${state.dest.toFixed(1)} ly`, X(state.dest), top - 8);
  if (story) {
    // The trip's markers (at rest in this frame) and the marker of the current leg.
    ctx.setLineDash([3, 6]); ctx.lineWidth = 1.5;
    for (const [x, name] of MARKERS) {
      ctx.strokeStyle = Math.abs(cs.target - x) < 1e-9 ? '#ffffffcc' : '#ffffff55'; line(ctx, X(x), Y(0), X(x), top);
      ctx.fillStyle = '#dfe3f5'; ctx.fillText(name[lang], X(x), top - 8 - (x > 1 ? 0 : axis + 4));
    }
    ctx.setLineDash([]);
    ctx.strokeStyle = '#ffffff'; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(X(cs.target), Y(state.t), 9, 0, 2 * Math.PI); ctx.stroke();
  }
  // The rocket's "now": events with the same ship time, t' = const, i.e. t − t0 = v (x − x0).
  const nowAt = x => state.t + state.v * (x - state.x);
  ctx.strokeStyle = C.now; ctx.setLineDash([4, 4]); ctx.lineWidth = 1.5;
  line(ctx, X(xMin), Y(nowAt(xMin)), X(xMax), Y(nowAt(xMax)));
  ctx.setLineDash([]);
  // The computed worldline (C trail plus the current event).
  ctx.strokeStyle = C.world; ctx.lineWidth = 3; ctx.beginPath();
  trail.forEach(([x, t], i) => i ? ctx.lineTo(X(x), Y(t)) : ctx.moveTo(X(x), Y(t)));
  ctx.lineTo(X(state.x), Y(state.t)); ctx.stroke();
  // Exhaust: a short flare against the thrust direction while the engine is on.
  if (state.thrust && !state.done) {
    const dir = -state.thrust, px = X(state.x), py = Y(state.t);
    const g = ctx.createLinearGradient(px, py, px + dir * 46, py + 10);
    g.addColorStop(0, '#ffd27acc'); g.addColorStop(1, '#ffd27a00');
    ctx.fillStyle = g; ctx.beginPath(); ctx.moveTo(px, py - 7); ctx.lineTo(px + dir * 46, py + 10); ctx.lineTo(px, py + 7); ctx.fill();
  }
  // Markers: Oren on Earth at the same Earth time, Vela at the current event. Both are computed events.
  const size = narrow ? 17 : Math.max(20, Math.min(30, w * 0.026));
  portrait(ctx, twin, X(0), Y(state.t), size * 0.8, C.earth);
  portrait(ctx, rider, X(state.x), Y(state.t), size, C.world);
  ctx.textAlign = 'left'; ctx.font = `600 ${mark}px system-ui`; ctx.fillStyle = '#ffe2a8';
  const labelX = Math.min(X(state.x) + size + 6, w - right - 120);
  ctx.fillText(`τ = ${state.tau.toFixed(2)} yr`, labelX, Y(state.t) - 4);
  ctx.fillStyle = '#b9e8d6'; ctx.fillText(`t = ${state.t.toFixed(2)} yr`, labelX, Y(state.t) + mark);
}

function drawPlot() {
  const small = textPx(9), {ctx, w, h} = canvasSize(plot), left = 44, right = 14, top = small + 11, bottom = small + 21;
  canvasTexts = canvasTexts.filter(row => row.text !== 'chart labels').concat({text: 'chart labels', px: small});
  const last = samples[samples.length - 1] || {tau: 0, t: 0};
  const tauMax = Math.max(4, Math.ceil(last.tau + 0.2)), yMax = Math.max(7, Math.ceil(last.t + 0.5));
  const X = tau => left + tau / tauMax * (w - left - right), Y = y => top + (1 - y / yMax) * (h - top - bottom);
  ctx.clearRect(0, 0, w, h); ctx.font = `${small}px system-ui`; ctx.lineWidth = 1; ctx.textAlign = 'right';
  const yStep = yMax > 14 ? 2 : 1;
  for (let y = 0; y <= yMax; y += yStep) {
    ctx.strokeStyle = '#2f3866'; line(ctx, left, Y(y), w - right, Y(y));
    ctx.fillStyle = '#aab3c9'; ctx.fillText(String(y), left - 8, Y(y) + small / 3);
  }
  ctx.textAlign = 'center';
  for (let tau = 0; tau <= tauMax; tau += tauMax > 10 ? 2 : 1) ctx.fillText(String(tau), X(tau), h - bottom + small + 4);
  ctx.textAlign = 'left'; ctx.fillText('years', 4, top - 7); ctx.textAlign = 'right'; ctx.fillText('ship time τ [yr]', w - right, h - 2);
  for (const [key, colour] of [['t', C.earth], ['tau', C.world]]) {
    ctx.strokeStyle = colour; ctx.lineWidth = 2; ctx.beginPath();
    samples.forEach((s, i) => i ? ctx.lineTo(X(s.tau), Y(s[key])) : ctx.moveTo(X(s.tau), Y(s[key])));
    ctx.stroke();
  }
}

function render() {
  $('story-data').hidden = !story;
  if (story) renderStory();
  else { $('banner').hidden = true; document.querySelector('.scene-label').hidden = false; }
  $('mode-name').textContent = story ? t$('mode') : modeNames[state.mode];
  $('clock').textContent = `τ = ${state.tau.toFixed(2)} yr`;
  $('v').textContent = (Math.abs(state.v) < 5e-4 ? 0 : state.v).toFixed(3);
  $('gamma').textContent = state.gamma.toFixed(3);
  $('x').textContent = state.x.toFixed(2);
  $('gap').textContent = state.gap.toFixed(2);
  $('age-rider').textContent = `${state.tau.toFixed(2)} yr`;
  $('age-twin').textContent = `${state.t.toFixed(2)} yr`;
  let label;
  if (story) label = storyStatus();
  else if (state.arrived) label = `Arrived · ${state.tau.toFixed(2)} yr on board, ${state.t.toFixed(2)} yr on Earth`;
  else if (state.done) label = 'Out of fuel time · back to Earth to try again';
  else if (paused) label = state.steps ? 'Paused' : 'Ready on Earth · hold Accelerate';
  else if (overshot()) label = state.v > 0 ? 'Past the star · decelerate to turn back' : 'Heading back toward the star';
  else if (state.thrust > 0) label = 'Burning toward the star';
  else if (state.thrust < 0) label = state.v > 0 ? 'Braking · burning back' : 'Burning back toward Earth';
  else label = `Coasting at ${state.v.toFixed(2)} c`;
  $('status').textContent = label;
  $('goal').hidden = !state.arrived || story;
  $('engine').textContent = story ? (cs.leg >= 5 ? t$('engineOff') : cs.drift > 0 || (cs.fuel <= 0 && cs.leg < 5) ? t$('engineDead') : state.thrust ? (cs.command === 2 ? t$('engineBrake') : t$('engineOn')) : t$('engineOff'))
    : state.done ? (state.arrived ? 'Engine off · at rest on the star' : 'Engine off') : state.thrust > 0 ? 'Engine on · +1 g toward the star' : state.thrust < 0 ? 'Engine on · −1 g (braking)' : 'Engine off · coasting';
  $('accel').classList.toggle('held', state.thrust > 0 && !state.done);
  $('decel').classList.toggle('held', state.thrust < 0 && !state.done);
  const locked = (state.mode !== 0 && !story) || state.done || (story && cs.scene >= 0);
  $('accel').disabled = $('decel').disabled = locked;
  $('accel').firstChild.textContent = story ? `${t$('burnKey')} ` : 'Accelerate ';
  $('decel').firstChild.textContent = story ? `${t$('brakeKey')} ` : 'Decelerate ';
  $('accel').setAttribute('aria-label', story ? t$('burn') : 'Accelerate toward the star');
  $('decel').setAttribute('aria-label', story ? t$('brake') : 'Decelerate');
  if (story) $('control-goal').textContent = t$('goal');
  else if ($('control-goal').innerHTML !== goalHTML) $('control-goal').innerHTML = goalHTML;
  $('pause').textContent = paused ? 'Resume' : 'Pause';
  $('pause').disabled = !engine || state.done || (!state.steps && paused);
  $('experiment-note').textContent = notes[Math.min(state.mode, 1)];
  applyLang();
  drawScene(); drawPlot();
  window.rrView = {snapshot: {...state, paused, overshot: !story && overshot(), samples: samples.length, story,
    sceneId: story && cs.scene >= 0 ? SCENES[cs.scene].id : null, sceneText: $('story-scene').hidden ? '' : $('story-text').textContent,
    bannerShown: !$('banner').hidden, heavyLog: heavyLog.map(row => ({...row})), lang,
    artReady: Boolean(sky.naturalWidth && rider.naturalWidth && twin.naturalWidth)}, engine};
}
function frame(timestamp) {
  if (!last) last = timestamp;
  const dt = Math.min((timestamp - last) / 1000, 0.25); last = timestamp;
  if (engine && story && !paused && cs.scene < 0 && cs.leg < 5) {
    accumulator += dt;
    let moved = false;
    while (accumulator >= TICK && cs.scene < 0 && cs.leg < 5) {
      engine._cw_tick(1, held > 0 ? 1 : held < 0 ? 2 : 0); accumulator -= TICK; moved = true;
      cs = readStory(); storyTrail.push([cs.x, cs.t]); samples.push({tau: cs.tau, t: cs.t});
    }
    if (moved) { pull(); if (cs.scene >= 0) { paused = true; showScene(); } render(); }
  } else if (engine && !story && !paused && !state.done) {
    accumulator += dt;
    let moved = false;
    while (accumulator >= TICK && !state.done) {
      engine._rr_step(STEPS_PER_TICK); accumulator -= TICK; moved = true;
      state = read(); sample();
    }
    if (moved) { pull(); if (state.done) { paused = true; held = 0; } render(); }
  }
  requestAnimationFrame(frame);
}
// Read-only hook for the phone playability check: sizes in CSS px of the scene canvas, the first goal, the next tap.
let burned = false, orbits = null;
const fromScene = el => { const c = scene.getBoundingClientRect(), r = el.getBoundingClientRect(); return {x: r.left + r.width / 2 - c.left, y: r.top + r.height / 2 - c.top, hold: true}; };
window.__tmGate = {
  version: 1,
  get logical() { const r = scene.getBoundingClientRect(); return {w: r.width || 1, h: r.height || 1}; },
  scene: () => !engine ? 'loading' : story && cs.scene >= 0 ? `scene:${SCENES[cs.scene].id}` : state.done ? 'done' : paused ? 'paused' : 'play',
  goal: () => burned || Boolean(story && cs && cs.leg > 0),
  objective: () => $('control-goal').textContent,
  texts: () => canvasTexts.map(row => ({...row})).concat(orbits ? orbits.texts() : []),
  targets: () => [],
  player: () => null,
  hint: () => {
    if (!engine || state.done) return null;
    if (story) return cs.scene >= 0 ? fromScene($('story-options').firstChild) : fromScene($(cs.cue || cs.range * cs.v < 0 ? 'decel' : 'accel'));
    return state.mode !== 0 ? null : fromScene($(state.x < state.dest / 2 ? 'accel' : 'decel'));
  },
};
phone.addEventListener('change', () => { if (engine) render(); if (orbits) orbits.rerender(); });
for (const img of [sky, rider, twin]) img.onload = () => { if (engine) render(); };
new ResizeObserver(() => { if (engine) render(); }).observe(scene);
try {
  engine = await createRider();
  if (engine._rr_reset(0)) throw new Error('reset');
  for (const id of ['accel', 'decel', 'reset', 'demo', 'story']) $(id).disabled = false;
  reset(); requestAnimationFrame(frame);
  orbits = startOrbits(engine, textPx);
} catch {
  $('status').textContent = 'Rocket model unavailable — reload to retry';
  $('story').disabled = true;
  $('experiment-note').textContent = 'The model could not load. Serve the built files over HTTP with WebAssembly enabled.';
}
