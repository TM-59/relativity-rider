# Measured values

Written by `make theory-figs` (tests/theory_figs.c) from the program's own functions. Do not edit by hand.
Units: c = 1, times in years, lengths in light-years; on page 5 G = c = M = 1, lengths and times in M. The last column is the relative error; where the theory column is 0 it is the absolute value.

| id | what is measured | measured | theory | error |
|---|---|---|---|---|
| `interval-event` | interval t^2 - x^2 of the event (t, x) = (3, 1.5) (theory 9 - 2.25) | 6.75 | 6.75 | 0.00e+00 |
| `interval-invariance` | largest change of that interval over 151 boosts of rapidity -1.5 to 1.5 (theory 0) | 1.42108547e-14 | 0 | 1.42e-14 |
| `boost-inverse` | distance from (3, 1.5) after a boost of 0.7 and a boost of -0.7 (theory 0) | 4.96506831e-16 | 0 | 4.97e-16 |
| `boost-rest` | x' of the event in the frame moving at 0.5: the event lies on that frame's own worldline x = 0.5 t (theory 0) | -4.4408921e-16 | 0 | 4.44e-16 |
| `boost-proper-time` | t' of the event in that frame: the proper time along x = 0.5 t (theory sqrt(6.75)) | 2.59807621 | 2.59807621 | 2.22e-16 |
| `light-stays-light` | x' / t' of an event on the light cone, (2, 2), after a boost of 0.9 (theory 1: light moves at c in every frame) | 1 | 1 | 0.00e+00 |
| `boosts-compose` | two boosts of rapidity 0.4 and 0.5 against one of 0.9, distance between the results (theory 0: rapidities add) | 0 | 0 | 0.00e+00 |
| `velocity-phi1` | velocity at rapidity 1 (theory tanh 1) | 0.761594156 | 0.761594156 | 0.00e+00 |
| `gamma-phi1` | Lorentz factor at rapidity 1 (theory cosh 1 = 1 / sqrt(1 - v^2)) | 1.54308063 | 1.54308063 | 0.00e+00 |
| `add-0.9-0.9` | 0.9 c combined with 0.9 c (theory 1.8 / 1.81) | 0.994475138 | 0.994475138 | 0.00e+00 |
| `add-light` | c combined with 0.5 c (theory 1) | 1 | 1 | 0.00e+00 |
| `add-rapidities` | rapidity of tanh 0.3 combined with tanh 0.5 (theory 0.8) | 0.8 | 0.8 | 1.11e-16 |
| `add-small` | 0.001 c combined with 0.002 c over the Newtonian sum (theory 1 / (1 + 2e-6)) | 0.999998 | 0.999998 | 1.11e-16 |
| `dilation-gamma2` | Earth time elapsed while a coasting clock at gamma = 2 ticks 3.01 yr (theory 2 x 3.01) | 6.02 | 6.02 | 6.66e-15 |
| `dilation-speed` | x / t of that coaster (theory sqrt(3) / 2) | 0.866025404 | 0.866025404 | 1.04e-14 |
| `dilation-interval` | sqrt(t^2 - x^2) of the coaster's path: the invariant interval is its own proper time (theory 3.01) | 3.01 | 3.01 | 3.80e-14 |
| `dilation-half-c` | Earth time for 3.01 yr on a clock moving at 0.5 c (theory 3.01 / sqrt(0.75)) | 3.47564862 | 3.47564862 | 6.66e-15 |
| `contraction-0.8` | distance Earth-star in the frame of a rocket at 0.8 c, both ends at the same rocket time (theory 4.2 x 0.6) | 2.52 | 2.52 | 4.44e-16 |
| `contraction-simultaneous` | largest difference of the two rocket-frame times used for those lengths (theory 0: measured at one instant) | 3.55271368e-15 | 0 | 3.55e-15 |
| `now-at-arrival` | Earth's clock simultaneous with the arrival in the rocket's frame, t - v x, with the rocket at rest again (theory: the Earth time of the arrival) | 5.82360188 | 5.82360188 | 0.00e+00 |
| `now-at-flip` | the same at the flip, halfway: half the Earth time has passed in Earth's frame, but by the rocket's simultaneity Earth's clock reads only this [yr] (theory t - v x with x = 2.1) | 0.919178409 | 0.919178409 | 1.22e-15 |
| `now-flip-earth-frame` | Earth time of the flip in Earth's own frame [yr] (theory sinh(a tau_half) / a) | 2.91180094 | 2.91180094 | 1.33e-14 |
| `now-round-flip` | Earth clock gained, by the rocket's simultaneity, during the 200 steps round the flip, per year on board | 0.496114883 | 0 | 4.96e-01 |
| `now-second-half` | the same averaged over the whole second half, while the rocket slows down (it has to catch up to the Earth time of the arrival) | 2.78132673 | 0 | 2.78e+00 |
| `now-line` | rocket-frame time difference between (5, 0) and (5 + 0.6 x 3, 3) at v = 0.6: events on the line t - t0 = v (x - x0) are simultaneous on board (theory 0) | 0 | 0 | 0.00e+00 |
| `hyperbola` | largest relative departure from (x + 1/a)^2 - t^2 = 1/a^2 over 3 yr of proper time at 1 g (theory 0) | 1.80189197e-13 | 0 | 1.80e-13 |
| `hyperbola-t` | largest difference between t and sinh(a tau) / a over those 600 steps [yr] (theory 0) | 1.79412041e-13 | 0 | 1.79e-13 |
| `hyperbola-v` | largest difference between v and tanh(a tau) (theory 0) | 3.10862447e-15 | 0 | 3.11e-15 |
| `after-3yr-x` | distance covered in 3 yr of proper time at 1 g [ly] (theory (cosh(3a) - 1) / a) | 9.77159414 | 9.77159414 | 2.96e-14 |
| `after-3yr-t` | Earth time for it [yr] (theory sinh(3a) / a) | 10.6965296 | 10.6965296 | 2.65e-14 |
| `horizon` | distance behind the start from which light never catches the rocket, 1 / a [ly] | 0.968710646 | 0.968710646 | 0.00e+00 |
| `v-after-1yr` | speed after one year on board at 1 g (theory tanh(1.0323)) | 0.774829151 | 0.774829151 | 2.78e-15 |
| `one-g` | ONE_G against 9.80665 m/s^2 x one Julian year / c (theory 1.03227) | 1.0323 | 1.03229528 | 4.58e-06 |
| `step-size-x` | x after 2 yr at 1 g in 100000 steps against one step [ly] (exact step: 0 but for rounding) | 7.69739827e-12 | 0 | 7.70e-12 |
| `step-size-t` | the same for t [yr] | 7.92654831e-12 | 0 | 7.93e-12 |
| `piecewise-x` | x after 1 yr at +1 g, 0.5 yr coasting, 1 yr at -1 g (theory: the three closed forms added) | 1.74012322 | 1.74012322 | 0.00e+00 |
| `piecewise-t` | t for the same (theory likewise) | 3.16555404 | 3.16555404 | 0.00e+00 |
| `piecewise-v` | velocity at the end (theory 0: equal burns cancel in rapidity) | 0 | 0 | 0.00e+00 |
| `coast-limit` | x after 0.3 yr with a = 1e-9 against a = 0 from rapidity 0.8 [ly] (theory 0; the closed form divides a rounding error by a) | -2.9117532e-08 | 0 | 2.91e-08 |
| `step-reverse` | distance from the start after a step of +1.3 yr and one of -1.3 yr (theory 0) | 0 | 0 | 0.00e+00 |
| `trip-distance` | where a 1 g flip-and-burn planned for 4.2 ly ends [ly] (theory 4.2) | 4.2 | 4.2 | 0.00e+00 |
| `trip-ship-time` | years on board (theory (2 / a) acosh(1 + a D / 2)) | 3.52667914 | 3.52667914 | 0.00e+00 |
| `trip-earth-time` | years on Earth (theory (2 / a) sinh(a tau_half) = sqrt(D^2 + 4 D / a)) | 5.82360188 | 5.82360188 | 0.00e+00 |
| `trip-top-speed` | speed at the flip (theory tanh(a tau_half)) | 0.948867872 | 0.948867872 | 0.00e+00 |
| `trip-top-gamma` | Lorentz factor at the flip (theory 1 + a D / 2) | 3.16783 | 3.16783 | 0.00e+00 |
| `trip-end-speed` | speed at the end (theory 0) | 0 | 0 | 0.00e+00 |
| `trip-vs-light` | Earth time over the light-travel time 4.2 yr (always above 1) | 1.38657188 | 1 | 3.87e-01 |
| `demo-steps` | steps of the demonstration (theory 2 x 400) | 800 | 800 | 0.00e+00 |
| `demo-dtau` | proper time per step [yr] (theory tau_half / 400) | 0.00440834892 | 0.00440834892 | 0.00e+00 |
| `demo-arrived` | RR_ARRIVED at the end (theory 1: within 0.05 ly and below 0.05 c) | 1 | 1 | 0.00e+00 |
| `demo-ship` | RR_TAU at the end [yr] (theory (2 / a) acosh(1 + a D / 2)) | 3.52667914 | 3.52667914 | 3.33e-15 |
| `demo-earth` | RR_T at the end [yr] (theory sqrt(D^2 + 4 D / a)) | 5.82360188 | 5.82360188 | 1.34e-14 |
| `demo-gap` | RR_GAP, Earth's clock ahead of the ship's at the end [yr] (theory the difference of the two) | 2.29692274 | 2.29692274 | 3.91e-14 |
| `demo-max-v` | RR_MAX_V (theory tanh(a tau_half)) | 0.948867872 | 0.948867872 | 1.22e-15 |
| `demo-trail` | worldline points recorded (theory 1 + 800 / 4) | 201 | 201 | 0.00e+00 |
| `demo-trail-end` | x of the last recorded point [ly] (theory 4.2) | 4.2 | 4.2 | 1.59e-14 |
| `demo-trail-end-t` | t of the last recorded point [yr] (theory the Earth time of the trip) | 5.82360188 | 5.82360188 | 1.34e-14 |
| `demo-locked` | steps taken after the end, with thrust asked for (theory 0: finished, controls locked) | 0 | 0 | 0.00e+00 |
| `manual-start` | RR_ARRIVED right after a reset (theory 0) | 0 | 0 | 0.00e+00 |
| `manual-rapidity` | rapidity after 100 steps of Accelerate (theory 1 g x 100 dtau) | 0.455073859 | 0.455073859 | 6.66e-16 |
| `manual-coast` | rapidity change over 100 coasting steps (theory 0) | 0 | 0 | 0.00e+00 |
| `manual-stop` | velocity after 100 steps of Decelerate (theory 0) | -2.60208521e-17 | 0 | 2.60e-17 |
| `manual-clamp` | steps taken by one call asking for 5000 (theory 400: one call is clamped) | 400 | 400 | 0.00e+00 |
| `manual-limit` | steps after 30 calls of 400 with no thrust (theory RR_MAX_STEPS 8000) | 8000 | 8000 | 0.00e+00 |
| `manual-never-arrives` | RR_ARRIVED for a rocket that never left (theory 0) | 0 | 0 | 0.00e+00 |
| `geo-e` | E of the orbit with periapsis 10 M and apoapsis 30 M, from 2V(10) (theory E^2 = 1 - 2M u1 u2 u3 L^2 from the turning points) | 0.976315261 | 0.976315261 | 0.00e+00 |
| `geo-l` | its L (theory 1 / sqrt(u1 u2 + u1 u3 + u2 u3), u3 = 1/2M - u1 - u2) | 4.37594974 | 4.37594974 | 0.00e+00 |
| `geo-e-fd` | largest abs((1 - 2M/r) dt/dtau / E - 1) from finite differences over 3 orbits, step 0.4 (theory 0; the midpoint difference is second order) | 1.13460854e-06 | 0 | 1.13e-06 |
| `geo-l-fd` | largest abs(r^2 dphi/dtau / L - 1) from the same differences (theory 0) | 9.07696232e-06 | 0 | 9.08e-06 |
| `geo-energy-100` | largest abs(H / H0 - 1) over 160000 steps (about 100 orbits) of 0.4 (theory 0; bounded, it does not grow) | 1.26574307e-11 | 0 | 1.27e-11 |
| `potential-force` | largest relative gap between geo_force and -dV/dr by central differences, 3 kinds, r = 2.5..60 (theory 0, up to the difference error) | 1.80163597e-08 | 0 | 1.80e-08 |
| `potential-extra` | 2V_GR - (1 + 2V_Newton) at r = 9, L = 4 (theory -2 M L^2 / r^3) | -0.0438957476 | -0.0438957476 | 1.11e-15 |
| `potential-level` | E^2 of the browser's default start (r0 = 20, L = 4.2, at rest radially) (theory 2V(20)) | 0.93969 | 0.93969 | 0.00e+00 |
| `circular-l` | L of the circular orbit at 10 M (theory sqrt(M r^2 / (r - 3M)) = sqrt(100/7)) | 3.77964473 | 3.77964473 | 0.00e+00 |
| `circular-radius` | geo_circular_radius(L) for that L [M] (theory 10) | 10 | 10 | 2.22e-16 |
| `circular-stays` | largest abs(r - 10) over 10 orbits integrated from the circle (theory 0) | 0 | 0 | 0.00e+00 |
| `circular-kepler` | dphi/dt measured over 10 orbits at 10 M (theory sqrt(M / r^3), Kepler's law in far-away time) | 0.0316227766 | 0.0316227766 | 1.80e-13 |
| `circular-energy` | E of that orbit (theory (1 - 2M/r) / sqrt(1 - 3M/r)) | 0.956182887 | 0.956182887 | 0.00e+00 |
| `isco-scan` | radius of the least angular momentum among circular orbits, scanned in steps of 1e-4 M (theory 6M) | 6 | 6 | 4.29e-13 |
| `isco-l` | that least L [M] (theory sqrt 12) | 3.46410162 | 3.46410162 | 0.00e+00 |
| `isco-flat` | V''(6M) at L^2 = 12 by central differences (theory 0: the minimum and the maximum merge) | -5.14406226e-10 | 0 | 5.14e-10 |
| `isco-outside` | width max r - min r of the circle at 7M nudged by dr/dtau = 0.01, over 1200 M [M] (theory 2 x 0.01 / sqrt(V''(7)), small oscillation) | 0.744015291 | 0.740810412 | 4.33e-03 |
| `isco-inside` | state of the circle at 5M nudged inward by 0.001 after 1200 M (theory 1: fell in) | 1 | 1 | 0.00e+00 |
| `photon-peak` | radius where the light potential peaks, scanned in steps of 1e-5 M (theory 3M) | 3 | 3 | 1.97e-12 |
| `photon-bcrit` | 1 / sqrt(2V) at that peak (theory 3 sqrt 3 M = 5.196) | 5.19615242 | 5.19615242 | 0.00e+00 |
| `light-below` | state of light with b = 0.999 b_c (theory 1: captured) | 1 | 1 | 0.00e+00 |
| `light-above` | angle swept by light with b = 1.001 b_c from 40M and back out [rad] (theory column 2 pi: it winds more than once round) | 9.39148745 | 6.28318531 | 4.95e-01 |
| `deflection-1000` | deflection of light passing at b = 1000 M [rad] (theory 4M/b, plus (15 pi / 4)(M/b)^2 = 1.18e-5) | 0.00401182382 | 0.004 | 2.96e-03 |
| `deflection-1000-two` | the same against 4M/b + (15 pi / 4)(M/b)^2 | 0.00401182382 | 0.00401178097 | 1.07e-05 |
| `deflection-6` | deflection at b = 6 M, close to the photon sphere [rad] (4M/b = 0.667 is far off) | 1.71938831 | 0.666666667 | 1.58e+00 |
| `deflection-sun` | deflection by the Sun at its rim, M = 1476.6 m, b = 6.957e8 m [arcsec] (theory 1.75) | 1.75119028 | 1.75 | 6.80e-04 |
| `deflection-quadrature` | largest gap between the integrated sweep (step 0.004 b, linear crossing) and the quadrature, b = 8..400 M [rad] (theory 0) | 1.31754607e-10 | 0 | 1.32e-10 |
| `precession-integrated` | largest gap between the integrated turn and the elliptic integral, periapsis 8..4000 M [rad] (theory 0) | 3.23066018e-11 | 0 | 3.23e-11 |
| `precession-10-30` | turn per orbit between 10 M and 30 M, quadrature [rad] (theory the elliptic integral) | 1.84727666 | 1.84727666 | 6.68e-12 |
| `precession-weak-10-30` | the weak-field formula for that orbit [rad] (a = 20, e = 0.5): strong field, 32% short | 1.25663706 | 1.84727666 | 3.20e-01 |
| `precession-first-order` | (turn / weak - 1) p at p = 1.33e5 M (theory (18 + e^2)/4 = 4.528) | 4.52794915 | 4.52777778 | 3.78e-05 |
| `precession-mercury` | Mercury: weak-field turn per century [arcsec] (M_sun = 1476.6 m, a = 5.79e10 m, e = 0.2056; theory 42.98) | 42.9807117 | 42.98 | 1.66e-05 |
| `clock-split` | largest gap between the integrated rate dtau/dt on circular orbits (one lap, r = 3.2..40) and sqrt(1 - 2M/r) / gamma(v_local) (theory 0) | 6.20614671e-14 | 0 | 6.21e-14 |
| `clock-static-6` | a clock held at 6M against the far-away clock (theory sqrt(2/3)) | 0.816496581 | 0.816496581 | 0.00e+00 |
| `clock-orbit-6` | a clock on the circular orbit at 6M (theory sqrt(1/2)) | 0.707106781 | 0.707106781 | 0.00e+00 |
| `clock-local-6` | speed of that orbit measured by a static observer at 6M (theory 1/2) | 0.5 | 0.5 | 0.00e+00 |
| `clock-earth` | a clock on the ground against one far away: 1 - rate, M = 4.435e-3 m, r = 6.371e6 m (theory 6.96e-10) | 6.96123048e-10 | 6.96e-10 | 1.77e-04 |
| `fall-3` | proper time of a fall from rest at 10M down to 3M (theory the cycloid sqrt(R^3/2M)(sqrt(q(1-q)) + acos sqrt q)) | 32.4098858 | 32.4098858 | 2.55e-10 |
| `fall-horizon` | proper time when the fall reaches 2.002M and stops (theory just under the time to 2M) | 33.6993512 | 33.7008699 | 4.51e-05 |
| `fall-rate` | dt/dtau at the stop (grows without bound towards 2M; theory E / (1 - 2M/r)) | 1318.13752 | 1318.13752 | 0.00e+00 |
| `order-fourth` | slope of log error against log step, fourth-order composition (theory 4) | 3.99748103 | 4 | 6.30e-04 |
| `order-second` | the same for the plain leapfrog at equal cost (theory 2) | 2.00110193 | 2 | 5.51e-04 |
| `newton-closes` | Newton's orbit from the same apsides: turn after one orbit [rad] (theory 0) | 2.56594745e-12 | 0 | 2.57e-12 |
| `newton-period` | its period between periapses (theory 2 pi a^1.5 with a = 40) | 1589.53412 | 1589.53412 | 1.35e-12 |
| `newton-limit` | far-away period of the relativistic orbit over Kepler's, periapsis 40960 M (theory 1 + O(M/r)) | 1.00003662 | 1 | 3.66e-05 |
| `adapter-precession` | measured turn per lap after 200000 steps from r0 = 20, L = 4.2 [rad] (theory the elliptic integral) | 2.12709401 | 2.12709401 | 8.98e-10 |
| `adapter-drift` | largest abs(H / H0 - 1) over those steps (theory 0) | 2.22066809e-12 | 0 | 2.22e-12 |
| `adapter-deflection` | light b = 6: angle gained inside 30 M over the straight line, integrated [rad] (theory the quadrature) | 1.71952688 | 1.71952702 | 8.13e-08 |
| `adapter-plunge` | state of r0 = 40, L = 3.2 < sqrt 12 (theory 1: captured), while Newton's orbit stays | 1 | 1 | 0.00e+00 |
