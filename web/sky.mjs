// The view from the window (M3): src/aberration.c and src/skyview.c compute where every catalogue star is seen from a
// ship moving at beta, its Doppler factor, colour and visible-band brightness (WebAssembly); this module only reads
// plain numbers and draws them. Field names follow the enums of src/skyview.h.
const $ = id => document.getElementById(id);
const FIELDS = ['beta','gamma','rapidity','stars','halfCone','forward','forwardExpected','restForward','dAhead','dBehind','tAhead','tBehind',
  'bandAhead','bandBehind','fluxRest','fluxShip','curveN','sunT'];
const STAR = ['x0','y0','x','y','cosRest','cosShip','doppler','tempRest','tempShip','fluxRest','fluxShip','r0','g0','b0','r','g','b'];
const CURVES = ['thetaShip','doppler','density','band','temp'];
const C = {ink: '#c9cfe3', muted: '#8f98b8', grid: '#ffffff22', cone: '#f0e442', d: '#e69f00', density: '#cc79a7', band: '#56b4e9', same: '#ffffff55'};
const DEG = 180 / Math.PI;
const NOTES = {
  0.5: 'At 0.5 c a star square to the motion is seen 60° from the front, blue-shifted by γ = 1.155. Ahead, light arrives 1.73 times bluer.',
  0.9: 'At 0.9 c half of all stars crowd into a cone of 25.8° round the direction of motion. Ahead they are 4.36 times bluer; behind, 4.36 times redder.',
  0.99: 'At 0.99 c half of all stars sit inside 8.1°. Straight ahead D = 14.1: a 5800 K star glows at 82,000 K. Its visible brightness is past its peak (×3.5 near D = 3.2) and falling, because most of its light is now ultraviolet.',
};

export function startSky(engine, textPx) {
  let state, stars = [], curves = [], texts = [], beta = 0.9, sweep = null, last = 0;
  const note = (name, px) => { texts = texts.filter(row => row.text !== name).concat({text: name, px}); };
  function pull() {
    state = Object.fromEntries(FIELDS.map((name, i) => [name, engine._sv_value(i)]));
    stars = Array.from({length: state.stars}, (_, i) => Object.fromEntries(STAR.map((name, f) => [name, engine._sv_star(i, f)])));
    curves = CURVES.map((_, k) => Array.from({length: state.curveN}, (_, i) => engine._sv_curve(i, k)));
  }
  function set(next) {
    if (engine._sv_reset(next)) return;
    beta = next; pull(); render();
  }
  function canvasSize(canvas) {
    const bounds = canvas.getBoundingClientRect(), dpr = Math.min(devicePixelRatio || 1, 2);
    const w = Math.round(bounds.width * dpr), h = Math.round(bounds.height * dpr);
    if (w && h && (canvas.width !== w || canvas.height !== h)) { canvas.width = w; canvas.height = h; }
    const ctx = canvas.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return {ctx, w: bounds.width, h: bounds.height};
  }
  const rgb = (r, g, b) => `rgb(${Math.round(255 * r)},${Math.round(255 * g)},${Math.round(255 * b)})`;
  const size = flux => Math.min(6, Math.max(0.7, 0.9 + 1.1 * (Math.log10(Math.max(flux, 1e-9)) + 2.5)));

  // Two whole-sky discs centred on the direction of motion: radius = angle from it / 180 deg.
  function drawDiscs() {
    const {ctx, w, h} = canvasSize($('sky-view'));
    if (!w) return;
    const label = textPx(11); note('sky labels', label);
    ctx.fillStyle = '#0b0e20'; ctx.fillRect(0, 0, w, h);
    const side = w / h >= 1.3, pad = label + 10;
    const R = side ? Math.min(w / 4 - 12, h / 2 - 2 * pad) : Math.min(w / 2 - 12, h / 4 - 2 * pad);
    const centres = side ? [[w / 4, h / 2 + label / 2], [3 * w / 4, h / 2 + label / 2]] : [[w / 2, h / 4 + label / 2], [w / 2, 3 * h / 4 + label / 2]];
    ctx.font = `${label}px system-ui`; ctx.textBaseline = 'middle';
    centres.forEach(([cx, cy], p) => {
      ctx.fillStyle = '#05060f'; ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill();
      ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
      for (let deg = 30; deg < 180; deg += 30) { ctx.beginPath(); ctx.arc(cx, cy, R * deg / 180, 0, 2 * Math.PI); ctx.stroke(); }
      ctx.strokeStyle = '#ffffff40'; ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.stroke();
      for (const s of stars) {
        const x = p ? s.x : s.x0, y = p ? s.y : s.y0, f = p ? s.fluxShip : s.fluxRest;
        ctx.fillStyle = p ? rgb(s.r, s.g, s.b) : rgb(s.r0, s.g0, s.b0);
        ctx.beginPath(); ctx.arc(cx + R * x, cy - R * y, size(f), 0, 2 * Math.PI); ctx.fill();
      }
      if (p) {
        ctx.strokeStyle = C.cone; ctx.lineWidth = 2; ctx.setLineDash([6, 4]);
        ctx.beginPath(); ctx.arc(cx, cy, Math.max(1, R * state.halfCone / Math.PI), 0, 2 * Math.PI); ctx.stroke(); ctx.setLineDash([]);
      }
      ctx.fillStyle = C.ink; ctx.textAlign = 'center';
      ctx.fillText(p ? `ON BOARD AT β = ${beta.toFixed(2)}` : 'AT REST (β = 0)', cx, cy - R - pad / 2 - 2);
      ctx.fillStyle = C.muted; ctx.textAlign = 'left';
      ctx.fillText('90°', cx + R / 2 + 3, cy - label * 0.6);
    });
  }

  // Left: where a star at Earth-frame angle theta is seen. Right: D, D^2 and the visible brightness against the ship angle (log scale).
  function drawCurves() {
    const {ctx, w, h} = canvasSize($('sky-curves'));
    if (!w || !curves.length) return;
    const axis = textPx(10); note('sky curve axes', axis);
    ctx.fillStyle = '#0b0e20'; ctx.fillRect(0, 0, w, h); ctx.font = `${axis}px system-ui`;
    const side = w / h >= 1.4, boxes = side ? [[0, 0, w / 2, h], [w / 2, 0, w / 2, h]] : [[0, 0, w, h / 2], [0, h / 2, w, h / 2]];
    const frame = ([bx, by, bw, bh]) => {
      const left = bx + Math.round(axis * 3.2) + 8, right = bx + bw - 10, top = by + axis + 14, base = by + bh - 2 * axis - 10;
      return {left, right, top, base, X: deg => left + deg / 180 * (right - left)};
    };
    const xAxis = (f, title) => {
      ctx.strokeStyle = '#ffffff40'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(f.left, f.base); ctx.lineTo(f.right, f.base); ctx.moveTo(f.left, f.top); ctx.lineTo(f.left, f.base); ctx.stroke();
      ctx.fillStyle = C.ink; ctx.textAlign = 'center'; ctx.textBaseline = 'alphabetic';
      for (let d = 0; d <= 180; d += 45) ctx.fillText(`${d}`, f.X(d), f.base + axis + 4);
      ctx.textAlign = 'right'; ctx.fillText(title, f.right, f.base + 2 * axis + 6);
    };
    const path = (f, ys, Y, colour, dash) => {
      ctx.strokeStyle = colour; ctx.lineWidth = 2; ctx.setLineDash(dash); ctx.beginPath();
      ys.forEach((v, i) => (i ? ctx.lineTo(f.X(i), Y(v)) : ctx.moveTo(f.X(i), Y(v)))); ctx.stroke(); ctx.setLineDash([]);
    };
    // 1. aberration
    const a = frame(boxes[0]), Ya = v => a.base - v / 180 * (a.base - a.top);
    xAxis(a, 'angle at rest θ [°]');
    ctx.textAlign = 'right';
    for (let d = 0; d <= 180; d += 90) ctx.fillText(`${d}`, a.left - 4, Ya(d) + axis * 0.35);
    ctx.textAlign = 'left'; ctx.fillText('angle on board θ′ [°]', a.left + 4, a.top - 4);
    path(a, curves[0].map((_, i) => i), Ya, C.same, [4, 4]);
    path(a, curves[0], Ya, C.cone, []);
    // 2. Doppler factor and brightness, log10 from -2 to 2
    const b = frame(boxes[1]), lo = -2, hi = 2, Yb = v => b.base - (Math.min(hi, Math.max(lo, Math.log10(Math.max(v, 1e-9)))) - lo) / (hi - lo) * (b.base - b.top);
    xAxis(b, 'angle on board θ′ [°]');
    ctx.textAlign = 'right';
    for (let e = lo; e <= hi; e++) ctx.fillText(e === 0 ? '1' : `10${['⁻²', '⁻¹', '', '¹', '²'][e + 2]}`, b.left - 4, Yb(10 ** e) + axis * 0.35);
    ctx.textAlign = 'left'; ctx.fillText('ratio to at rest (log)', b.left + 4, b.top - 4);
    ctx.strokeStyle = '#ffffff30'; ctx.beginPath(); ctx.moveTo(b.left, Yb(1)); ctx.lineTo(b.right, Yb(1)); ctx.stroke();
    path(b, curves[2], Yb, C.density, []);
    path(b, curves[1], Yb, C.d, []);
    path(b, curves[3], Yb, C.band, [6, 4]);
  }

  const fmt = (v, d = 2) => (Number.isFinite(v) ? v.toFixed(d) : '—');
  function render() {
    if (!state) return;
    $('sky-beta-out').textContent = `${fmt(beta)} c`;
    $('sky-beta').value = String(beta);
    $('sky-status').textContent = beta === 0 ? 'At rest: the sky as it is' : `β = ${fmt(beta)}: ${state.forward} of ${state.stars} stars ahead of the ship`;
    $('sky-gamma').textContent = fmt(state.gamma, 3);
    $('sky-cone').textContent = fmt(state.halfCone * DEG, 1);
    $('sky-ahead').textContent = `${state.forward} / ${fmt(state.forwardExpected, 0)}`;
    $('sky-doppler').textContent = `${fmt(state.dAhead, 2)} · ${fmt(state.dBehind, 2)}`;
    $('sky-sun').textContent = `${Math.round(state.tAhead).toLocaleString('en-US')} K · ×${fmt(state.bandAhead, 2)}`;
    $('sky-note').textContent = NOTES[beta] || `γ = ${fmt(state.gamma, 3)}: half of all stars are seen within ${fmt(state.halfCone * DEG, 1)}° of the direction of motion; straight ahead light is ${fmt(state.dAhead, 2)} times bluer, straight behind ${fmt(1 / state.dBehind, 2)} times redder.`;
    for (const value of [0.5, 0.9, 0.99]) $(`sky-${String(value).replace('.', '')}`).setAttribute('aria-pressed', String(beta === value));
    $('sky-sweep').textContent = sweep ? 'Stop the sweep' : 'Sweep 0 → 0.99 c';
    drawDiscs(); drawCurves();
    window.skyView = {snapshot: {...state, betaInput: beta, sweeping: Boolean(sweep), starsDrawn: stars.length}};
  }
  function frame(timestamp) {
    if (sweep) {
      if (!last) last = timestamp;
      const dt = Math.min((timestamp - last) / 1000, 0.25); last = timestamp;
      sweep.t += dt;
      const next = Math.min(0.99, Math.round(0.99 * Math.min(1, sweep.t / 8) * 100) / 100);
      if (next !== beta) set(next);
      if (sweep.t >= 8) { sweep = null; render(); }
    } else last = 0;
    requestAnimationFrame(frame);
  }
  $('sky-beta').oninput = event => { sweep = null; set(Number(event.target.value)); };
  for (const value of [0.5, 0.9, 0.99]) $(`sky-${String(value).replace('.', '')}`).onclick = () => { sweep = null; set(value); };
  $('sky-sweep').onclick = () => { sweep = sweep ? null : {t: 0}; if (sweep) set(0); else render(); };
  set(beta);
  requestAnimationFrame(frame);
  new ResizeObserver(() => render()).observe($('sky-view'));
  return {texts: () => texts.map(row => ({...row})), rerender: () => render()};
}
