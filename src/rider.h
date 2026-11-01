#ifndef RIDER_H
#define RIDER_H

/* Browser adapter: one Rocket on the same model as the terminal game, read through plain doubles.
 * Each step advances DTAU of proper time with the exact closed form (rocket_step). */
enum {
    RR_TAU, RR_T, RR_X, RR_V, RR_GAMMA, RR_PHI, RR_THRUST, RR_MODE, RR_STEPS,
    RR_DEST, RR_ARRIVED, RR_HALF_TAU, RR_DTAU, RR_TRAIL, RR_DONE, RR_GAP, RR_MAX_V,
    RR_FIELDS
};
#define RR_TRAIL_MAX 4096
#define RR_TRAIL_EVERY 4
#define RR_DEMO_HALF 400 /* demo steps per half of the flip-and-burn trip */
#define RR_MAX_STEPS 8000

/* mode: 0 manual thrust, 1 scripted 1 g flip-and-burn trip to the star (controls locked). */
int rr_reset(int mode);
/* Manual thrust direction: -1 decelerate, 0 coast, 1 accelerate (about 1 g). Ignored in the demo. */
void rr_thrust(int sign);
void rr_step(int steps);
double rr_value(int field);
double rr_trail_t(int i); /* past worldline, every RR_TRAIL_EVERY steps */
double rr_trail_x(int i);

#endif
