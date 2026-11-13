/* Checks for the Schwarzschild geodesics (src/schwarzschild.c) and their browser adapter (src/orbit.c):
 * closed forms, conserved quantities, the order of the integrator and the Newtonian limit.
 * With --trace, prints the native samples that scripts/smoke.cjs compares with the WASM build. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/orbit.h"
#include "../src/relativity.h"
#include "../src/schwarzschild.h"

#define PI 3.14159265358979323846

static int failures, checks;

#define CHECK(cond, ...)                                      \
    do {                                                      \
        checks++;                                             \
        if (!(cond)) {                                        \
            failures++;                                       \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);       \
            printf(__VA_ARGS__);                              \
            printf("\n");                                     \
        }                                                     \
    } while (0)

/* Steps until the next periapsis and returns the angle there (refined to pr = 0); -1 if none within max steps. */
static double next_periapsis(Orbit *o, double h, int max)
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

/* Steps until r grows back through r_mark and returns the angle there (secant on the fraction of the last step). */
static double angle_back_at(Orbit *o, double h, double r_mark, int max)
{
    for (int k = 0; k < max && o->state == GEO_MOVING; k++) {
        Orbit before = *o;
        geo_step(o, h);
        if (before.r < r_mark && o->r >= r_mark && o->pr > 0.0) {
            double a = 0.0, b = h, fa = before.r - r_mark, fb = o->r - r_mark, x = 0.0;
            for (int i = 0; i < 60; i++) {
                x = a - fa * (b - a) / (fb - fa);
                Orbit c = before;
                geo_step(&c, x);
                double fx = c.r - r_mark;
                if (fabs(fx) < 1e-14) break;
                if (fx < 0.0) { a = x; fa = fx; } else { b = x; fb = fx; }
            }
            Orbit c = before;
            geo_step(&c, x);
            return c.phi;
        }
    }
    return NAN;
}

static void test_potential_and_force(void)
{
    double worst = 0.0;
    for (int kind = 0; kind < 3; kind++)
        for (double r = 2.5; r < 60.0; r += 0.37) {
            double e = 1e-5 * r, num = -(geo_potential(kind, 1.0, 4.0, r + e) - geo_potential(kind, 1.0, 4.0, r - e)) / (2.0 * e);
            worst = fmax(worst, fabs(num - geo_force(kind, 1.0, 4.0, r)) / fmax(1e-3, fabs(geo_force(kind, 1.0, 4.0, r))));
        }
    CHECK(worst < 1e-7, "force is -dV/dr (worst relative %.2e)", worst);
    /* The relativistic potential is Newton's plus -M L^2 / r^3 (written as 1 + 2V). */
    double r = 9.0, L = 4.0;
    CHECK(fabs(2.0 * geo_potential(GEO_MASSIVE, 1.0, L, r) - (1.0 + 2.0 * geo_potential(GEO_NEWTON, 1.0, L, r) - 2.0 * L * L / (r * r * r))) < 1e-15,
          "GR potential = Newton's + the L^2/r^3 term");
}

static void test_conserved_quantities(void)
{
    Orbit o;
    CHECK(geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0) == 0, "orbit 10..30");
    double H0 = geo_energy(&o), worst = 0.0, eworst = 0.0, lworst = 0.0;
    for (int k = 0; k < 160000; k++) { /* about 100 orbits */
        Orbit before = o;
        geo_step(&o, 0.4);
        worst = fmax(worst, fabs(geo_energy(&o) / H0 - 1.0));
        if (k % 997 == 0) {
            /* E and L read back from the finite differences of t and phi over the step. */
            double rm = 0.5 * (before.r + o.r);
            double e = (1.0 - 2.0 / rm) * (o.t - before.t) / 0.4, l = rm * rm * (o.phi - before.phi) / 0.4;
            eworst = fmax(eworst, fabs(e / o.E - 1.0));
            lworst = fmax(lworst, fabs(l / o.L - 1.0));
        }
    }
    CHECK(worst < 1e-10, "energy error stays bounded over 100 orbits: %.2e", worst);
    CHECK(eworst < 2e-3 && lworst < 2e-3, "E and L from finite differences (%.2e, %.2e)", eworst, lworst);
    CHECK(o.state == GEO_MOVING, "still in orbit");
}

static void test_circular_orbits(void)
{
    for (double r = 7.0; r < 40.0; r += 11.0) {
        double L = geo_circular_L(1.0, r);
        CHECK(fabs(geo_circular_radius(1.0, L) - r) < 1e-10 * r, "circular radius of L(r) at r = %g", r);
        Orbit o;
        geo_init(&o, GEO_MASSIVE, 1.0, L, r, 0.0);
        double h = 2.0 * PI * pow(r, 1.5) / 800.0, dev = 0.0;
        for (int k = 0; k < 8000; k++) { geo_step(&o, h); dev = fmax(dev, fabs(o.r - r)); }
        CHECK(dev < 1e-8 * r, "stays on the circle at r = %g (%.2e)", r, dev);
        double omega = o.phi / o.t, kepler = sqrt(1.0 / (r * r * r));
        CHECK(fabs(omega / kepler - 1.0) < 1e-10, "Kepler's third law in far-away time at r = %g: %.3e", r, omega / kepler - 1.0);
        /* The orbiting clock: the static clock's rate times 1 / gamma of the local speed (special relativity). */
        double v = geo_local_speed(1.0, r);
        Rocket local = {0.0, 0.0, 0.0, atanh(v)};
        double predicted = geo_static_rate(1.0, r) / lorentz_gamma(&local);
        CHECK(fabs(o.s / o.t - predicted) < 1e-10 && fabs(predicted - geo_circular_rate(1.0, r)) < 1e-14,
              "orbiting clock rate at r = %g: measured %.12f, sqrt(1 - 3M/r) %.12f", r, o.s / o.t, geo_circular_rate(1.0, r));
        /* E of a circular orbit: (1 - 2M/r) / sqrt(1 - 3M/r). */
        CHECK(fabs(o.E - (1.0 - 2.0 / r) / sqrt(1.0 - 3.0 / r)) < 1e-12, "E of the circular orbit at r = %g", r);
    }
}

static void test_isco(void)
{
    double L = sqrt(12.0), r = geo_isco(1.0), e = 1e-3;
    CHECK(r == 6.0 && fabs(geo_circular_radius(1.0, L) - 6.0) < 1e-6 && fabs(geo_unstable_radius(1.0, L) - 6.0) < 1e-6, "at L^2 = 12 the two circular radii meet at 6M");
    CHECK(geo_circular_radius(1.0, 3.4) == 0.0, "no circular orbit for L^2 < 12");
    double v1 = geo_force(GEO_MASSIVE, 1.0, L, r), v2 = -(geo_force(GEO_MASSIVE, 1.0, L, r + e) - geo_force(GEO_MASSIVE, 1.0, L, r - e)) / (2.0 * e);
    CHECK(fabs(v1) < 1e-15 && fabs(v2) < 1e-8, "V' = V'' = 0 at 6M (%.2e, %.2e)", v1, v2);
    double rmin = 50.0;
    for (double x = 3.01; x < 50.0; x += 0.001)
        if (geo_circular_L(1.0, x) < geo_circular_L(1.0, rmin)) rmin = x;
    CHECK(fabs(rmin - 6.0) < 2e-3, "L of circular orbits is least at 6M (scan %.4f)", rmin);
    /* Nudge circular orbits outward by 1%: outside 6M they wobble, inside they leave. */
    Orbit o;
    geo_init(&o, GEO_MASSIVE, 1.0, geo_circular_L(1.0, 7.0), 7.0, 0.0);
    o.pr = 0.01;
    double lo = 1e9, hi = 0.0;
    for (int k = 0; k < 200000 && o.state == GEO_MOVING; k++) { geo_step(&o, 0.05); lo = fmin(lo, o.r); hi = fmax(hi, o.r); }
    CHECK(o.state == GEO_MOVING && lo > 6.6 && hi < 7.4, "stable at 7M: r in [%.3f, %.3f]", lo, hi);
    geo_init(&o, GEO_MASSIVE, 1.0, geo_circular_L(1.0, 5.0), 5.0, 0.0);
    o.pr = -0.001;
    for (int k = 0; k < 200000 && o.state == GEO_MOVING; k++) geo_step(&o, 0.05);
    CHECK(o.state == GEO_CAPTURED, "unstable at 5M: a small inward nudge falls in");
}

static void test_light(void)
{
    double bc = geo_critical_b(1.0);
    /* The photon sphere: the light potential peaks at 3M, where 2V / L^2 = 1 / 27 = 1 / bc^2. */
    double best = 0.0, at = 0.0;
    for (double r = 2.1; r < 10.0; r += 1e-4) {
        double v = geo_potential(GEO_LIGHT, 1.0, 1.0, r);
        if (v > best) { best = v; at = r; }
    }
    CHECK(fabs(at - geo_photon_sphere(1.0)) < 2e-4 && fabs(2.0 * best - 1.0 / (bc * bc)) < 1e-12, "peak at %.4f, 2V = 1/b_c^2", at);
    for (int side = -1; side <= 1; side += 2) {
        double b = bc * (1.0 + side * 1e-3);
        Orbit o;
        geo_init(&o, GEO_LIGHT, 1.0, b, 40.0, 0.0);
        o.E = 1.0;
        o.pr = -sqrt(1.0 - 2.0 * geo_potential(GEO_LIGHT, 1.0, b, 40.0));
        for (int k = 0; k < 200000 && o.state == GEO_MOVING && !(o.pr > 0.0 && o.r > 40.0); k++) geo_step(&o, 0.01);
        CHECK(side < 0 ? o.state == GEO_CAPTURED : o.state == GEO_MOVING, "b = b_c (1 %+g): %s", side * 1e-3, side < 0 ? "falls in" : "escapes");
        if (side > 0) CHECK(o.phi > 2.0 * PI, "just above b_c the light winds round the hole (%.2f rad)", o.phi);
    }
    /* Deflection: the integrated ray against the quadrature with the same limits, and the far-field formula. */
    for (double b = 6.0; b < 1500.0; b *= 5.0) {
        double r0 = 30.0 * b;
        Orbit o;
        geo_init(&o, GEO_LIGHT, 1.0, b, r0, 0.0);
        o.E = 1.0;
        o.pr = -sqrt(1.0 - 2.0 * geo_potential(GEO_LIGHT, 1.0, b, r0));
        double exact = geo_swept_angle(&o, r0), total = geo_swept_angle(&o, INFINITY) - PI;
        double swept = angle_back_at(&o, 0.002 * b, r0, 10000000);
        CHECK(fabs(swept - exact) < 1e-8, "b = %g: integrated sweep %.12f, quadrature %.12f", b, swept, exact);
        double two = 4.0 / b + 15.0 * PI / (4.0 * b * b);
        if (b > 10.0) CHECK(fabs(total / geo_deflection_weak(1.0, b) - 1.0) < 1.5 * 15.0 * PI / 16.0 / b && fabs(total / two - 1.0) < 30.0 / (b * b),
              "b = %g: deflection %.10f, 4M/b %.10f, with the (M/b)^2 term %.10f", b, total, 4.0 / b, two);
    }
}

static void test_precession(void)
{
    double prev = 1e9;
    for (double rp = 10.0; rp < 3000.0; rp *= 3.0) {
        double ra = 2.0 * rp, a = 1.5 * rp, e = 1.0 / 3.0;
        Orbit o;
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, rp, ra);
        double exact = geo_precession_exact(1.0, rp, ra), quad = geo_swept_angle(&o, 0.0) - 2.0 * PI;
        double h = 2.0 * PI * pow(rp, 1.5) / 2000.0, measured = next_periapsis(&o, h, 100000) - 2.0 * PI;
        CHECK(fabs(quad - exact) < 1e-9 * fmax(1.0, exact) && fabs(measured - exact) < 1e-9, "rp = %g: quadrature %.3e, integrated %.3e off the elliptic integral",
              rp, quad - exact, measured - exact);
        double rel = fabs(exact / geo_precession_weak(1.0, a, e) - 1.0), p = a * (1.0 - e * e);
        double two = geo_precession_weak(1.0, a, e) * (1.0 + (18.0 + e * e) / 4.0 / p), rel2 = fabs(exact / two - 1.0);
        CHECK(rel < 8.0 / p && rel < prev, "rp = %g: weak-field formula off by %.3e (p = %g)", rp, rel, p);
        CHECK(rel2 < 30.0 / (p * p), "rp = %g: with the (M/p)^2 term (18 + e^2)/4 the error falls to %.3e", rp, rel2);
        prev = rel;
    }
    /* Newton's ellipse does not turn. */
    Orbit n;
    geo_from_apsides(&n, GEO_NEWTON, 1.0, 10.0, 30.0);
    double turn = next_periapsis(&n, 0.2, 100000) - 2.0 * PI;
    CHECK(fabs(turn) < 1e-9, "Newton's orbit closes (%.2e rad)", turn);
    CHECK(fabs(n.t - 2.0 * PI * pow(20.0, 1.5)) < 1e-7, "Newton's period 2 pi a^1.5 (%.9f)", n.t);
    geo_from_apsides(&n, GEO_NEWTON, 1.0, 10.0, 30.0);
    CHECK(fabs(geo_swept_angle(&n, 0.0) - 2.0 * PI) < 1e-10, "Newton's quadrature gives 2 pi");
}

static void test_order(void)
{
    double err4[3], err2[3], exact = geo_precession_exact(1.0, 10.0, 30.0) + 2.0 * PI;
    for (int i = 0; i < 3; i++) {
        double h = 0.8 / (1 << i);
        Orbit o;
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
        err4[i] = fabs(next_periapsis(&o, h, 1000000) - exact);
        geo_from_apsides(&o, GEO_MASSIVE, 1.0, 10.0, 30.0);
        double phi = -1.0;
        for (int k = 0; k < 1000000; k++) {
            Orbit before = o;
            geo_leapfrog(&o, h / 4.0);
            if (before.pr < 0.0 && o.pr >= 0.0) { phi = before.phi + (o.phi - before.phi) * (-before.pr) / (o.pr - before.pr); break; }
        }
        err2[i] = fabs(phi - exact);
    }
    double p4 = log2(err4[1] / err4[2]), p2 = log2(err2[1] / err2[2]);
    CHECK(p4 > 3.7 && p4 < 4.3, "fourth order: halving the step divides the error by 2^%.2f", p4);
    CHECK(p2 > 1.7 && p2 < 2.3, "the plain leapfrog is second order: 2^%.2f", p2);
}

static void test_newton_limit(void)
{
    double prev = 1e9;
    for (double rp = 20.0; rp < 20000.0; rp *= 10.0) {
        Orbit g, n;
        geo_from_apsides(&g, GEO_MASSIVE, 1.0, rp, 3.0 * rp);
        geo_from_apsides(&n, GEO_NEWTON, 1.0, rp, 3.0 * rp);
        double h = 2.0 * PI * pow(rp, 1.5) / 4000.0;
        next_periapsis(&g, h, 1000000);
        next_periapsis(&n, h, 1000000);
        double rel = fabs(g.t / n.t - 1.0);
        CHECK(rel < prev && rel < 10.0 / rp, "rp = %g: radial period in far-away time over Kepler's: off by %.3e", rp, rel);
        CHECK(fabs(g.L / n.L - 1.0) < 3.0 / rp, "rp = %g: same orbit, L ratio %.6f", rp, g.L / n.L);
        prev = rel;
    }
}

static void test_fall(void)
{
    Orbit o;
    geo_init(&o, GEO_MASSIVE, 1.0, 0.0, 10.0, 0.0);
    double h = 1e-3, rmark = 3.0, tau = NAN;
    for (int k = 0; k < 1000000 && o.state == GEO_MOVING; k++) {
        Orbit before = o;
        geo_step(&o, h);
        if (before.r > rmark && o.r <= rmark) tau = before.s + h * (before.r - rmark) / (before.r - o.r);
    }
    CHECK(fabs(tau - geo_fall_time(1.0, 10.0, 3.0)) < 1e-6, "radial fall 10M -> 3M takes %.9f (closed form %.9f)", tau, geo_fall_time(1.0, 10.0, 3.0));
    CHECK(o.state == GEO_CAPTURED && o.r < 2.002 && o.s < geo_fall_time(1.0, 10.0, 2.0) + h, "captured at the horizon in finite proper time");
    CHECK(geo_rate(&o) > 500.0, "the far-away clock runs away near the horizon: dt/dtau = %.1f at r = %.4f", geo_rate(&o), o.r);
}

static void test_adapter(void)
{
    CHECK(so_reset(0, 1.0, 4.0) == -1 && so_reset(0, 20.0, -1.0) == -1, "bad inputs refused");
    so_reset(0, 20.0, 4.2);
    CHECK(so_value(SO_BOUND) == GEO_BOUND && fabs(so_value(SO_RA) - 20.0) < 1e-9, "r0 = 20 is the apoapsis of a bound orbit");
    for (int i = 0; i < 100; i++) so_step(2000);
    CHECK(so_value(SO_STEPS) == 200000 && so_value(SO_APSES) > 100, "200000 steps, %g periapses", so_value(SO_APSES));
    CHECK(fabs(so_value(SO_PREC) - so_value(SO_PREC_EXACT)) < 1e-6, "measured precession %.9f vs exact %.9f", so_value(SO_PREC), so_value(SO_PREC_EXACT));
    CHECK(so_value(SO_DRIFT) < 1e-10, "energy drift %.2e", so_value(SO_DRIFT));
    CHECK(so_value(SO_NBOUND) == GEO_BOUND && fabs(so_value(SO_NRA) - 20.0) < 1e-9, "Newton's orbit from the same start");
    CHECK(so_value(SO_TRAIL) == SO_TRAIL_MAX, "trail ring full");
    double x = so_trail_x(SO_TRAIL_MAX - 1), y = so_trail_y(SO_TRAIL_MAX - 1);
    CHECK(fabs(x - so_value(SO_X)) < 1e-4 && fabs(y - so_value(SO_Y)) < 1e-4, "newest trail point is the current position");
    CHECK(so_value(SO_CLOCK_RATE) < so_value(SO_STATIC_RATE) && so_value(SO_STATIC_RATE) < 1.0, "orbiting clock slower than a static one");
    so_step(10);
    CHECK(so_value(SO_STATE) == SO_LIMIT || so_value(SO_STEPS) <= SO_MAX_STEPS, "step limit");

    so_reset(0, 40.0, 3.2);
    for (int i = 0; i < 100 && so_value(SO_STATE) == SO_MOVING; i++) so_step(2000);
    CHECK(so_value(SO_BOUND) == GEO_PLUNGES && so_value(SO_STATE) == SO_CAPTURED, "L = 3.2 from 40M plunges");
    CHECK(so_value(SO_NSTATE) == SO_MOVING, "Newton's orbit with the same L does not");

    so_reset(1, 0.0, 6.0);
    for (int i = 0; i < 10 && so_value(SO_STATE) == SO_MOVING; i++) so_step(2000);
    CHECK(so_value(SO_STATE) == SO_ESCAPED && fabs(so_value(SO_DEFLECT) - so_value(SO_DEFLECT_INNER)) < 1e-3,
          "light b = 6 escapes, deflection %.6f vs %.6f", so_value(SO_DEFLECT), so_value(SO_DEFLECT_INNER));
    so_reset(1, 0.0, 5.0);
    for (int i = 0; i < 10 && so_value(SO_STATE) == SO_MOVING; i++) so_step(2000);
    CHECK(so_value(SO_STATE) == SO_CAPTURED && so_value(SO_NSTATE) == SO_ESCAPED, "light b = 5 < 3 sqrt 3 is captured; the straight line passes");
    CHECK(so_value(SO_CURVE_N) == SO_CURVE_POINTS && so_curve(-1) == 0.0 && so_curve(SO_CURVE_POINTS) == 0.0, "curve bounds");
}

/* The scripted runs used by the trace and the WASM comparison. */
static const double RUNS[][3] = {{0, 20.0, 4.2}, {0, 7.0, 3.5}, {0, 40.0, 3.2}, {1, 0.0, 6.0}, {1, 0.0, 5.2}};
enum { TRACE_FIELDS = SO_FIELDS };

static void trace(void)
{
    printf("run,step");
    for (int f = 0; f < TRACE_FIELDS; f++) printf(",f%d", f);
    printf(",trail_x,trail_y,ntrail_x,ntrail_y,curve,ncurve,apsis\n");
    for (int run = 0; run < 5; run++) {
        so_reset((int)RUNS[run][0], RUNS[run][1], RUNS[run][2]);
        for (int step = 0; step <= 3000; step += 300) {
            int n = (int)so_value(SO_TRAIL), a = (int)so_value(SO_APSES);
            printf("%d,%d", run, step);
            for (int f = 0; f < TRACE_FIELDS; f++) printf(",%.17g", so_value(f));
            printf(",%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n", so_trail_x(n - 1), so_trail_y(n - 1), so_ntrail_x(n - 1), so_ntrail_y(n - 1),
                   so_curve(step % SO_CURVE_POINTS), so_ncurve(step % SO_CURVE_POINTS), so_apsis(a > 0 ? a - 1 : 0));
            so_step(300);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--trace") == 0) {
        trace();
        return 0;
    }
    test_potential_and_force();
    test_conserved_quantities();
    test_circular_orbits();
    test_isco();
    test_light();
    test_precession();
    test_order();
    test_newton_limit();
    test_fall();
    test_adapter();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
