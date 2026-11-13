// The black-hole experiment (M2): src/orbit.c integrates a Schwarzschild geodesic and its Newtonian twin in C
// (WebAssembly); this module only reads plain numbers and draws them. Field names follow the enum of src/orbit.h.
const $ = id => document.getElementById(id);
const FIELDS = ['kind','M','r0','L','E','h','steps','s','t','r','phi','x','y','pr','state','bound','rp','ra','apses','prec','precExact','precWeak',
  'drift','clockRate','staticRate','isco','photon','bcrit','circStable','circUnstable','nx','ny','nr','nstate','nbound','nrp','nra',
  'trail','view','curveN','curveR0','curveR1','level','nlevel','deflect','deflectInner','deflectTotal','deflectWeak'];
const C = {gr: '#e69f00', newton: '#56b4e9', photon: '#cc79a7', isco: '#009e73', level: '#f0e442', ink: '#c9cfe3', grid: '#ffffff1c'};
const STEPS = {0: 12, 1: 8}, TICK = 1 / 60, DEG = 180 / Math.PI;
const PRESETS = {
  rosette: {kind: 0, r0: 20, L: 4.2, note: 'Released at 20 M with L = 4.2 M: a bound orbit between 10.3 M and 20 M. Each lap the orbit turns by about 122°; Newton’s ellipse from the same start (dashed) closes.'},
  isco: {kind: 0, r0: 7, L: 3.5, note: 'L = 3.5 M is just above √12 M: the circular orbit at 7 M is barely stable (the innermost stable one is at 6 M). Here the orbit is that circle.'},
  plunge: {kind: 0, r0: 30, L: 3.3, note: 'L = 3.3 M is below √12 M: there is no barrier, so the particle falls into the hole. Newton’s orbit with the same L swings round and comes back.'},
  graze: {kind: 1, r0: 30, L: 5.21, note: 'Light with impact parameter 5.21 M, just above the critical 3√3 M = 5.196 M: it circles the photon sphere at 3 M before escaping. The dashed line is the straight path without gravity.'},
  capture: {kind: 1, r0: 30, L: 5.0, note: 'Light with b = 5 M < 3√3 M is captured: nothing stops it falling through the photon sphere and the horizon.'},
};

export function startOrbits(engine, textPx) {
  let labelInfo = null, state, curve = [], ncurve = [], trail = [], ntrail = [], apses = [], paused = true, last = 0, acc = 0, texts = [], preset = 'rosette';
  let kind = 0, r0 = 20, L = 4.2;
  const read = () => Object.fromEntries(FIELDS.map((name, i) => [name, engine._so_value(i)]));
  function pull() {
    state = read();
    curve = Array.from({length: state.curveN}, (_, i) => engine._so_curve(i));
    ncurve = Array.from({length: state.curveN}, (_, i) => engine._so_ncurve(i));
    trail = Array.from({length: state.trail}, (_, i) => [engine._so_trail_x(i), engine._so_trail_y(i)]);
    ntrail = Array.from({length: state.trail}, (_, i) => [engine._so_ntrail_x(i), engine._so_ntrail_y(i)]);
    apses = Array.from({length: Math.min(state.apses, 256)}, (_, i) => engine._so_apsis(i));
  }
  const note = (name, px) => { texts = texts.filter(row => row.text !== name).concat({text: name, px}); };
  function canvasSize(canvas) {
    const bounds = canvas.getBoundingClientRect(), dpr = Math.min(devicePixelRatio || 1, 2);
    const w = Math.round(bounds.width * dpr), h = Math.round(bounds.height * dpr);
    if (w && h && (canvas.width !== w || canvas.height !== h)) { canvas.width = w; canvas.height = h; }
    const ctx = canvas.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return {ctx, w: bounds.width, h: bounds.height};
  }

  function reset(run) {
    if (engine._so_reset(kind, r0, L)) return;
    pull(); paused = !run; acc = 0; render();
  }
  function setKind(next) {
    kind = next;
    if (kind === 1 && (L < 3 || L > 10)) L = 5.21;
    if (kind === 0 && (L < 3 || L > 6)) L = 4.2;
    syncInputs(); reset(false);
  }
  function syncInputs() {
    const slider = $('bh-l');
    if (kind === 1) { slider.min = '3'; slider.max = '10'; } else { slider.min = '3'; slider.max = '6'; }
    slider.value = String(L); $('bh-r0').value = String(r0);
    $('bh-r0').disabled = kind === 1;
    $('bh-r0-row').classList.toggle('off', kind === 1);
    $('bh-l-name').textContent = kind === 1 ? 'Impact parameter b' : 'Angular momentum L';
    $('bh-particle').setAttribute('aria-pressed', String(kind === 0));
    $('bh-light').setAttribute('aria-pressed', String(kind === 1));
  }
  $('bh-particle').onclick = () => { preset = null; setKind(0); };
  $('bh-light').onclick = () => { preset = null; setKind(1); };
  $('bh-r0').oninput = event => { r0 = Number(event.target.value); preset = null; reset(false); };
  $('bh-l').oninput = event => { L = Number(event.target.value); preset = null; reset(false); };
  $('bh-launch').onclick = () => reset(true);
  $('bh-freeze').onclick = () => { if (state.state !== 0 || !state.steps && paused) return; paused = !paused; acc = 0; render(); };
  for (const [name, p] of Object.entries(PRESETS)) {
    $(`bh-${name}`).onclick = () => { preset = name; kind = p.kind; r0 = p.r0; L = p.L; syncInputs(); reset(true); };
  }

  function drawOrbit() {
    const {ctx, w, h} = canvasSize($('bh-orbit'));
    if (!w) return;
    const label = textPx(11); note('orbit labels', label);
    const view = state.view, cx = w / 2, cy = h / 2, k = (Math.min(w, h) / 2 - label - 8) / view;
    const X = x => cx + x * k, Y = y => cy - y * k;
    ctx.fillStyle = '#0b0e20'; ctx.fillRect(0, 0, w, h);
    ctx.font = `${label}px system-ui`; ctx.textBaseline = 'middle';
    // Distance rings every 10 M (5 M for small views). Their labels are drawn last, on a backing box, along the
    // direction farthest from every periapsis tick (so no tick crosses a label).
    let stepR = 5;
    while (stepR * k < 3.4 * label) stepR *= 2;
    ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
    const rings = [];
    for (let r = stepR; r < view; r += stepR) { ctx.beginPath(); ctx.arc(cx, cy, r * k, 0, 2 * Math.PI); ctx.stroke(); rings.push(r); }
    const ring = (r, colour, dash) => { ctx.strokeStyle = colour; ctx.setLineDash(dash); ctx.lineWidth = 1.5; ctx.beginPath(); ctx.arc(cx, cy, r * k, 0, 2 * Math.PI); ctx.stroke(); ctx.setLineDash([]); };
    ring(state.isco, C.isco, [5, 4]); ring(state.photon, C.photon, [3, 3]);
    ctx.fillStyle = '#000'; ctx.beginPath(); ctx.arc(cx, cy, Math.max(2, 2 * state.M * k), 0, 2 * Math.PI); ctx.fill();
    ctx.strokeStyle = '#ffffff60'; ctx.lineWidth = 1; ctx.stroke();
    const path = (points, colour, dash, width) => {
      if (points.length < 2) return;
      ctx.strokeStyle = colour; ctx.setLineDash(dash); ctx.lineWidth = width; ctx.beginPath();
      points.forEach(([x, y], i) => (i ? ctx.lineTo(X(x), Y(y)) : ctx.moveTo(X(x), Y(y)))); ctx.stroke(); ctx.setLineDash([]);
    };
    path(ntrail, C.newton, [6, 5], 1.6);
    path(trail, C.gr, [], 2.2);
    // Periapsis marks: a short tick at each measured periapsis direction, at r_p.
    const ticks = [];
    if (state.kind === 0 && state.rp > 0) {
      ctx.strokeStyle = '#ffffffb0'; ctx.lineWidth = 1.5;
      for (const a of apses) {
        const c = Math.cos(a), s = Math.sin(a), seg = [X(c * (state.rp - 1.2)), Y(s * (state.rp - 1.2)), X(c * (state.rp + 1.2)), Y(s * (state.rp + 1.2))];
        ctx.beginPath(); ctx.moveTo(seg[0], seg[1]); ctx.lineTo(seg[2], seg[3]); ctx.stroke(); ticks.push(seg);
      }
    }
    const dot = (x, y, colour) => { ctx.fillStyle = colour; ctx.beginPath(); ctx.arc(X(x), Y(y), 5, 0, 2 * Math.PI); ctx.fill(); };
    if (state.nstate !== 1 && Math.hypot(state.nx, state.ny) < 1.5 * view) dot(state.nx, state.ny, C.newton);
    if (state.state !== 1) dot(state.x, state.y, C.gr);
    // Ring labels: try directions round the hole and keep the one whose label boxes stay inside the canvas and farthest
    // (in angle) from every periapsis direction; a box crossed by no tick is required, the backing box only tidies.
    ctx.textBaseline = 'middle'; ctx.textAlign = 'left';
    const boxes = angle => rings.map(r => {
      const text = `${r} M`, tw = ctx.measureText(text).width, x = cx + r * k * Math.cos(angle) + 4, y = cy - r * k * Math.sin(angle);
      return {text, x: x - 2, y: y - label * 0.65, w: tw + 4, h: label * 1.3};
    });
    const inside = b => b.x >= 0 && b.y >= 0 && b.x + b.w <= w && b.y + b.h <= h - label - 10;
    const crosses = (b, [x0, y0, x1, y1]) => {
      // segment against rectangle: either end inside, or the segment meets one of the four edges
      const inRect = (x, y) => x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h;
      if (inRect(x0, y0) || inRect(x1, y1)) return true;
      const meet = (ax, ay, bx, by) => {
        const d = (x1 - x0) * (by - ay) - (y1 - y0) * (bx - ax);
        if (!d) return false;
        const t = ((ax - x0) * (by - ay) - (ay - y0) * (bx - ax)) / d, u = ((ax - x0) * (y1 - y0) - (ay - y0) * (x1 - x0)) / d;
        return t >= 0 && t <= 1 && u >= 0 && u <= 1;
      };
      return meet(b.x, b.y, b.x + b.w, b.y) || meet(b.x, b.y + b.h, b.x + b.w, b.y + b.h) || meet(b.x, b.y, b.x, b.y + b.h) || meet(b.x + b.w, b.y, b.x + b.w, b.y + b.h);
    };
    const gap = angle => apses.reduce((m, a) => Math.min(m, Math.abs(Math.atan2(Math.sin(a - angle), Math.cos(a - angle)))), Math.PI);
    let best = null;
    for (let deg = 0; deg < 360; deg += 15) {
      const angle = deg / DEG, list = boxes(angle), hits = list.reduce((n, b) => n + ticks.filter(t => crosses(b, t)).length, 0);
      const score = [list.every(inside) ? 0 : 1, hits, -gap(angle), Math.abs(Math.sin(angle - Math.PI / 12))];
      const better = !best || (() => { for (let i = 0; i < score.length; i++) if (score[i] !== best.score[i]) return score[i] < best.score[i]; return false; })();
      if (better) best = {angle, list, hits, score};
    }
    for (const b of best.list) {
      ctx.fillStyle = '#0b0e20d0'; ctx.fillRect(b.x, b.y, b.w, b.h);
      ctx.fillStyle = '#8f98b8'; ctx.fillText(b.text, b.x + 2, b.y + b.h / 2);
    }
    labelInfo = {angle: best.angle * DEG, hits: best.hits, gap: gap(best.angle) * DEG, labels: best.list.length, outside: best.list.filter(b => !inside(b)).length};
    ctx.fillStyle = C.ink; ctx.textAlign = 'left'; ctx.textBaseline = 'bottom';
    ctx.fillText(`orbital plane, x and y in M · view ±${view.toFixed(0)} M`, 8, h - 6);
  }

  function drawPotential() {
    const {ctx, w, h} = canvasSize($('bh-potential'));
    if (!w || !curve.length) return;
    const axis = textPx(10); note('potential axis', axis);
    const left = Math.round(axis * 3.6) + 8, right = 12, top = axis + 12, base = h - 2 * axis - 10;
    const r0c = state.curveR0, r1c = state.curveR1, X = r => left + (r - r0c) / (r1c - r0c) * (w - left - right);
    // Vertical range: for a particle, magnify the well around the level E² (the well of a wide orbit is shallow);
    // for light, from 0 to a little above the level and the peak.
    const level = state.level, peak = Math.max(...curve);
    let lo, hi;
    if (state.kind === 1) { lo = 0; hi = Math.max(1.2, Math.min(peak, 2) * 1.1); }
    else {
      const wells = curve.filter((v, i) => i > 0 && i < curve.length - 1 && v <= curve[i - 1] && v <= curve[i + 1]);
      const span = wells.length ? Math.max(level - Math.min(...wells), 0.004) : 0.05;
      lo = (wells.length ? Math.min(...wells) : level) - 0.8 * span; hi = level + 1.6 * span;
    }
    const Y = v => base - (Math.min(Math.max(v, lo), hi) - lo) / (hi - lo) * (base - top);
    ctx.fillStyle = '#0b0e20'; ctx.fillRect(0, 0, w, h); ctx.font = `${axis}px system-ui`;
    ctx.strokeStyle = '#ffffff40'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(left, base); ctx.lineTo(w - right, base); ctx.moveTo(left, top); ctx.lineTo(left, base); ctx.stroke();
    ctx.fillStyle = C.ink; ctx.textAlign = 'center'; ctx.textBaseline = 'alphabetic';
    const tickStep = r1c > 30 ? 10 : 5;
    for (let r = tickStep; r <= r1c; r += tickStep) ctx.fillText(String(r), X(r), base + axis + 4);
    ctx.textAlign = 'right'; ctx.fillText('r [M]', w - right, base + 2 * axis + 6);
    ctx.textAlign = 'left'; ctx.fillText(state.kind === 0 ? '2V(r) and E²' : '2V(r) = (1 − 2M/r) b²/r² and E² = 1', left + 4, top - 4);
    ctx.textAlign = 'right'; ctx.fillText(lo.toFixed(3), left - 4, base); ctx.fillText(hi.toFixed(3), left - 4, top + axis * 0.6);
    // Vertical marks at 3M and 6M.
    for (const [r, colour] of [[state.photon, C.photon], [state.isco, C.isco]]) {
      ctx.strokeStyle = colour; ctx.setLineDash([3, 3]); ctx.beginPath(); ctx.moveTo(X(r), top); ctx.lineTo(X(r), base); ctx.stroke(); ctx.setLineDash([]);
    }
    const line = (values, colour, dash, width) => {
      ctx.strokeStyle = colour; ctx.setLineDash(dash); ctx.lineWidth = width; ctx.beginPath();
      values.forEach((v, i) => { const x = X(r0c + (r1c - r0c) * i / (values.length - 1)); return i ? ctx.lineTo(x, Y(v)) : ctx.moveTo(x, Y(v)); });
      ctx.stroke(); ctx.setLineDash([]);
    };
    line(ncurve, C.newton, [6, 5], 1.5);
    line(curve, C.gr, [], 2.2);
    ctx.strokeStyle = C.level; ctx.lineWidth = 1.5; ctx.beginPath(); ctx.moveTo(left, Y(level)); ctx.lineTo(w - right, Y(level)); ctx.stroke();
    if (state.state !== 1 && state.r >= r0c && state.r <= r1c) {
      ctx.fillStyle = C.gr; ctx.beginPath(); ctx.arc(X(state.r), Y(level), 5, 0, 2 * Math.PI); ctx.fill();
    }
  }

  const fmt = (v, d = 2) => (Number.isFinite(v) ? v.toFixed(d) : '—');
  function status() {
    const s = state;
    if (s.kind === 1) {
      if (s.state === 1) return `Captured: b = ${fmt(s.L)} M < 3√3 M = ${fmt(s.bcrit, 3)} M`;
      if (s.state === 2) return `Escaped, bent by ${fmt(s.deflect * DEG, 1)}° inside 30 M`;
      return paused ? (s.steps ? 'Frozen' : `Light ready at 30 M · b = ${fmt(s.L)} M`) : 'Light in flight';
    }
    if (s.state === 1) return 'Fell into the hole';
    if (s.state === 3) return 'Step limit reached';
    if (s.bound === 2) return paused && !s.steps ? `No turning point: this start falls in (L = ${fmt(s.L)} M)` : 'Falling in';
    if (s.bound === 1) return 'Unbound: it escapes';
    if (s.rp === s.ra || Math.abs(s.ra - s.rp) < 1e-6 * s.ra) return `Circular orbit at ${fmt(s.rp)} M`;
    return `Bound orbit between ${fmt(s.rp)} M and ${fmt(s.ra)} M${paused && !s.steps ? ' · press Launch' : ''}`;
  }
  function render() {
    const s = state;
    $('bh-mode').textContent = s.kind === 0 ? 'PARTICLE' : 'LIGHT';
    $('bh-clock').textContent = s.kind === 0 ? `τ = ${fmt(s.s, 0)} M · t = ${fmt(s.t, 0)} M` : `λ = ${fmt(s.s, 1)} M`;
    $('bh-status').textContent = status();
    $('bh-r0-out').textContent = s.kind === 0 ? `${fmt(r0, 1)} M` : '30 M (fixed)';
    $('bh-l-out').textContent = `${fmt(L)} M`;
    $('bh-r').textContent = s.state === 1 ? '≤ 2.002' : fmt(s.r);
    const particle = s.kind === 0;
    $('bh-k1').textContent = particle ? 'TURN PER LAP (MEASURED)' : 'BENT INSIDE 30 M (MEASURED)';
    $('bh-v1').textContent = particle ? (s.prec > -90 ? fmt(s.prec * DEG, 2) : '—') : (s.deflect > -90 ? fmt(s.deflect * DEG, 2) : '—');
    $('bh-k2').textContent = particle ? 'ELLIPTIC INTEGRAL · 6πM/p' : 'QUADRATURE · 4M/b (TOTAL)';
    $('bh-v2').textContent = particle
      ? (s.precExact > -90 ? `${fmt(s.precExact * DEG, 2)} · ${fmt(s.precWeak * DEG, 1)}` : '—')
      : (s.deflectInner > -90 ? `${fmt(s.deflectInner * DEG, 2)} · ${fmt(s.deflectWeak * DEG, 1)}` : `— · ${fmt(s.deflectWeak * DEG, 1)}`);
    $('bh-k3').textContent = particle ? 'CLOCK dτ/dt · AT REST' : 'ANGLE φ (DEGREES)';
    $('bh-v3').textContent = particle ? (s.state === 1 ? '0' : `${fmt(s.clockRate, 3)} · ${fmt(s.staticRate, 3)}`) : fmt(s.phi * DEG, 0);
    $('bh-laps').textContent = particle ? `${Math.max(0, s.apses - 1)} laps · energy error ${s.drift.toExponential(1)}` : `energy error ${s.drift.toExponential(1)}`;
    $('bh-freeze').textContent = paused && s.steps ? 'Continue' : 'Freeze';
    $('bh-freeze').disabled = s.state !== 0 || (!s.steps && paused);
    if (preset) $('bh-note').textContent = PRESETS[preset].note;
    else $('bh-note').textContent = particle
      ? `Released at rest radially at r₀ with angular momentum L. Circular orbits need L ≥ √12 M ≈ 3.46 M; the one for L = ${fmt(L)} M is at ${s.circStable ? fmt(s.circStable) + ' M' : 'none'}.`
      : 'Light from 30 M with impact parameter b. Below 3√3 M ≈ 5.196 M it is captured.';
    drawOrbit(); drawPotential();
    window.bhView = {snapshot: {...s, paused, trailPoints: trail.length, apsesDrawn: apses.length, labelInfo, preset, kindInput: kind, r0Input: r0, LInput: L}};
  }
  function frame(timestamp) {
    if (!last) last = timestamp;
    const dt = Math.min((timestamp - last) / 1000, 0.25); last = timestamp;
    if (!paused && state.state === 0) {
      acc += dt;
      let moved = false;
      while (acc >= TICK && state.state === 0) { engine._so_step(STEPS[state.kind]); acc -= TICK; moved = true; state.state = engine._so_value(14); }
      if (moved) { pull(); if (state.state !== 0) paused = true; render(); }
    }
    requestAnimationFrame(frame);
  }
  syncInputs(); reset(false); requestAnimationFrame(frame);
  new ResizeObserver(() => render()).observe($('bh-orbit'));
  return {texts: () => texts.map(row => ({...row})), rerender: () => render()};
}
