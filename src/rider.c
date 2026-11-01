#include "rider.h"
#include "relativity.h"

#include <math.h>

#define DEST 4.2

static Rocket rocket;
static int mode, steps, thrust, arrived, trail_len;
static double half_tau, dtau, max_v;
static double trail_t[RR_TRAIL_MAX], trail_x[RR_TRAIL_MAX];

static void record(void)
{
    if (trail_len < RR_TRAIL_MAX) {
        trail_t[trail_len] = rocket.t;
        trail_x[trail_len] = rocket.x;
        trail_len++;
    }
}

/* Same arrival rule as the terminal game: within 0.05 ly of the star, slower than 0.05 c. */
static int at_rest_on_star(void)
{
    return fabs(rocket.x - DEST) < 0.05 && fabs(velocity(&rocket)) < 0.05;
}

int rr_reset(int next_mode)
{
    mode = next_mode == 1 ? 1 : 0;
    rocket = (Rocket){0, 0, 0, 0};
    half_tau = acosh(1.0 + ONE_G * DEST / 2.0) / ONE_G;
    dtau = half_tau / RR_DEMO_HALF;
    steps = thrust = arrived = trail_len = 0;
    max_v = 0.0;
    record();
    return 0;
}

void rr_thrust(int sign)
{
    if (mode) return;
    thrust = sign > 0 ? 1 : sign < 0 ? -1 : 0;
}

static int finished(void)
{
    return arrived || steps >= RR_MAX_STEPS || (mode == 1 && steps >= 2 * RR_DEMO_HALF);
}

void rr_step(int count)
{
    if (count < 0) return;
    if (count > 400) count = 400;
    for (int i = 0; i < count && !finished(); i++) {
        double a = mode == 1 ? (steps < RR_DEMO_HALF ? ONE_G : -ONE_G) : thrust * ONE_G;
        rocket_step(&rocket, a, dtau);
        steps++;
        if (fabs(velocity(&rocket)) > max_v) max_v = fabs(velocity(&rocket));
        if (steps % RR_TRAIL_EVERY == 0) record();
        /* Manual arrival needs a deliberate stop, so do not count the departure moment. */
        if (mode == 0 && steps > 1 && at_rest_on_star()) arrived = 1;
    }
    if (mode == 1 && steps >= 2 * RR_DEMO_HALF && at_rest_on_star()) arrived = 1;
}

double rr_value(int field)
{
    switch (field) {
    case RR_TAU: return rocket.tau;
    case RR_T: return rocket.t;
    case RR_X: return rocket.x;
    case RR_V: return velocity(&rocket);
    case RR_GAMMA: return lorentz_gamma(&rocket);
    case RR_PHI: return rocket.phi;
    case RR_THRUST: return mode == 1 ? (steps < RR_DEMO_HALF ? 1 : -1) : thrust;
    case RR_MODE: return mode;
    case RR_STEPS: return steps;
    case RR_DEST: return DEST;
    case RR_ARRIVED: return arrived;
    case RR_HALF_TAU: return half_tau;
    case RR_DTAU: return dtau;
    case RR_TRAIL: return trail_len;
    case RR_DONE: return finished();
    case RR_GAP: return rocket.t - rocket.tau;
    case RR_MAX_V: return max_v;
    default: return 0.0;
    }
}

double rr_trail_t(int i) { return i >= 0 && i < trail_len ? trail_t[i] : 0.0; }
double rr_trail_x(int i) { return i >= 0 && i < trail_len ? trail_x[i] : 0.0; }
