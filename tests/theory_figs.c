/* Regenerates every plotted figure of docs/theory and the table of measured values
 * (docs/theory/figures/values.md). Run with `make theory-figs`. Each figure is produced by the
 * same model (src/relativity.c) and browser adapter (src/rider.c) the program uses; nothing here
 * changes them. */
#include "../src/aberration.h"
#include "../src/relativity.h"
#include "../src/orbit.h"
#include "../src/rider.h"
#include "../src/schwarzschild.h"
#include "../src/skyview.h"
#include "../src/svg.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXN 4096
#define DEST 4.2

static char dir[512];
static FILE *values;
static float xs[MAXN], ya[MAXN], yb[MAXN], yc[MAXN];

static const char *path(const char *name)
{
    static char buf[640];
    snprintf(buf, sizeof buf, "%s/%s", dir, name);
    return buf;
}

static void plot(const char *name, const char *title, const char *xl, const char *yl, int count, const Series *s, int ns)
{
    if (svg_plot(path(name), title, xl, yl, xs, count, s, ns) != 0) {
        fprintf(stderr, "cannot write %s\n", name);
        exit(1);
    }
}

/* One row of the measured-values table: id | what | measured | theory | error (relative; absolute when theory is 0). */
static void value(const char *id, const char *what, double measured, double theory)
{
    double err = theory != 0.0 ? fabs(measured / theory - 1.0) : fabs(measured - theory);
    fprintf(values, "| `%s` | %s | %.9g | %.9g | %.2e |\n", id, what, measured, theory, err);
}

/* S01, S02: one event seen from frames of every rapidity. */
static void fig_boost(void)
{
    int n = 0;
    double worst = 0.0, s0 = interval2(3.0, 1.5);
    for (double phi = -1.5; phi <= 1.5001; phi += 0.02, n++) {
        double t2, x2;
        boost(phi, 3.0, 1.5, &t2, &x2);
        xs[n] = (float)phi;
        ya[n] = (float)t2;
        yb[n] = (float)x2;
        yc[n] = (float)interval2(t2, x2);
        worst = fmax(worst, fabs(interval2(t2, x2) - s0));
    }
    double t2, x2, t3, x3;
    boost(0.7, 3.0, 1.5, &t2, &x2);
    boost(-0.7, t2, x2, &t3, &x3);
    value("interval-event", "interval t^2 - x^2 of the event (t, x) = (3, 1.5) (theory 9 - 2.25)", s0, 6.75);
    value("interval-invariance", "largest change of that interval over 151 boosts of rapidity -1.5 to 1.5 (theory 0)", worst, 0.0);
    value("boost-inverse", "distance from (3, 1.5) after a boost of 0.7 and a boost of -0.7 (theory 0)", hypot(t3 - 3.0, x3 - 1.5), 0.0);
    boost(atanh(0.5), 3.0, 1.5, &t2, &x2);
    value("boost-rest", "x' of the event in the frame moving at 0.5: the event lies on that frame's own worldline x = 0.5 t (theory 0)", x2, 0.0);
    value("boost-proper-time", "t' of the event in that frame: the proper time along x = 0.5 t (theory sqrt(6.75))", t2, sqrt(6.75));
    boost(0.9, 2.0, 2.0, &t2, &x2);
    value("light-stays-light", "x' / t' of an event on the light cone, (2, 2), after a boost of 0.9 (theory 1: light moves at c in every frame)", x2 / t2, 1.0);
    boost(0.4, 1.0, 0.0, &t2, &x2);
    boost(0.5, t2, x2, &t3, &x3);
    double t4, x4;
    boost(0.9, 1.0, 0.0, &t4, &x4);
    value("boosts-compose", "two boosts of rapidity 0.4 and 0.5 against one of 0.9, distance between the results (theory 0: rapidities add)", hypot(t3 - t4, x3 - x4), 0.0);
    Series s[] = {{"t'", ya, "#1f77b4"}, {"x'", yb, "#d62728"}, {"t'^2 - x'^2", yc, "#2ca02c"}};
    plot("boost.svg", "The event (t, x) = (3, 1.5) in frames of every rapidity", "rapidity of the frame", "yr, ly, yr^2", n, s, 3);
}

/* S03: rapidity, velocity, gamma, and the addition of velocities. */
static void fig_rapidity(void)
{
    int n = 0;
    for (double phi = 0.0; phi <= 3.0001; phi += 0.02, n++) {
        Rocket r = {0, 0, 0, phi};
        xs[n] = (float)phi;
        ya[n] = (float)velocity(&r);
        yb[n] = (float)fmin(phi, 1.2);
        yc[n] = (float)(lorentz_gamma(&r) / 10.0);
    }
    Rocket r = {0, 0, 0, 1.0};
    value("velocity-phi1", "velocity at rapidity 1 (theory tanh 1)", velocity(&r), tanh(1.0));
    value("gamma-phi1", "Lorentz factor at rapidity 1 (theory cosh 1 = 1 / sqrt(1 - v^2))", lorentz_gamma(&r), 1.0 / sqrt(1.0 - tanh(1.0) * tanh(1.0)));
    Series s[] = {{"v = tanh(phi)", ya, "#1f77b4"}, {"Newton: v = phi", yb, "#888888"}, {"gamma / 10", yc, "#d62728"}};
    plot("rapidity.svg", "Velocity and Lorentz factor against rapidity", "rapidity phi", "v / c, gamma / 10", n, s, 3);

    n = 0;
    for (double u = 0.0; u <= 1.0001; u += 0.01, n++) {
        xs[n] = (float)u;
        ya[n] = (float)add_velocity(u, u);
        yb[n] = (float)fmin(2.0 * u, 1.2);
        yc[n] = 1.0f;
    }
    value("add-0.9-0.9", "0.9 c combined with 0.9 c (theory 1.8 / 1.81)", add_velocity(0.9, 0.9), 1.8 / 1.81);
    value("add-light", "c combined with 0.5 c (theory 1)", add_velocity(1.0, 0.5), 1.0);
    value("add-rapidities", "rapidity of tanh 0.3 combined with tanh 0.5 (theory 0.8)", atanh(add_velocity(tanh(0.3), tanh(0.5))), 0.8);
    value("add-small", "0.001 c combined with 0.002 c over the Newtonian sum (theory 1 / (1 + 2e-6))", add_velocity(0.001, 0.002) / 0.003, 1.0 / 1.000002);
    Series a[] = {{"u combined with u", ya, "#1f77b4"}, {"2 u (Newton)", yb, "#888888"}, {"c", yc, "#d62728"}};
    plot("addition.svg", "A velocity combined with itself", "u / c", "result / c", n, a, 3);
}

/* S04, S05: time dilation while coasting, and the contracted distance. */
static void fig_dilation(void)
{
    Rocket fast = {0, 0, 0, acosh(2.0)}, slow = {0, 0, 0, atanh(0.5)};
    int n = 0;
    for (int k = 0; k <= 300; k++) {
        if (k % 2 == 0) { xs[n] = (float)fast.tau; ya[n] = (float)fast.t; yb[n] = (float)slow.t; yc[n] = (float)fast.tau; n++; }
        rocket_step(&fast, 0.0, 0.01);
        rocket_step(&slow, 0.0, 0.01);
    }
    value("dilation-gamma2", "Earth time elapsed while a coasting clock at gamma = 2 ticks 3.01 yr (theory 2 x 3.01)", fast.t, 6.02);
    value("dilation-speed", "x / t of that coaster (theory sqrt(3) / 2)", fast.x / fast.t, sqrt(3.0) / 2.0);
    value("dilation-interval", "sqrt(t^2 - x^2) of the coaster's path: the invariant interval is its own proper time (theory 3.01)", sqrt(interval2(fast.t, fast.x)), 3.01);
    value("dilation-half-c", "Earth time for 3.01 yr on a clock moving at 0.5 c (theory 3.01 / sqrt(0.75))", slow.t, 3.01 / sqrt(0.75));
    Series s[] = {{"Earth, gamma = 2", ya, "#d62728"}, {"Earth, v = 0.5 c", yb, "#1f77b4"}, {"clock at rest", yc, "#888888"}};
    plot("dilation.svg", "Earth time against a coasting clock's own time", "proper time on board [yr]", "Earth time [yr]", n, s, 3);

    /* The 4.2 ly to the star, measured in the frame of a rocket coasting at v: the two ends at the same rocket time. */
    n = 0;
    double worst = 0.0;
    for (double v = 0.0; v <= 0.9901; v += 0.01, n++) {
        double phi = atanh(v), t1, x1, t2, x2;
        /* Earth's worldline x = 0 and the star's x = 4.2; pick the star event simultaneous in the rocket frame with Earth's (0, 0): t = v x. */
        boost(phi, 0.0, 0.0, &t1, &x1);
        boost(phi, v * DEST, DEST, &t2, &x2);
        xs[n] = (float)v;
        ya[n] = (float)(x2 - x1);
        yb[n] = (float)(DEST * sqrt(1.0 - v * v));
        worst = fmax(worst, fabs(t2 - t1));
    }
    double t1, x1, t2, x2;
    boost(atanh(0.8), 0.0, 0.0, &t1, &x1);
    boost(atanh(0.8), 0.8 * DEST, DEST, &t2, &x2);
    value("contraction-0.8", "distance Earth-star in the frame of a rocket at 0.8 c, both ends at the same rocket time (theory 4.2 x 0.6)", x2 - x1, DEST * 0.6);
    value("contraction-simultaneous", "largest difference of the two rocket-frame times used for those lengths (theory 0: measured at one instant)", worst, 0.0);
    Series c[] = {{"from boost()", ya, "#1f77b4"}, {"4.2 sqrt(1 - v^2)", yb, "#d62728"}};
    plot("contraction.svg", "The 4.2 ly to the star, measured from a coasting rocket", "v / c", "distance [ly]", n, c, 2);
}

/* S06: what "now on Earth" means on board, along the demonstration trip. */
static void fig_simultaneity(void)
{
    rr_reset(1);
    int n = 0;
    double before = 0.0, after = 0.0, at_flip = 0.0;
    while (!rr_value(RR_DONE) && n < MAXN) {
        double t = rr_value(RR_T), x = rr_value(RR_X), v = rr_value(RR_V);
        xs[n] = (float)rr_value(RR_TAU);
        ya[n] = (float)t;                 /* Earth's clock now, by Earth's own simultaneity */
        yb[n] = (float)(t - v * x);       /* Earth's clock now, by the rocket's: the event (t - v x, 0) */
        yc[n] = (float)rr_value(RR_TAU);
        if ((int)rr_value(RR_STEPS) == RR_DEMO_HALF - 100) before = t - v * x;
        if ((int)rr_value(RR_STEPS) == RR_DEMO_HALF) at_flip = t - v * x;
        if ((int)rr_value(RR_STEPS) == RR_DEMO_HALF + 100) after = t - v * x;
        n++;
        rr_step(1);
    }
    double t = rr_value(RR_T), x = rr_value(RR_X), v = rr_value(RR_V), tb, xb;
    value("now-at-arrival", "Earth's clock simultaneous with the arrival in the rocket's frame, t - v x, with the rocket at rest again (theory: the Earth time of the arrival)", t - v * x, t);
    double half = rr_value(RR_HALF_TAU), th = sinh(ONE_G * half) / ONE_G;
    value("now-at-flip", "the same at the flip, halfway: half the Earth time has passed in Earth's frame, but by the rocket's simultaneity Earth's clock reads only this [yr] (theory t - v x with x = 2.1)", at_flip, th - tanh(ONE_G * half) * 2.1);
    value("now-flip-earth-frame", "Earth time of the flip in Earth's own frame [yr] (theory sinh(a tau_half) / a)", th, t / 2.0);
    value("now-round-flip", "Earth clock gained, by the rocket's simultaneity, during the 200 steps round the flip, per year on board", (after - before) / (200.0 * rr_value(RR_DTAU)), 0.0);
    value("now-second-half", "the same averaged over the whole second half, while the rocket slows down (it has to catch up to the Earth time of the arrival)", (t - at_flip) / half, 0.0);
    boost(atanh(0.6), 5.0, 0.0, &tb, &xb);
    double te, xe;
    boost(atanh(0.6), 5.0 + 0.6 * 3.0, 3.0, &te, &xe);
    value("now-line", "rocket-frame time difference between (5, 0) and (5 + 0.6 x 3, 3) at v = 0.6: events on the line t - t0 = v (x - x0) are simultaneous on board (theory 0)", te - tb, 0.0);
    Series s[] = {{"Earth frame", ya, "#d62728"}, {"on board: t - v x", yb, "#1f77b4"}, {"rocket's clock", yc, "#888888"}};
    plot("simultaneity.svg", "Earth's clock 'now' along the 1 g trip, by two definitions of now", "proper time on board [yr]", "years", n, s, 3);
}

/* S07: hyperbolic motion. */
static void fig_hyperbola(void)
{
    double a = ONE_G;
    Rocket r = {0, 0, 0, 0};
    int n = 0;
    double worst = 0.0, worst_t = 0.0, worst_v = 0.0;
    for (int k = 0; k <= 600; k++) {
        double h = (r.x + 1.0 / a) * (r.x + 1.0 / a) - r.t * r.t;
        worst = fmax(worst, fabs(h * a * a - 1.0));
        worst_t = fmax(worst_t, fabs(r.t - sinh(a * r.tau) / a));
        worst_v = fmax(worst_v, fabs(velocity(&r) - tanh(a * r.tau)));
        if (k % 3 == 0) {
            xs[n] = (float)r.t;
            ya[n] = (float)r.x;
            yb[n] = (float)fmax(r.t - 1.0 / a, 0.0);   /* the asymptote: light leaving x = -1/a at t = 0 */
            yc[n] = (float)(0.5 * a * r.t * r.t > 6.0 ? 6.0 : 0.5 * a * r.t * r.t);
            n++;
        }
        rocket_step(&r, a, 0.005);
    }
    value("hyperbola", "largest relative departure from (x + 1/a)^2 - t^2 = 1/a^2 over 3 yr of proper time at 1 g (theory 0)", worst, 0.0);
    value("hyperbola-t", "largest difference between t and sinh(a tau) / a over those 600 steps [yr] (theory 0)", worst_t, 0.0);
    value("hyperbola-v", "largest difference between v and tanh(a tau) (theory 0)", worst_v, 0.0);
    Rocket three = {0, 0, 0, 0};
    for (int k = 0; k < 600; k++) rocket_step(&three, a, 0.005);
    value("after-3yr-x", "distance covered in 3 yr of proper time at 1 g [ly] (theory (cosh(3a) - 1) / a)", three.x, (cosh(3.0 * a) - 1.0) / a);
    value("after-3yr-t", "Earth time for it [yr] (theory sinh(3a) / a)", three.t, sinh(3.0 * a) / a);
    value("horizon", "distance behind the start from which light never catches the rocket, 1 / a [ly]", 1.0 / a, 1.0 / ONE_G);
    Series s[] = {{"x(t) at 1 g", ya, "#1f77b4"}, {"light: t - 1/a", yb, "#d62728"}, {"Newton: a t^2 / 2", yc, "#888888"}};
    plot("hyperbola.svg", "Worldline at constant proper acceleration (Earth frame)", "t [yr]", "x [ly]", n, s, 3);

    /* v against Earth time: Newton's a t crosses c within a year, the rocket never does. */
    Rocket q = {0, 0, 0, 0};
    n = 0;
    for (int k = 0; k <= 400; k++) {
        if (k % 2 == 0) {
            xs[n] = (float)q.t;
            ya[n] = (float)velocity(&q);
            yb[n] = (float)fmin(a * q.t, 1.2);
            yc[n] = 1.0f;
            n++;
        }
        rocket_step(&q, a, 0.005);
    }
    Rocket y = {0, 0, 0, 0};
    for (int k = 0; k < 1000; k++) rocket_step(&y, a, 0.001);
    value("v-after-1yr", "speed after one year on board at 1 g (theory tanh(1.0323))", velocity(&y), tanh(ONE_G));
    value("one-g", "ONE_G against 9.80665 m/s^2 x one Julian year / c (theory 1.03227)", ONE_G, 9.80665 * 365.25 * 86400.0 / 299792458.0);
    Series p[] = {{"v(t) at 1 g", ya, "#1f77b4"}, {"Newton: a t", yb, "#888888"}, {"c", yc, "#d62728"}};
    plot("speed.svg", "Speed against Earth time at 1 g", "t [yr]", "v / c", n, p, 3);
}

/* S08: the step is exact, whatever its size. */
static void val_step(void)
{
    double a = ONE_G;
    Rocket one = {0, 0, 0, 0}, many = {0, 0, 0, 0}, mixed = {0, 0, 0, 0};
    rocket_step(&one, a, 2.0);
    for (int k = 0; k < 100000; k++) rocket_step(&many, a, 2.0 / 100000);
    value("step-size-x", "x after 2 yr at 1 g in 100000 steps against one step [ly] (exact step: 0 but for rounding)", many.x - one.x, 0.0);
    value("step-size-t", "the same for t [yr]", many.t - one.t, 0.0);
    /* accelerate, coast, decelerate: against the closed forms pieced together by hand */
    rocket_step(&mixed, a, 1.0);
    rocket_step(&mixed, 0.0, 0.5);
    rocket_step(&mixed, -a, 1.0);
    double x = (cosh(a) - 1.0) / a + sinh(a) * 0.5 + (cosh(a) - 1.0) / a;
    double t = sinh(a) / a + cosh(a) * 0.5 + sinh(a) / a;
    value("piecewise-x", "x after 1 yr at +1 g, 0.5 yr coasting, 1 yr at -1 g (theory: the three closed forms added)", mixed.x, x);
    value("piecewise-t", "t for the same (theory likewise)", mixed.t, t);
    value("piecewise-v", "velocity at the end (theory 0: equal burns cancel in rapidity)", velocity(&mixed), 0.0);
    /* the coasting branch joins the accelerating one continuously */
    Rocket c0 = {0, 0, 0, 0.8}, c1 = {0, 0, 0, 0.8};
    rocket_step(&c0, 0.0, 0.3);
    rocket_step(&c1, 1e-9, 0.3);
    value("coast-limit", "x after 0.3 yr with a = 1e-9 against a = 0 from rapidity 0.8 [ly] (theory 0; the closed form divides a rounding error by a)", c1.x - c0.x, 0.0);
    Rocket back = {0, 0, 0, 0};
    rocket_step(&back, a, 1.3);
    rocket_step(&back, a, -1.3);
    value("step-reverse", "distance from the start after a step of +1.3 yr and one of -1.3 yr (theory 0)", hypot(back.t, back.x), 0.0);
}

/* S09: the flip-and-burn trip against distance. */
static void fig_trip(void)
{
    double a = ONE_G;
    int n = 0;
    for (double d = 0.1; d <= 20.0001; d += 0.1, n++) {
        double half = acosh(1.0 + a * d / 2.0) / a;
        Rocket r = {0, 0, 0, 0};
        rocket_step(&r, a, half);
        rocket_step(&r, -a, half);
        xs[n] = (float)d;
        ya[n] = (float)r.t;
        yb[n] = (float)r.tau;
        yc[n] = (float)d;   /* light */
    }
    double half = acosh(1.0 + a * DEST / 2.0) / a;
    Rocket r = {0, 0, 0, 0};
    rocket_step(&r, a, half);
    double vmax = velocity(&r), gmax = lorentz_gamma(&r);
    rocket_step(&r, -a, half);
    value("trip-distance", "where a 1 g flip-and-burn planned for 4.2 ly ends [ly] (theory 4.2)", r.x, DEST);
    value("trip-ship-time", "years on board (theory (2 / a) acosh(1 + a D / 2))", r.tau, 2.0 * acosh(1.0 + a * DEST / 2.0) / a);
    value("trip-earth-time", "years on Earth (theory (2 / a) sinh(a tau_half) = sqrt(D^2 + 4 D / a))", r.t, sqrt(DEST * DEST + 4.0 * DEST / a));
    value("trip-top-speed", "speed at the flip (theory tanh(a tau_half))", vmax, tanh(a * half));
    value("trip-top-gamma", "Lorentz factor at the flip (theory 1 + a D / 2)", gmax, 1.0 + a * DEST / 2.0);
    value("trip-end-speed", "speed at the end (theory 0)", velocity(&r), 0.0);
    value("trip-vs-light", "Earth time over the light-travel time 4.2 yr (always above 1)", r.t / DEST, 1.0);
    Series s[] = {{"Earth time", ya, "#d62728"}, {"ship time", yb, "#1f77b4"}, {"light", yc, "#888888"}};
    plot("trip.svg", "1 g flip-and-burn: years against distance", "distance [ly]", "years", n, s, 3);
}

/* S11..S13: the browser adapter's demonstration and its clocks. */
static void fig_demo(void)
{
    rr_reset(1);
    int n = 0;
    while (!rr_value(RR_DONE) && n < MAXN) {
        xs[n] = (float)rr_value(RR_TAU);
        ya[n] = (float)rr_value(RR_V);
        yb[n] = (float)rr_value(RR_GAP);
        yc[n] = (float)(rr_value(RR_X) / DEST);
        n++;
        rr_step(1);
    }
    value("demo-steps", "steps of the demonstration (theory 2 x 400)", rr_value(RR_STEPS), 800.0);
    value("demo-dtau", "proper time per step [yr] (theory tau_half / 400)", rr_value(RR_DTAU), acosh(1.0 + ONE_G * DEST / 2.0) / ONE_G / 400.0);
    value("demo-arrived", "RR_ARRIVED at the end (theory 1: within 0.05 ly and below 0.05 c)", rr_value(RR_ARRIVED), 1.0);
    value("demo-ship", "RR_TAU at the end [yr] (theory (2 / a) acosh(1 + a D / 2))", rr_value(RR_TAU), 2.0 * acosh(1.0 + ONE_G * DEST / 2.0) / ONE_G);
    value("demo-earth", "RR_T at the end [yr] (theory sqrt(D^2 + 4 D / a))", rr_value(RR_T), sqrt(DEST * DEST + 4.0 * DEST / ONE_G));
    value("demo-gap", "RR_GAP, Earth's clock ahead of the ship's at the end [yr] (theory the difference of the two)", rr_value(RR_GAP), sqrt(DEST * DEST + 4.0 * DEST / ONE_G) - 2.0 * acosh(1.0 + ONE_G * DEST / 2.0) / ONE_G);
    value("demo-max-v", "RR_MAX_V (theory tanh(a tau_half))", rr_value(RR_MAX_V), tanh(acosh(1.0 + ONE_G * DEST / 2.0)));
    value("demo-trail", "worldline points recorded (theory 1 + 800 / 4)", rr_value(RR_TRAIL), 201.0);
    value("demo-trail-end", "x of the last recorded point [ly] (theory 4.2)", rr_trail_x((int)rr_value(RR_TRAIL) - 1), DEST);
    value("demo-trail-end-t", "t of the last recorded point [yr] (theory the Earth time of the trip)", rr_trail_t((int)rr_value(RR_TRAIL) - 1), sqrt(DEST * DEST + 4.0 * DEST / ONE_G));
    rr_thrust(1);
    rr_step(50);
    value("demo-locked", "steps taken after the end, with thrust asked for (theory 0: finished, controls locked)", rr_value(RR_STEPS) - 800.0, 0.0);
    Series s[] = {{"v / c", ya, "#1f77b4"}, {"Earth ahead [yr]", yb, "#d62728"}, {"x / 4.2 ly", yc, "#888888"}};
    plot("demo.svg", "The demonstration trip from the browser adapter", "proper time on board [yr]", "v / c, years, fraction of the way", n, s, 3);

    /* Manual mode: thrust for 100 steps, coast 100, and the limits. */
    rr_reset(0);
    value("manual-start", "RR_ARRIVED right after a reset (theory 0)", rr_value(RR_ARRIVED), 0.0);
    rr_thrust(1);
    rr_step(100);
    double phi = rr_value(RR_PHI);
    value("manual-rapidity", "rapidity after 100 steps of Accelerate (theory 1 g x 100 dtau)", phi, ONE_G * 100.0 * rr_value(RR_DTAU));
    rr_thrust(0);
    rr_step(100);
    value("manual-coast", "rapidity change over 100 coasting steps (theory 0)", rr_value(RR_PHI) - phi, 0.0);
    rr_thrust(-1);
    rr_step(100);
    value("manual-stop", "velocity after 100 steps of Decelerate (theory 0)", rr_value(RR_V), 0.0);
    rr_step(5000);
    value("manual-clamp", "steps taken by one call asking for 5000 (theory 400: one call is clamped)", rr_value(RR_STEPS) - 300.0, 400.0);
    rr_reset(0);
    for (int k = 0; k < 30; k++) rr_step(400);
    value("manual-limit", "steps after 30 calls of 400 with no thrust (theory RR_MAX_STEPS 8000)", rr_value(RR_STEPS), 8000.0);
    value("manual-never-arrives", "RR_ARRIVED for a rocket that never left (theory 0)", rr_value(RR_ARRIVED), 0.0);
}

/* ---------------------------------------------------------------- page 5: the black hole (G = c = M = 1) */

static float yd[MAXN];

static double periapsis(Orbit *o, double h, int max)
{
    for (int k = 0; k < max; k++) {
        Orbit before = *o;
        geo_step(o, h);
        if (before.pr < 0.0 && o->pr >= 0.0) {
            *o = before;
            geo_step(o, geo_apsis_step(&before, h));
            return o->phi;
        }
    }
    return -1.0;
}

static Orbit light_from(double b, double r0)
{
    Orbit o;
    geo_init(&o, GEO_LIGHT, 1.0, b, r0, 0.0);
    o.E = 1.0;
    o.pr = -sqrt(1.0 - 2.0 * geo_potential(GEO_LIGHT, 1.0, b, r0));
    return o;
}

/* S19: E and L read back from the integrated t and phi. */
static void fig_geo_conserved(void)
{
    Orbit o;
    geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
    double H0 = geo_energy(&o), worst = 0.0, eworst = 0.0, lworst = 0.0;
    int n = 0;
    for (int k = 0; k < 160000; k++) {
        Orbit before = o;
        geo_step(&o, 0.4);
        worst = fmax(worst, fabs(geo_energy(&o) / H0 - 1.0));
        double rm = 0.5 * (before.r + o.r), e = (1.0 - 2.0 / rm) * (o.t - before.t) / 0.4, l = rm * rm * (o.phi - before.phi) / 0.4;
        if (k < 4800) {
            eworst = fmax(eworst, fabs(e / o.E - 1.0));
            lworst = fmax(lworst, fabs(l / o.L - 1.0));
            if (k % 8 == 0 && n < MAXN) { xs[n] = (float)o.s; ya[n] = (float)(e / o.E); yb[n] = (float)(l / o.L); yc[n] = (float)(o.r / 30.0); n++; }
        }
    }
    double u1 = 1.0 / 30.0, u2 = 0.1, u3 = 0.5 - u1 - u2, L2 = 1.0 / (u1 * u2 + u1 * u3 + u2 * u3);
    value("geo-e", "E of the orbit with periapsis 10 M and apoapsis 30 M, from 2V(10) (theory E^2 = 1 - 2M u1 u2 u3 L^2 from the turning points)", o.E, sqrt(1.0 - 2.0 * u1 * u2 * u3 * L2));
    value("geo-l", "its L (theory 1 / sqrt(u1 u2 + u1 u3 + u2 u3), u3 = 1/2M - u1 - u2)", o.L, sqrt(L2));
    value("geo-e-fd", "largest abs((1 - 2M/r) dt/dtau / E - 1) from finite differences over 3 orbits, step 0.4 (theory 0; the midpoint difference is second order)", eworst, 0.0);
    value("geo-l-fd", "largest abs(r^2 dphi/dtau / L - 1) from the same differences (theory 0)", lworst, 0.0);
    value("geo-energy-100", "largest abs(H / H0 - 1) over 160000 steps (about 100 orbits) of 0.4 (theory 0; bounded, it does not grow)", worst, 0.0);
    Series s[] = {{"E read back / E", ya, "#d62728"}, {"L read back / L", yb, "#1f77b4"}, {"r / 30 M", yc, "#888888"}};
    plot("geo-conserved.svg", "E and L read back from the integrated clocks and angle", "proper time [M]", "ratio", n, s, 3);
}

/* S20: the effective potential, relativistic and Newtonian. */
static void fig_geo_potential(void)
{
    int n = 0;
    for (double r = 3.0; r <= 40.0001 && n < MAXN; r += 0.05, n++) {
        xs[n] = (float)r;
        ya[n] = (float)(2.0 * geo_potential(GEO_MASSIVE, 1.0, 3.0, r));
        yb[n] = (float)(2.0 * geo_potential(GEO_MASSIVE, 1.0, sqrt(12.0), r));
        yc[n] = (float)(2.0 * geo_potential(GEO_MASSIVE, 1.0, 4.2, r));
        yd[n] = (float)fmin(1.1, 1.0 + 2.0 * geo_potential(GEO_NEWTON, 1.0, 4.2, r));
    }
    double worst = 0.0;
    for (int kind = 0; kind < 3; kind++)
        for (double r = 2.5; r < 60.0; r += 0.37) {
            double e = 1e-5 * r, num = -(geo_potential(kind, 1.0, 4.0, r + e) - geo_potential(kind, 1.0, 4.0, r - e)) / (2.0 * e);
            worst = fmax(worst, fabs(num - geo_force(kind, 1.0, 4.0, r)) / fmax(1e-3, fabs(geo_force(kind, 1.0, 4.0, r))));
        }
    value("potential-force", "largest relative gap between geo_force and -dV/dr by central differences, 3 kinds, r = 2.5..60 (theory 0, up to the difference error)", worst, 0.0);
    value("potential-extra", "2V_GR - (1 + 2V_Newton) at r = 9, L = 4 (theory -2 M L^2 / r^3)",
          2.0 * geo_potential(GEO_MASSIVE, 1.0, 4.0, 9.0) - 1.0 - 2.0 * geo_potential(GEO_NEWTON, 1.0, 4.0, 9.0), -32.0 / 729.0);
    Orbit o;
    geo_init(&o, GEO_MASSIVE, 1.0, 4.2, 20.0, 0.0);
    value("potential-level", "E^2 of the browser's default start (r0 = 20, L = 4.2, at rest radially) (theory 2V(20))", o.E * o.E, 2.0 * geo_potential(GEO_MASSIVE, 1.0, 4.2, 20.0));
    Series s[] = {{"L = 3", ya, "#888888"}, {"L = sqrt 12", yb, "#009e73"}, {"L = 4.2", yc, "#e69f00"}, {"Newton 4.2", yd, "#0072b2"}};
    plot("potential.svg", "Effective potential 2V(r) = (1 - 2M/r)(1 + L^2/r^2)", "r [M]", "2V (E^2 for a turning point)", n, s, 4);
}

/* S21: circular orbits and Kepler's third law in far-away time. */
static void fig_geo_circular(void)
{
    int n = 0;
    for (double L = sqrt(12.0); L <= 6.0001 && n < MAXN; L += 0.01, n++) {
        xs[n] = (float)L;
        ya[n] = (float)geo_circular_radius(1.0, L);
        yb[n] = (float)geo_unstable_radius(1.0, L);
        yc[n] = 6.0f;
    }
    double r = 10.0, L = geo_circular_L(1.0, r), h = 2.0 * 3.14159265358979323846 * pow(r, 1.5) / 800.0, dev = 0.0;
    Orbit o;
    geo_init(&o, GEO_MASSIVE, 1.0, L, r, 0.0);
    for (int k = 0; k < 8000; k++) { geo_step(&o, h); dev = fmax(dev, fabs(o.r - r)); }
    value("circular-l", "L of the circular orbit at 10 M (theory sqrt(M r^2 / (r - 3M)) = sqrt(100/7))", L, sqrt(100.0 / 7.0));
    value("circular-radius", "geo_circular_radius(L) for that L [M] (theory 10)", geo_circular_radius(1.0, L), 10.0);
    value("circular-stays", "largest abs(r - 10) over 10 orbits integrated from the circle (theory 0)", dev, 0.0);
    value("circular-kepler", "dphi/dt measured over 10 orbits at 10 M (theory sqrt(M / r^3), Kepler's law in far-away time)", o.phi / o.t, sqrt(1e-3));
    value("circular-energy", "E of that orbit (theory (1 - 2M/r) / sqrt(1 - 3M/r))", o.E, 0.8 / sqrt(0.7));
    Series s[] = {{"stable r+", ya, "#e69f00"}, {"unstable r-", yb, "#cc79a7"}, {"6M", yc, "#009e73"}};
    plot("circular.svg", "Circular orbits: radius against angular momentum", "L [M]", "r [M]", n, s, 3);
}

/* S22: the innermost stable circular orbit. */
static void fig_geo_isco(void)
{
    double rmin = 50.0;
    for (double x = 3.01; x < 50.0; x += 0.0001)
        if (geo_circular_L(1.0, x) < geo_circular_L(1.0, rmin)) rmin = x;
    value("isco-scan", "radius of the least angular momentum among circular orbits, scanned in steps of 1e-4 M (theory 6M)", rmin, 6.0);
    value("isco-l", "that least L [M] (theory sqrt 12)", geo_circular_L(1.0, rmin), sqrt(12.0));
    double e = 1e-3, v2 = -(geo_force(GEO_MASSIVE, 1.0, sqrt(12.0), 6.0 + e) - geo_force(GEO_MASSIVE, 1.0, sqrt(12.0), 6.0 - e)) / (2.0 * e);
    value("isco-flat", "V''(6M) at L^2 = 12 by central differences (theory 0: the minimum and the maximum merge)", v2, 0.0);
    Orbit a, b;
    geo_init(&a, GEO_MASSIVE, 1.0, geo_circular_L(1.0, 7.0), 7.0, 0.0);
    geo_init(&b, GEO_MASSIVE, 1.0, geo_circular_L(1.0, 5.0), 5.0, 0.0);
    a.pr = 0.01;
    b.pr = -0.001;
    int n = 0;
    double lo = 1e9, hi = 0.0;
    for (int k = 0; k < 24000; k++) {
        geo_step(&a, 0.05);
        geo_step(&b, 0.05);
        lo = fmin(lo, a.r);
        hi = fmax(hi, a.r);
        if (k % 12 == 0 && n < MAXN) { xs[n] = (float)a.s; ya[n] = (float)a.r; yb[n] = (float)(b.state == GEO_MOVING ? b.r : 2.0); yc[n] = 6.0f; n++; }
    }
    double k2 = -(geo_force(GEO_MASSIVE, 1.0, geo_circular_L(1.0, 7.0), 7.0 + e) - geo_force(GEO_MASSIVE, 1.0, geo_circular_L(1.0, 7.0), 7.0 - e)) / (2.0 * e);
    value("isco-outside", "width max r - min r of the circle at 7M nudged by dr/dtau = 0.01, over 1200 M [M] (theory 2 x 0.01 / sqrt(V''(7)), small oscillation)", hi - lo, 0.02 / sqrt(k2));
    value("isco-inside", "state of the circle at 5M nudged inward by 0.001 after 1200 M (theory 1: fell in)", b.state, 1.0);
    Series s[] = {{"from 7M", ya, "#e69f00"}, {"from 5M", yb, "#cc79a7"}, {"6M", yc, "#009e73"}};
    plot("isco.svg", "Inside 6M a circular orbit does not survive a nudge", "proper time [M]", "r [M]", n, s, 3);
}

/* S23: light, the photon sphere and the critical impact parameter. */
static void fig_geo_light(void)
{
    static float lx[6][MAXN], ly[6][MAXN];
    static const double bs[6] = {4.5, 5.0, 5.19, 5.21, 6.0, 8.0};
    static const char *names[6] = {"b = 4.5 M", "b = 5.0 M", "b = 5.19 M", "b = 5.21 M", "b = 6 M", "b = 8 M"};
    static const char *colours[6] = {"#888888", "#56b4e9", "#cc79a7", "#d55e00", "#e69f00", "#009e73"};
    XYSeries s[6];
    for (int i = 0; i < 6; i++) {
        Orbit o = light_from(bs[i], 30.0);
        o.phi = 3.14159265358979323846 + asin(bs[i] / 30.0);
        int n = 0;
        for (int k = 0; k < 200000 && o.state == GEO_MOVING && !(o.pr > 0.0 && o.r > 32.0); k++) {
            geo_step(&o, 0.01);
            if (k % 20 == 0 && n < MAXN) { lx[i][n] = (float)(o.r * cos(o.phi)); ly[i][n] = (float)(o.r * sin(o.phi)); n++; }
        }
        s[i] = (XYSeries){names[i], lx[i], ly[i], n, colours[i], 0};
    }
    Ring rings[] = {{"horizon 2M", 2.0, "#111111", 1}, {"photon sphere 3M", 3.0, "#cc79a7", 0}};
    if (svg_xy(path("light.svg"), "Light passing a black hole (b_c = 5.196 M)", "M", 16.0, s, 6, rings, 2)) exit(1);
    double best = 0.0, at = 0.0;
    for (double r = 2.1; r < 10.0; r += 1e-5) {
        double v = geo_potential(GEO_LIGHT, 1.0, 1.0, r);
        if (v > best) { best = v; at = r; }
    }
    value("photon-peak", "radius where the light potential peaks, scanned in steps of 1e-5 M (theory 3M)", at, 3.0);
    value("photon-bcrit", "1 / sqrt(2V) at that peak (theory 3 sqrt 3 M = 5.196)", 1.0 / sqrt(2.0 * best), geo_critical_b(1.0));
    double turns[2];
    for (int side = 0; side < 2; side++) {
        Orbit o = light_from(geo_critical_b(1.0) * (side ? 1.001 : 0.999), 40.0);
        for (int k = 0; k < 400000 && o.state == GEO_MOVING && !(o.pr > 0.0 && o.r > 40.0); k++) geo_step(&o, 0.01);
        turns[side] = side ? o.phi : o.state;
    }
    value("light-below", "state of light with b = 0.999 b_c (theory 1: captured)", turns[0], 1.0);
    value("light-above", "angle swept by light with b = 1.001 b_c from 40M and back out [rad] (theory column 2 pi: it winds more than once round)", turns[1], 2.0 * 3.14159265358979323846);
}

/* S24: the bending of light far from the hole. */
static void fig_geo_deflection(void)
{
    int n = 0;
    double worst = 0.0;
    for (double b = 8.0; b <= 400.0 && n < MAXN; b *= 1.15, n++) {
        Orbit o = light_from(b, 30.0 * b);
        double total = geo_swept_angle(&o, INFINITY) - 3.14159265358979323846, exact = geo_swept_angle(&o, 30.0 * b);
        double swept = -1.0;
        for (int k = 0; k < 10000000 && o.state == GEO_MOVING; k++) {
            Orbit before = o;
            geo_step(&o, 0.004 * b);
            if (before.r < 30.0 * b && o.r >= 30.0 * b) { swept = before.phi + (o.phi - before.phi) * (30.0 * b - before.r) / (o.r - before.r); break; }
        }
        worst = fmax(worst, fabs(swept - exact));
        xs[n] = (float)(1.0 / b);
        ya[n] = (float)(total / (4.0 / b));
        yb[n] = (float)(1.0 + 15.0 * 3.14159265358979323846 / 16.0 / b);
        yc[n] = 1.0f;
    }
    Orbit o = light_from(1000.0, 1e6);
    double total = geo_swept_angle(&o, INFINITY) - 3.14159265358979323846;
    value("deflection-1000", "deflection of light passing at b = 1000 M [rad] (theory 4M/b, plus (15 pi / 4)(M/b)^2 = 1.18e-5)", total, 4e-3);
    value("deflection-1000-two", "the same against 4M/b + (15 pi / 4)(M/b)^2", total, 4e-3 + 15.0 * 3.14159265358979323846 / 4.0 * 1e-6);
    o = light_from(6.0, 1e6);
    value("deflection-6", "deflection at b = 6 M, close to the photon sphere [rad] (4M/b = 0.667 is far off)", geo_swept_angle(&o, INFINITY) - 3.14159265358979323846, 4.0 / 6.0);
    value("deflection-sun", "deflection by the Sun at its rim, M = 1476.6 m, b = 6.957e8 m [arcsec] (theory 1.75)", 4.0 * 1476.625 / 6.957e8 * 206264.806, 1.75);
    value("deflection-quadrature", "largest gap between the integrated sweep (step 0.004 b, linear crossing) and the quadrature, b = 8..400 M [rad] (theory 0)", worst, 0.0);
    Series s[] = {{"exact", ya, "#e69f00"}, {"1+(15pi/16)M/b", yb, "#0072b2"}, {"4M/b", yc, "#888888"}};
    plot("deflection.svg", "Bending of light against the far-field 4M/b", "M / b", "deflection / (4M/b)", n, s, 3);
}

/* S25: the turning of the orbit (periapsis precession). */
static void fig_geo_precession(void)
{
    int n = 0;
    double worst = 0.0;
    for (double rp = 8.0; rp <= 4000.0 && n < MAXN; rp *= 1.3, n++) {
        double ra = 2.0 * rp, a = 1.5 * rp, e = 1.0 / 3.0, p = a * (1.0 - e * e), weak = geo_precession_weak(1.0, a, e);
        Orbit o;
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, rp, ra);
        double exact = geo_precession_exact(1.0, rp, ra), measured = periapsis(&o, 2.0 * 3.14159265358979323846 * pow(rp, 1.5) / 2000.0, 1000000) - 2.0 * 3.14159265358979323846;
        worst = fmax(worst, fabs(measured - exact));
        xs[n] = (float)(1.0 / p);
        ya[n] = (float)(exact / weak);
        yb[n] = (float)(measured / weak);
        yc[n] = (float)(1.0 + (18.0 + e * e) / 4.0 / p);
    }
    Series s[] = {{"elliptic", ya, "#e69f00"}, {"integrated", yb, "#d62728"}, {"1+(18+e^2)M/4p", yc, "#0072b2"}};
    plot("precession.svg", "Periapsis turn per orbit against 6 pi M / p (e = 1/3)", "M / p", "turn / (6 pi M / p)", n, s, 3);
    value("precession-integrated", "largest gap between the integrated turn and the elliptic integral, periapsis 8..4000 M [rad] (theory 0)", worst, 0.0);
    Orbit o;
    geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
    value("precession-10-30", "turn per orbit between 10 M and 30 M, quadrature [rad] (theory the elliptic integral)", geo_swept_angle(&o, 0.0) - 2.0 * 3.14159265358979323846, geo_precession_exact(1.0, 10.0, 30.0));
    value("precession-weak-10-30", "the weak-field formula for that orbit [rad] (a = 20, e = 0.5): strong field, 32% short", geo_precession_weak(1.0, 20.0, 0.5), geo_precession_exact(1.0, 10.0, 30.0));
    double rp = 1e5, exact = geo_precession_exact(1.0, rp, 2.0 * rp), weak = geo_precession_weak(1.0, 1.5 * rp, 1.0 / 3.0);
    value("precession-first-order", "(turn / weak - 1) p at p = 1.33e5 M (theory (18 + e^2)/4 = 4.528)", (exact / weak - 1.0) * (1.5 * rp * 8.0 / 9.0), (18.0 + 1.0 / 9.0) / 4.0);
    double M = 1476.625, a = 5.7909e10, e = 0.205630, laps = 36525.0 / 87.9691;
    value("precession-mercury", "Mercury: weak-field turn per century [arcsec] (M_sun = 1476.6 m, a = 5.79e10 m, e = 0.2056; theory 42.98)",
          geo_precession_weak(M, a, e) * laps * 206264.806, 42.98);
    static float gx[MAXN], gy[MAXN], nx[MAXN], ny[MAXN];
    Orbit g, w;
    geo_from_apsides(&g, GEO_MASSIVE, 1.0, 10.0, 30.0);
    geo_from_apsides(&w, GEO_NEWTON, 1.0, 10.0, 30.0);
    int m = 0;
    for (int k = 0, laps = 0; laps < 3 && m < MAXN; k++) {
        Orbit before = g;
        geo_step(&g, 0.4);
        geo_step(&w, 0.4);
        laps += before.pr < 0.0 && g.pr >= 0.0;
        if (k % 2 == 0) { gx[m] = (float)(g.r * cos(g.phi)); gy[m] = (float)(g.r * sin(g.phi)); nx[m] = (float)(w.r * cos(w.phi)); ny[m] = (float)(w.r * sin(w.phi)); m++; }
    }
    XYSeries xy[] = {{"general relativity", gx, gy, m, "#e69f00", 0}, {"Newton, same apsides", nx, ny, m, "#0072b2", 1}};
    Ring rings[] = {{"horizon 2M", 2.0, "#111111", 1}, {"innermost stable 6M", 6.0, "#009e73", 0}};
    if (svg_xy(path("rosette.svg"), "Periapsis 10 M, apoapsis 30 M: three orbits", "M", 32.0, xy, 2, rings, 2)) exit(1);
}

/* S26: clocks near the hole. */
static void fig_geo_clocks(void)
{
    int n = 0;
    double worst = 0.0;
    for (double r = 3.2; r <= 40.0001 && n < MAXN; r += 0.4, n++) {
        Orbit o;
        geo_init(&o, GEO_MASSIVE, 1.0, geo_circular_L(1.0, r), r, 0.0);
        double h = 2.0 * 3.14159265358979323846 * pow(r, 1.5) / 400.0;
        for (int k = 0; k < 400; k++) geo_step(&o, h);
        double v = geo_local_speed(1.0, r);
        Rocket local = {0.0, 0.0, 0.0, atanh(v)};
        worst = fmax(worst, fabs(o.s / o.t - geo_static_rate(1.0, r) / lorentz_gamma(&local)));
        xs[n] = (float)r;
        ya[n] = (float)geo_static_rate(1.0, r);
        yb[n] = (float)geo_circular_rate(1.0, r);
        yc[n] = (float)(o.s / o.t);
        yd[n] = (float)v;
    }
    value("clock-split", "largest gap between the integrated rate dtau/dt on circular orbits (one lap, r = 3.2..40) and sqrt(1 - 2M/r) / gamma(v_local) (theory 0)", worst, 0.0);
    value("clock-static-6", "a clock held at 6M against the far-away clock (theory sqrt(2/3))", geo_static_rate(1.0, 6.0), sqrt(2.0 / 3.0));
    value("clock-orbit-6", "a clock on the circular orbit at 6M (theory sqrt(1/2))", geo_circular_rate(1.0, 6.0), sqrt(0.5));
    value("clock-local-6", "speed of that orbit measured by a static observer at 6M (theory 1/2)", geo_local_speed(1.0, 6.0), 0.5);
    value("clock-earth", "a clock on the ground against one far away: 1 - rate, M = 4.435e-3 m, r = 6.371e6 m (theory 6.96e-10)", 1.0 - geo_static_rate(4.435e-3, 6.371e6), 6.96e-10);
    Series s[] = {{"at rest", ya, "#0072b2"}, {"orbit", yb, "#e69f00"}, {"integrated", yc, "#d62728"}, {"speed v", yd, "#888888"}};
    plot("gr-clocks.svg", "Clock rates against a far-away clock", "r [M]", "dtau/dt, v / c", n, s, 4);
}

/* S27: falling in. */
static void fig_geo_fall(void)
{
    Orbit o;
    geo_init(&o, GEO_MASSIVE, 1.0, 0.0, 10.0, 0.0);
    int n = 0;
    double tau3 = -1.0;
    for (int k = 0; k < 1000000 && o.state == GEO_MOVING; k++) {
        Orbit before = o;
        geo_step(&o, 1e-3);
        if (before.r > 3.0 && o.r <= 3.0) tau3 = before.s + 1e-3 * (before.r - 3.0) / (before.r - o.r);
        if (k % 40 == 0 && n < MAXN) { xs[n] = (float)o.r; ya[n] = (float)o.s; yb[n] = (float)geo_fall_time(1.0, 10.0, o.r); yc[n] = (float)fmin(o.t, 60.0); n++; }
    }
    value("fall-3", "proper time of a fall from rest at 10M down to 3M (theory the cycloid sqrt(R^3/2M)(sqrt(q(1-q)) + acos sqrt q))", tau3, geo_fall_time(1.0, 10.0, 3.0));
    value("fall-horizon", "proper time when the fall reaches 2.002M and stops (theory just under the time to 2M)", o.s, geo_fall_time(1.0, 10.0, 2.0));
    value("fall-rate", "dt/dtau at the stop (grows without bound towards 2M; theory E / (1 - 2M/r))", geo_rate(&o), sqrt(0.8) / (1.0 - 2.0 / o.r));
    Series s[] = {{"tau integrated", ya, "#e69f00"}, {"tau closed form", yb, "#0072b2"}, {"t far away", yc, "#d62728"}};
    plot("fall.svg", "Radial fall from rest at 10 M", "r [M]", "time [M]", n, s, 3);
}

/* S28: the integrator: order and energy. */
static void fig_geo_order(void)
{
    double exact = geo_precession_exact(1.0, 10.0, 30.0) + 2.0 * 3.14159265358979323846;
    int n = 0;
    double e4[8], e2[8];
    for (int i = 0; i < 8; i++, n++) {
        double h = 1.6 / pow(2.0, 0.5 * i);
        Orbit o;
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
        e4[i] = fabs(periapsis(&o, h, 10000000) - exact);
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
        double phi = 0.0;
        for (int k = 0; k < 10000000; k++) {
            Orbit before = o;
            geo_leapfrog(&o, h / 3.0); /* three leapfrog steps cost what one fourth-order step costs */
            if (before.pr < 0.0 && o.pr >= 0.0) {
                Orbit at = before; /* refine with the leapfrog itself */
                double lo = 0.0, hi = h / 3.0;
                for (int j = 0; j < 60; j++) { double mid = 0.5 * (lo + hi); at = before; geo_leapfrog(&at, mid); if (at.pr < 0.0) lo = mid; else hi = mid; }
                phi = at.phi;
                break;
            }
        }
        e2[i] = fabs(phi - exact);
        xs[n] = (float)log10(h);
        ya[n] = (float)log10(e4[i]);
        yb[n] = (float)log10(e2[i]);
    }
    value("order-fourth", "slope of log error against log step, fourth-order composition (theory 4)", log10(e4[2] / e4[6]) / log10(4.0), 4.0);
    value("order-second", "the same for the plain leapfrog at equal cost (theory 2)", log10(e2[2] / e2[6]) / log10(4.0), 2.0);
    Series s[] = {{"fourth order", ya, "#e69f00"}, {"leapfrog, same cost", yb, "#0072b2"}};
    plot("order.svg", "Error of the periapsis angle after one orbit", "log10 step [M]", "log10 error [rad]", n, s, 2);
}

/* S29: Newton's limit. */
static void fig_geo_newton(void)
{
    int n = 0;
    double last = 0.0;
    for (double rp = 20.0; rp <= 50000.0 && n < MAXN; rp *= 2.0, n++) {
        Orbit g, w;
        geo_from_apsides(&g, GEO_MASSIVE, 1.0, rp, 3.0 * rp);
        geo_from_apsides(&w, GEO_NEWTON, 1.0, rp, 3.0 * rp);
        double h = 2.0 * 3.14159265358979323846 * pow(rp, 1.5) / 4000.0;
        periapsis(&g, h, 1000000);
        periapsis(&w, h, 1000000);
        xs[n] = (float)(1.0 / rp);
        ya[n] = (float)(g.t / w.t);
        yb[n] = (float)(g.L / w.L);
        yc[n] = (float)((g.phi - 2.0 * 3.14159265358979323846) / (2.0 * 3.14159265358979323846));
        last = g.t / w.t;
        if (rp == 20.0) {
            value("newton-closes", "Newton's orbit from the same apsides: turn after one orbit [rad] (theory 0)", w.phi - 2.0 * 3.14159265358979323846, 0.0);
            value("newton-period", "its period between periapses (theory 2 pi a^1.5 with a = 40)", w.t, 2.0 * 3.14159265358979323846 * pow(40.0, 1.5));
        }
    }
    value("newton-limit", "far-away period of the relativistic orbit over Kepler's, periapsis 40960 M (theory 1 + O(M/r))", last, 1.0);
    Series s[] = {{"period/Kepler", ya, "#e69f00"}, {"L/Newton L", yb, "#0072b2"}, {"turn/2pi", yc, "#d62728"}};
    plot("newton-limit.svg", "Far from the hole the orbit becomes Newton's ellipse (apsides rp, 3 rp)", "M / rp", "ratio", n, s, 3);
}

/* S30: the browser adapter. */
static void val_geo_adapter(void)
{
    so_reset(0, 20.0, 4.2);
    for (int i = 0; i < 100; i++) so_step(SO_CALL_MAX);
    value("adapter-precession", "measured turn per lap after 200000 steps from r0 = 20, L = 4.2 [rad] (theory the elliptic integral)", so_value(SO_PREC), so_value(SO_PREC_EXACT));
    value("adapter-drift", "largest abs(H / H0 - 1) over those steps (theory 0)", so_value(SO_DRIFT), 0.0);
    so_reset(1, 0.0, 6.0);
    for (int i = 0; i < 10 && so_value(SO_STATE) == SO_MOVING; i++) so_step(SO_CALL_MAX);
    value("adapter-deflection", "light b = 6: angle gained inside 30 M over the straight line, integrated [rad] (theory the quadrature)", so_value(SO_DEFLECT), so_value(SO_DEFLECT_INNER));
    so_reset(0, 40.0, 3.2);
    for (int i = 0; i < 100 && so_value(SO_STATE) == SO_MOVING; i++) so_step(SO_CALL_MAX);
    value("adapter-plunge", "state of r0 = 40, L = 3.2 < sqrt 12 (theory 1: captured), while Newton's orbit stays", so_value(SO_STATE) + so_value(SO_NSTATE), 1.0);
}

/* ---------------------------------------------------------------- page 6: the view from the window */
#define PI6 3.14159265358979323846

/* S31: aberration, theta' against theta. */
static void fig_aberration(void)
{
    int n = 0;
    for (int i = 0; i <= 180; i++, n++) {
        double c = cos(i * PI6 / 180.0);
        xs[n] = (float)i;
        ya[n] = (float)i;
        yb[n] = (float)(acos(ab_cos_ship(0.5, c)) * 180.0 / PI6);
        yc[n] = (float)(acos(ab_cos_ship(0.9, c)) * 180.0 / PI6);
        yd[n] = (float)(acos(ab_cos_ship(0.99, c)) * 180.0 / PI6);
    }
    double worst = 0.0, worst0 = 0.0;
    for (int i = 0; i <= 2000; i++) {
        double c = -1.0 + i / 1000.0;
        for (double b = 0.1; b < 0.9951; b += 0.1) worst = fmax(worst, fabs(ab_cos_rest(b, ab_cos_ship(b, c)) - c));
        worst0 = fmax(worst0, fabs(ab_cos_ship(1e-12, c) - c));
    }
    value("ab-90-0.5", "angle on board of a star at 90 deg at rest, beta 0.5 [deg] (theory acos 0.5 = 60)", acos(ab_cos_ship(0.5, 0.0)) * 180.0 / PI6, 60.0);
    value("ab-90-0.9", "the same at beta 0.9 [deg] (theory acos 0.9)", acos(ab_cos_ship(0.9, 0.0)) * 180.0 / PI6, acos(0.9) * 180.0 / PI6);
    value("ab-round-trip", "largest abs(cos theta) error after beta then -beta, beta 0.1 to 0.99, 2001 angles (theory 0)", worst, 0.0);
    value("ab-identity", "largest change of cos theta at beta 1e-12 (theory beta sin^2 theta, at most 1e-12: first order in beta, vanishing with it)", worst0, 1e-12);
    Series s[] = {{"at rest", ya, "#888888"}, {"0.5 c", yb, "#1f77b4"}, {"0.9 c", yc, "#e69f00"}, {"0.99 c", yd, "#d62728"}};
    plot("aberration.svg", "Where a star is seen on board: theta' against theta", "angle at rest theta [deg]", "angle on board theta' [deg]", n, s, 4);
}

/* S32: aberration shifts ln tan(theta/2) by the rapidity, so it composes like velocity addition. */
static void fig_aberration_rapidity(void)
{
    int n = 0;
    for (int i = 2; i <= 178; i += 2, n++) {
        double th = i * PI6 / 180.0, u = log(tan(0.5 * th));
        xs[n] = (float)u;
        ya[n] = (float)u;
        yb[n] = (float)log(tan(0.5 * ab_theta_ship(0.5, th)));
        yc[n] = (float)log(tan(0.5 * ab_theta_ship(0.9, th)));
    }
    double th = PI6 / 3.0, shift = log(tan(0.5 * acos(ab_cos_ship(0.9, cos(th))))) - log(tan(0.5 * th)), worst = 0.0;
    for (double b1 = -0.9; b1 <= 0.91; b1 += 0.3)
        for (double b2 = -0.6; b2 <= 0.91; b2 += 0.5)
            for (int i = 0; i <= 100; i++) {
                double c = -1.0 + i / 50.0;
                worst = fmax(worst, fabs(ab_cos_ship(b2, ab_cos_ship(b1, c)) - ab_cos_ship(add_velocity(b1, b2), c)));
            }
    value("ab-shift", "ln tan(theta'/2) - ln tan(theta/2) at theta = 60 deg, beta 0.9 (theory -atanh 0.9)", shift, -atanh(0.9));
    value("ab-compose", "largest cos theta difference between two aberrations and one at the combined velocity, 42 pairs x 101 angles (theory 0)", worst, 0.0);
    Series s[] = {{"at rest", ya, "#888888"}, {"0.5 c", yb, "#1f77b4"}, {"0.9 c", yc, "#e69f00"}};
    plot("aberration-rapidity.svg", "ln tan(theta'/2) = ln tan(theta/2) - rapidity: aberration is a shift", "ln tan(theta / 2)", "ln tan(theta' / 2)", n, s, 3);
}

/* S33: the Doppler factor against the angle on board. */
static void fig_doppler(void)
{
    int n = 0;
    for (int i = 0; i <= 180; i++, n++) {
        double c = cos(i * PI6 / 180.0);
        xs[n] = (float)i;
        ya[n] = (float)log10(ab_doppler_ship(0.5, c));
        yb[n] = (float)log10(ab_doppler_ship(0.9, c));
        yc[n] = (float)log10(ab_doppler_ship(0.99, c));
    }
    double worst = 0.0;
    for (int i = 0; i <= 180; i++) {
        double th = i * PI6 / 180.0, w2, k2;
        boost(atanh(0.9), 1.0, -cos(th), &w2, &k2);
        worst = fmax(worst, fabs(w2 / ab_doppler_rest(0.9, cos(th)) - 1.0));
    }
    double g = ab_gamma(0.9), cnull = (1.0 - 1.0 / g) / 0.9;
    value("doppler-ahead", "D straight ahead at 0.9 c (theory sqrt((1 + beta) / (1 - beta)) = sqrt 19)", ab_doppler_ship(0.9, 1.0), sqrt(19.0));
    value("doppler-behind", "D straight behind (theory 1 / sqrt 19)", ab_doppler_ship(0.9, -1.0), 1.0 / sqrt(19.0));
    value("doppler-side-rest", "D of a star at 90 deg in the rest frame (theory gamma: transverse blue shift)", ab_doppler_rest(0.9, 0.0), g);
    value("doppler-side-ship", "D of a star seen at 90 deg on board (theory 1 / gamma: transverse red shift)", ab_doppler_ship(0.9, 0.0), 1.0 / g);
    value("doppler-unshifted", "D at the ship angle acos((1 - 1/gamma) / beta) = 64.2 deg, where colours are unchanged (theory 1)", ab_doppler_ship(0.9, cnull), 1.0);
    value("doppler-boost", "largest relative difference between D and omega' / omega from the Lorentz boost of the photon's 4-momentum, 181 angles (theory 0)", worst, 0.0);
    Series s[] = {{"0.5 c", ya, "#1f77b4"}, {"0.9 c", yb, "#e69f00"}, {"0.99 c", yc, "#d62728"}};
    plot("doppler.svg", "Doppler factor against the angle on board (log10 D)", "angle on board theta' [deg]", "log10 D", n, s, 3);
}

/* S34: the solid angle shrinks ahead by 1/D^2; fraction of all stars inside an angle on board. */
static void fig_star_density(void)
{
    int n = 0;
    for (int i = 0; i <= 180; i++, n++) {
        double c = cos(i * PI6 / 180.0);
        xs[n] = (float)i;
        ya[n] = (float)ab_cone_fraction(0.0, c);
        yb[n] = (float)ab_cone_fraction(0.5, c);
        yc[n] = (float)ab_cone_fraction(0.9, c);
        yd[n] = (float)ab_cone_fraction(0.99, c);
    }
    double worst = 0.0;
    for (int i = 1; i < 200; i++) {
        double c = -1.0 + i / 100.0, h = 1e-6;
        worst = fmax(worst, fabs((ab_cos_ship(0.9, c + h) - ab_cos_ship(0.9, c - h)) / (2.0 * h) / ab_solid_angle(0.9, c) - 1.0));
    }
    enum { N = 20000 };
    double sum = ab_solid_angle(0.99, -1.0) + ab_solid_angle(0.99, 1.0), h = 2.0 / N;
    for (int i = 1; i < N; i++) sum += (i % 2 ? 4.0 : 2.0) * ab_solid_angle(0.99, -1.0 + i * h);
    sv_reset(0.9);
    int inside = 0;
    for (int i = 0; i < SV_STARS_MAX; i++) inside += sv_star(i, SV_COS_SHIP) >= 0.9 - 1e-12;
    value("solid-jacobian", "largest relative difference between d cos theta' / d cos theta (finite differences) and 1 / D^2 at 0.9 c (theory 0)", worst, 0.0);
    value("solid-total", "solid angle of the whole sky on board at 0.99 c, integral of dOmega / D^2 over 4 pi [sr] (theory 4 pi)", 2.0 * PI6 * sum * h / 3.0, 4.0 * PI6);
    value("density-ahead", "stars per solid angle straight ahead at 0.9 c over at rest (theory D^2 = 19)", 1.0 / ab_solid_angle(0.9, 1.0), 19.0);
    value("cone-half", "fraction of all stars seen inside acos(beta) at 0.9 c (theory 1/2)", ab_cone_fraction(0.9, 0.9), 0.5);
    value("catalogue-ahead", "catalogue stars seen in the front hemisphere at 0.9 c, out of 600 (theory 600 (1 + beta) / 2 = 570, one random sky)", sv_value(SV_FORWARD), 570.0);
    value("catalogue-cone", "catalogue stars inside acos 0.9 on board against those in the front half at rest (theory equal, star by star)", inside, sv_value(SV_REST_FORWARD));
    Series s[] = {{"at rest", ya, "#888888"}, {"0.5 c", yb, "#1f77b4"}, {"0.9 c", yc, "#e69f00"}, {"0.99 c", yd, "#d62728"}};
    plot("star-density.svg", "Fraction of all stars seen within an angle of the direction of motion", "angle on board theta' [deg]", "fraction of stars", n, s, 4);
}

static double fig_seen(double nu, double D, double T) { return D * D * D * ab_planck(nu / D, T); }
static double fig_integral(double D, double T)
{
    enum { N = 200000 };
    double top = 40.0 * D * T / AB_C2, h = top / N, sum = fig_seen(top, D, T);
    for (int i = 1; i < N; i++) sum += (i % 2 ? 4.0 : 2.0) * fig_seen(i * h, D, T);
    return sum * h / 3.0;
}

/* S35: the powers of D: specific intensity D^3, bolometric intensity D^4, solid angle D^-2, point-star flux D^2. */
static void fig_brightness(void)
{
    int n = 0;
    for (int i = 0; i <= 180; i++, n++) {
        double D = ab_doppler_ship(0.9, cos(i * PI6 / 180.0));
        xs[n] = (float)i;
        ya[n] = (float)(2.0 * log10(D));
        yb[n] = (float)(3.0 * log10(D));
        yc[n] = (float)(4.0 * log10(D));
        yd[n] = (float)log10(ab_band_ratio(D, SV_SUN));
    }
    double worst = 0.0;
    for (int i = 1; i <= 100; i++) {
        double D = ab_doppler_rest(0.9, -1.0 + i / 50.0), nu = i * 4e-5;
        worst = fmax(worst, fabs(fig_seen(nu, D, 5800.0) / ab_planck(nu, D * 5800.0) - 1.0));
    }
    double D = sqrt(19.0), rest = fig_integral(1.0, 5800.0), moving = fig_integral(D, 5800.0), t2, x2;
    boost(atanh(0.9), -1.0, 1.0, &t2, &x2);
    value("planck-invariance", "largest relative difference between D^3 B(nu / D, T) and B(nu, D T), 100 D x 100 nu (theory 0: I_nu / nu^3 is invariant)", worst, 0.0);
    value("stefan-d4", "bolometric intensity of a 5800 K black body seen with D = sqrt 19 over at rest, by quadrature (theory D^4 = 361)", moving / rest, pow(D, 4.0));
    value("flux-d2", "bolometric flux of a point star: intensity D^4 times solid angle D^-2 (theory D^2 = 19)", moving / rest * ab_solid_angle(0.9, 1.0), 19.0);
    value("distance-d", "distance on board to the emission event of light that left a star straight ahead 1 yr ago (theory D = sqrt 19: so D^2 / r^2 = D^4 / r'^2)", -t2, D);
    Series s[] = {{"D^2 point star", ya, "#cc79a7"}, {"D^3", yb, "#009e73"}, {"D^4 intensity", yc, "#e69f00"}, {"visible, 5800 K", yd, "#56b4e9"}};
    plot("brightness.svg", "How much brighter at 0.9 c: powers of D and the visible band (log10)", "angle on board theta' [deg]", "log10 ratio to at rest", n, s, 4);
}

/* S36: the colour temperature D T and the visible-band brightness against D. */
static void fig_colour(void)
{
    int n = 0;
    for (double lg = -0.7; lg <= 2.0001 && n < MAXN; lg += 0.02, n++) {
        double D = pow(10.0, lg);
        xs[n] = (float)lg;
        ya[n] = (float)log10(ab_band_ratio(D, SV_SUN));
        yb[n] = (float)(2.0 * lg);
        yc[n] = (float)log10(D * SV_SUN / 1e4);
    }
    double best = 0.0, bd = 1.0, back = 0.0;
    for (double D = 1.0; D < 200.0; D *= 1.0005) {
        double r = ab_band_ratio(D, SV_SUN);
        if (r > best) { best = r; bd = D; }
        if (D > bd && r < 1.0 && back == 0.0) back = D;
    }
    double a = 50.0, b = 3000.0, g = 0.5 * (sqrt(5.0) - 1.0), T = 5800.0;
    for (int i = 0; i < 200; i++) {
        double x1 = b - g * (b - a), x2 = a + g * (b - a);
        double f1 = 1.0 / (pow(x1, 5.0) * expm1(AB_C2 / (x1 * T))), f2 = 1.0 / (pow(x2, 5.0) * expm1(AB_C2 / (x2 * T)));
        if (f1 > f2) b = x2; else a = x1;
    }
    double rgb[3];
    ab_colour(6500.0, rgb);
    value("wien", "peak of B_lambda at 5800 K by golden-section search, times T [nm K] (theory Wien's b = 2.8978e6)", 0.5 * (a + b) * T, AB_WIEN);
    value("colour-white", "smallest channel of the display colour at 6500 K (theory 1: white by construction)", fmin(rgb[0], fmin(rgb[1], rgb[2])), 1.0);
    value("band-peak", "largest visible-band brightening of a 5800 K star over D (no closed form: the theory column repeats the measurement)", best, best);
    value("band-peak-d", "the D of that peak (no closed form; stars seen with this D are brightest to the eye)", bd, bd);
    value("band-back", "the D beyond which the visible band is fainter than at rest again (no closed form; the light has moved to the ultraviolet)", back, back);
    value("band-0.99", "visible-band brightening straight ahead at 0.99 c, D = sqrt 199 (no closed form)", ab_band_ratio(sqrt(199.0), SV_SUN), ab_band_ratio(sqrt(199.0), SV_SUN));
    Series s[] = {{"visible band, 5800 K", ya, "#56b4e9"}, {"bolometric D^2", yb, "#cc79a7"}, {"log10(D T / 10000 K)", yc, "#e69f00"}};
    plot("colour.svg", "A 5800 K star seen with Doppler factor D", "log10 D", "log10", n, s, 3);
}

/* S37: the browser adapter. */
static void val_sky_adapter(void)
{
    sv_reset(0.9);
    value("adapter-sky-d", "SV_D_AHEAD at 0.9 c (theory sqrt 19)", sv_value(SV_D_AHEAD), sqrt(19.0));
    value("adapter-sky-cone", "SV_HALF_CONE at 0.9 c [rad] (theory acos 0.9)", sv_value(SV_HALF_CONE), acos(0.9));
    value("adapter-sky-curve", "sv_curve(90, SV_C_THETA_SHIP) at 0.9 c [deg] (theory acos 0.9 in degrees)", sv_curve(90, SV_C_THETA_SHIP), acos(0.9) * 180.0 / PI6);
    sv_reset(0.0);
    value("adapter-sky-rest", "stars ahead at rest out of 600 (theory 300, one random sky)", sv_value(SV_FORWARD), 300.0);
}

int main(int argc, char **argv)
{
    snprintf(dir, sizeof dir, "%s", argc > 1 ? argv[1] : "docs/theory/figures");
    values = fopen(path("values.md"), "w");
    if (!values) { fprintf(stderr, "cannot write %s/values.md\n", dir); return 1; }
    fprintf(values, "# Measured values\n\nWritten by `make theory-figs` (tests/theory_figs.c) from the program's own functions. Do not edit by hand.\n"
                    "Units: c = 1, times in years, lengths in light-years; on page 5 G = c = M = 1, lengths and times in M; on page 6 angles in degrees, temperatures in K and brightness as a ratio to the star at rest. The last column is the relative error; where the theory column is 0 it is the absolute value.\n\n"
                    "| id | what is measured | measured | theory | error |\n|---|---|---|---|---|\n");
    fig_boost();
    fig_rapidity();
    fig_dilation();
    fig_simultaneity();
    fig_hyperbola();
    val_step();
    fig_trip();
    fig_demo();
    fig_geo_conserved();
    fig_geo_potential();
    fig_geo_circular();
    fig_geo_isco();
    fig_geo_light();
    fig_geo_deflection();
    fig_geo_precession();
    fig_geo_clocks();
    fig_geo_fall();
    fig_geo_order();
    fig_geo_newton();
    val_geo_adapter();
    fig_aberration();
    fig_aberration_rapidity();
    fig_doppler();
    fig_star_density();
    fig_brightness();
    fig_colour();
    val_sky_adapter();
    fclose(values);
    return 0;
}
