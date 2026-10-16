#ifndef RENDER_H
#define RENDER_H

#include <stddef.h>

#include "relativity.h"

#define RENDER_COLS 78
#define RENDER_ROWS 28

/* Spacetime diagram of the rest frame: x to the right (0.25 ly per column), t upwards
 * (0.5 yr per row), centred on the rocket. '/' and '\' are the light cone of the origin,
 * '#' marks the destination at dest_x, '.' the past worldline, '@' the rocket.
 * Returns bytes written or 0 if cap is too small. */
size_t render_frame(const Rocket *r, const Rocket *trail, int trail_len, double dest_x, char *buf, size_t cap);

#endif
