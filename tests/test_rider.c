/* Checks for the browser adapter (src/rider.c) against the closed form and the Rocket API.
 * With --trace, prints the native samples that scripts/smoke.cjs compares with the WASM build. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/relativity.h"
#include "../src/rider.h"

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

/* The scripted manual run used by the trace and the WASM comparison. */
static int manual_thrust(int step)
{
    if (step < 500) return 1;
    if (step < 700) return 0;
    if (step < 1200) return -1;
    return 1;
}

static void test_demo_matches_closed_form(void)
{
    rr_reset(1);
    double a = ONE_G, h = rr_value(RR_HALF_TAU);
    CHECK(fabs(h - acosh(1.0 + a * 4.2 / 2.0) / a) < 1e-15, "half proper time %.12f", h);
    rr_step(RR_DEMO_HALF);
    /* Turnaround: from rest at 1 g for half the proper time, exact hyperbolic motion. */
    CHECK(fabs(rr_value(RR_T) - sinh(a * h) / a) < 1e-9, "t at flip %.9f", rr_value(RR_T));
    CHECK(fabs(rr_value(RR_X) - 2.1) < 1e-9, "flip at half way: x %.9f", rr_value(RR_X));
    CHECK(fabs(rr_value(RR_V) - tanh(a * h)) < 1e-12, "v at flip");
    for (int i = 0; i < 10; i++) rr_step(RR_DEMO_HALF / 10);
    CHECK(rr_value(RR_DONE) == 1 && rr_value(RR_ARRIVED) == 1, "demo arrives");
    CHECK(fabs(rr_value(RR_X) - 4.2) < 1e-9, "arrives on the star: x %.12f", rr_value(RR_X));
    CHECK(fabs(rr_value(RR_V)) < 1e-9, "arrives at rest: v %.2e", rr_value(RR_V));
    CHECK(fabs(rr_value(RR_TAU) - 2.0 * h) < 1e-12, "proper time 2h");
    CHECK(fabs(rr_value(RR_T) - 2.0 * sinh(a * h) / a) < 1e-9, "Earth time 2 sinh(ah)/a");
    CHECK(fabs(rr_value(RR_TAU) - 3.53) < 0.005 && fabs(rr_value(RR_T) - 5.82) < 0.005,
          "README numbers 3.53 / 5.82 yr: %.4f / %.4f", rr_value(RR_TAU), rr_value(RR_T));
    CHECK(fabs(rr_value(RR_GAP) - (rr_value(RR_T) - rr_value(RR_TAU))) < 1e-15, "gap = t - tau");
    double tau = rr_value(RR_TAU);
    rr_step(50);
    CHECK(rr_value(RR_TAU) == tau, "finished demo does not move");
}

static void test_adapter_is_identical_to_rocket_api(void)
{
    rr_reset(0);
    Rocket r = {0, 0, 0, 0};
    double dtau = rr_value(RR_DTAU);
    int same = 1;
    for (int k = 0; k < 1800; k++) {
        rr_thrust(manual_thrust(k));
        rr_step(1);
        rocket_step(&r, manual_thrust(k) * ONE_G, dtau);
        same &= rr_value(RR_T) == r.t && rr_value(RR_X) == r.x && rr_value(RR_TAU) == r.tau && rr_value(RR_PHI) == r.phi;
    }
    CHECK(same, "adapter state bit-identical to rocket_step over 1,800 steps");
}

static void test_interval_and_trail(void)
{
    rr_reset(1);
    rr_step(RR_DEMO_HALF);
    rr_step(RR_DEMO_HALF);
    int n = (int)rr_value(RR_TRAIL);
    CHECK(n == 2 * RR_DEMO_HALF / RR_TRAIL_EVERY + 1, "trail length %d", n);
    int timelike = 1, ordered = 1;
    for (int i = 1; i < n; i++) {
        double dt = rr_trail_t(i) - rr_trail_t(i - 1), dx = rr_trail_x(i) - rr_trail_x(i - 1);
        timelike &= interval2(dt, dx) > 0.0;
        ordered &= dt > 0.0;
    }
    CHECK(timelike, "every worldline segment is timelike (inside the light cone)");
    CHECK(ordered, "trail moves forward in coordinate time");
    CHECK(rr_value(RR_MAX_V) < 1.0 && rr_value(RR_MAX_V) > 0.9, "top speed below c: %.6f", rr_value(RR_MAX_V));
    CHECK(rr_trail_t(-1) == 0.0 && rr_trail_x(n) == 0.0, "out-of-range trail reads are zero");
}

static void test_manual_controls(void)
{
    rr_reset(0);
    CHECK(rr_value(RR_ARRIVED) == 0 && rr_value(RR_DONE) == 0, "manual starts at Earth, not arrived");
    rr_thrust(5);
    CHECK(rr_value(RR_THRUST) == 1, "thrust clamps to +1");
    rr_thrust(-9);
    CHECK(rr_value(RR_THRUST) == -1, "thrust clamps to -1");
    rr_thrust(0);
    rr_step(100);
    CHECK(rr_value(RR_X) == 0.0 && rr_value(RR_T) == rr_value(RR_TAU), "coasting at rest: Earth and ship clocks agree");
    /* Flying the demo schedule by hand also stops on the star. */
    rr_reset(0);
    rr_thrust(1);
    rr_step(RR_DEMO_HALF);
    rr_thrust(-1);
    rr_step(RR_DEMO_HALF);
    CHECK(rr_value(RR_ARRIVED) == 1, "hand-flown flip-and-burn arrives (x %.4f v %.4f)", rr_value(RR_X), rr_value(RR_V));
    CHECK(rr_value(RR_STEPS) < 2 * RR_DEMO_HALF, "manual arrival stops the clock once inside the window");
    /* The demo ignores manual thrust. */
    rr_reset(1);
    rr_thrust(-1);
    rr_step(10);
    CHECK(rr_value(RR_V) > 0.0, "demo locks the controls");
    rr_reset(0);
    rr_step(1000);
    CHECK(rr_value(RR_STEPS) == 400, "one call advances at most 400 steps");
    rr_reset(0);
    rr_thrust(1);
    rr_step(-3);
    CHECK(rr_value(RR_STEPS) == 0, "negative step count ignored");
    for (int i = 0; i < 30; i++) rr_step(400);
    CHECK(rr_value(RR_STEPS) == RR_MAX_STEPS && rr_value(RR_DONE) == 1, "manual run stops at the step cap");
    CHECK(rr_value(RR_TRAIL) <= RR_TRAIL_MAX, "trail bounded");
}

static void trace(void)
{
    printf("mode,step");
    for (int f = 0; f < RR_FIELDS; f++) printf(",f%d", f);
    printf(",trail_t,trail_x\n");
    for (int m = 0; m < 2; m++) {
        rr_reset(m);
        for (int step = 0; step <= 1800; step++) {
            if (step % 100 == 0) {
                int n = (int)rr_value(RR_TRAIL);
                printf("%d,%d", m, step);
                for (int f = 0; f < RR_FIELDS; f++) printf(",%.17g", rr_value(f));
                printf(",%.17g,%.17g\n", rr_trail_t(n - 1), rr_trail_x(n - 1));
            }
            if (m == 0) rr_thrust(manual_thrust(step));
            rr_step(1);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--trace") == 0) {
        trace();
        return 0;
    }
    test_demo_matches_closed_form();
    test_adapter_is_identical_to_rocket_api();
    test_interval_and_trail();
    test_manual_controls();
    if (failures) {
        printf("%d of %d adapter checks failed\n", failures, checks);
        return 1;
    }
    printf("adapter: %d checks passed\n", checks);
    return 0;
}
