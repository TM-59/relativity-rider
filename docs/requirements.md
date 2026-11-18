# Requirements (relativity-rider)

Taken from the current code and tests; the limits come from the README Tests section and the test programs.
The `make check` trace collects only CTest testcases (`tests/ctest/CMakeLists.txt`) named with an ID below (M61).
The mapping is a declared name, not a proof that the requirement is met.

### REQ-special-relativity
Hyperbolic motion in closed form (1e-9), the worldline on the hyperbola, time dilation at γ = 2, boosts preserve the interval,
velocity addition stays below c, the flip-and-burn trip stops at rest on the target; the browser adapter moves bit for bit
like the Rocket API over 1,800 steps.
Tests: `test_relativity`, `test_rider`.

### REQ-black-hole-orbits
Schwarzschild orbits: circular orbits, the innermost stable orbit at 6M, light capture below 3√3 M, light bending and the
periapsis turn against the quadrature/elliptic integral (1e-9), fourth-order convergence, Newton's limit.
Tests: `test_schwarzschild`.

### REQ-aberration
The view from the window: known aberration and Doppler values, β → 0 is the identity, composition like velocity addition,
the whole sky still 4π, the black body seen at D T.
Tests: `test_aberration`.

### REQ-courier
The story layer: Burn is `rocket_step` at 1 g bit for bit, scene order, every scene number, heavy scenes without a banner,
each choice trades one outcome for another, and the representative `docs/narrative/` rows byte for byte.
Tests: `test_courier`, `story_bench --check`.
