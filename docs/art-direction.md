# M4: a spacetime observatory with two foxes

2026-11-02. Direction: flat gouache children's-book style. A violet deep-sky painting sits behind
a darkened spacetime diagram; two fox characters carry the twin story: **Vela** rides the rocket
(orange suit, round helmet) and **Oren**, the twin, stays on Earth (glasses, knitted sweater).
Computed lines use the Okabe–Ito colour-blind-safe set: orange for the rocket's worldline, sky blue
for the light cone, yellow for the star, bluish green for Earth and reddish purple for the rocket's "now".

## Before and after

The terminal renderer draws the spacetime diagram as characters (`/` `\` light cone, `#` star,
`.` past worldline, `@` rocket) with no axis numbers or units. `docs/images/before-terminal.jpg`
captures the actual `--snapshot` output, which is still available, as are the GIF, CSV and SVG outputs.

The browser view draws the worldline computed by the same C model (WebAssembly) in the Earth frame,
with labelled axes (x in light-years, Earth time t in years), the light cone of the departure,
the star's worldline, the rocket's line of simultaneity, two clock cards and a "two clocks" chart.
The painting does not determine anything: it sits under a dark veil and the scene label says so.

## Generated assets: 7 generated, 3 accepted

Generator: Stable Diffusion 1.5 through stable-diffusion.cpp on the CPU of a local machine
(no API key, no paid service), 24 steps, CFG 7. Original designs; no third-party image, sound
or font files. Every generated image was inspected by eye before deciding.

Negative prompt for all: `text, letters, watermark, logo, signature, words, frame, border,
blurry, deformed, extra limbs, extra fingers, human, person`.

| Candidate | Seed, size | Decision |
|---|---|---|
| bg-a | 11, 768×448 | Rejected: calm, but pale border strips along the top and bottom edges and a washed-out lower half. |
| bg-b | 23, 768×448 | **Accepted** as `assets/art/deep-sky.jpg`: swirling violet sky with a bright yellow glow and a small amber planet, dark horizon band. No text or marks. Resized to 1024×597, JPEG q84 (51,602 B). Drawn under a 69 % dark veil behind the diagram. |
| rider-a | 5, 448×448 | Rejected: empty visor (no face), and the suit carries flag-like and agency-like patches and a text-like badge. |
| rider-b | 17, 448×448 | **Accepted** as `assets/art/rider-portrait.png` (Vela): a red fox face clearly visible in a white helmet, orange suit, star field; one small dotted patch without text. Resized to 256×256 PNG (83,929 B). Drawn in the ship-clock card and as the marker on the rocket's current event. |
| rider-c | 29, 448×448 | Rejected: two characters instead of one, the right one with malformed eyes. |
| twin-a | 7, 448×448 | Rejected: coherent knitted grey fox with glasses, but a long neck and a crochet texture that does not match Vela's flat style. |
| twin-b | 19, 448×448 | **Accepted** as `assets/art/twin-portrait.png` (Oren): orange fox with round glasses, knitted sweater, plain teal background, no text. The paws at the bottom are soft and fuzzy but not malformed. Resized to 256×256 PNG (92,136 B). Drawn in the Earth-clock card and as the marker on Earth at the same Earth time. |

Background prompt: `wide gouache painting of deep space seen from a quiet observatory, a single
bright warm yellow star on the far right, a tiny pale blue planet on the far left, soft violet and
indigo nebula, dark empty band across the middle, calm, painterly texture, muted palette, original
design, no logos, no text`. The accepted image does not follow the layout literally (the glow is
left of centre, the planet is amber); it was kept because it has no border or marks and stays
quiet under the veil.

Rider prompt: `cute small fox astronaut pilot character, head and shoulders portrait, white round
space helmet with clear visor, orange fur, big friendly eyes, teal space suit collar, centered,
plain cream background, gouache children's book illustration, original design, no logos, no text`.

Twin prompt: `cute small old fox character with grey whiskers and round glasses, head and shoulders
portrait, knitted green sweater, gentle smile, centered, plain cream background, gouache children's
book illustration, original design, no logos, no text`. The accepted twin does not look old; the
page therefore calls Oren "the twin who stays on Earth", not an older fox.

## Limits

The foxes are decoration. Their markers sit on computed events (the rocket's current (x, t) and
Earth at the same Earth time); they add no physics. No human first-play evaluation, real
mobile-device test or real-GPU measurement. No publication or submission.
