/* Regenerates every plotted figure of docs/theory and the table of measured values
 * (docs/theory/figures/values.md). Run with `make theory-figs`. Each figure is produced by the
 * same model (src/relativity.c) and browser adapter (src/rider.c) the program uses; nothing here
 * changes them. */
#include "../src/relativity.h"
#include "../src/rider.h"
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

int main(int argc, char **argv)
{
    snprintf(dir, sizeof dir, "%s", argc > 1 ? argv[1] : "docs/theory/figures");
    values = fopen(path("values.md"), "w");
    if (!values) { fprintf(stderr, "cannot write %s/values.md\n", dir); return 1; }
    fprintf(values, "# Measured values\n\nWritten by `make theory-figs` (tests/theory_figs.c) from the program's own functions. Do not edit by hand.\n"
                    "Units: c = 1, times in years, lengths in light-years. The last column is the relative error; where the theory column is 0 it is the absolute value.\n\n"
                    "| id | what is measured | measured | theory | error |\n|---|---|---|---|---|\n");
    fig_boost();
    fig_rapidity();
    fig_dilation();
    fig_simultaneity();
    fig_hyperbola();
    val_step();
    fig_trip();
    fig_demo();
    fclose(values);
    return 0;
}
