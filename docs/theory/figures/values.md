# Measured values

Written by `make theory-figs` (tests/theory_figs.c) from the program's own functions. Do not edit by hand.
Units: c = 1, times in years, lengths in light-years. The last column is the relative error; where the theory column is 0 it is the absolute value.

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
