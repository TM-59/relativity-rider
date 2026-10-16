#include <math.h>
#include <stdio.h>

#include "../src/relativity.h"
#include "../src/render.h"

static int failures;

#define CHECK(cond, ...)                                      \
    do {                                                      \
        if (!(cond)) {                                        \
            failures++;                                       \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);       \
            printf(__VA_ARGS__);                              \
            printf("\n");                                     \
        }                                                     \
    } while (0)

static void test_hyperbolic_motion_matches_closed_form(void)
{
    Rocket r = {0, 0, 0, 0};
    double a = ONE_G;
    for (int k = 0; k < 1000; k++) rocket_step(&r, a, 0.005);
    double tau = 5.0;
    CHECK(fabs(r.t - sinh(a * tau) / a) < 1e-9, "t %.9f vs %.9f", r.t, sinh(a * tau) / a);
    CHECK(fabs(r.x - (cosh(a * tau) - 1.0) / a) < 1e-9, "x %.9f", r.x);
    CHECK(fabs(velocity(&r) - tanh(a * tau)) < 1e-12, "velocity mismatch");
}

static void test_worldline_is_a_hyperbola(void)
{
    Rocket r = {0, 0, 0, 0};
    double a = 0.7;
    for (int k = 0; k < 400; k++) {
        rocket_step(&r, a, 0.01);
        double h = (r.x + 1.0 / a) * (r.x + 1.0 / a) - r.t * r.t;
        CHECK(fabs(h - 1.0 / (a * a)) < 1e-9, "left the hyperbola by %.2e", h - 1.0 / (a * a));
    }
}

static void test_time_dilation_while_coasting(void)
{
    Rocket r = {0, 0, 0, atanh(0.866025403784)};
    CHECK(fabs(lorentz_gamma(&r) - 2.0) < 1e-6, "gamma %.6f should be 2", lorentz_gamma(&r));
    rocket_step(&r, 0.0, 3.0);
    CHECK(fabs(r.t - 6.0) < 1e-6, "3 yr of proper time should be 6 yr outside, got %.6f", r.t);
    CHECK(fabs(r.x / r.t - 0.866025403784) < 1e-9, "coasting speed changed");
}

static void test_boost_preserves_the_interval(void)
{
    double t2, x2;
    boost(0.8, 3.0, 1.5, &t2, &x2);
    CHECK(fabs(interval2(t2, x2) - interval2(3.0, 1.5)) < 1e-12, "interval changed under boost");
    double t3, x3;
    boost(-0.8, t2, x2, &t3, &x3);
    CHECK(fabs(t3 - 3.0) < 1e-12 && fabs(x3 - 1.5) < 1e-12, "inverse boost should restore the event");
}

static void test_velocity_addition_never_exceeds_light(void)
{
    CHECK(fabs(add_velocity(0.9, 0.9) - 1.8 / 1.81) < 1e-12, "0.9 (+) 0.9");
    CHECK(add_velocity(0.999999, 0.999999) < 1.0, "sum must stay below c");
    CHECK(fabs(add_velocity(1.0, 0.5) - 1.0) < 1e-12, "light speed is invariant");
    CHECK(fabs(atanh(add_velocity(tanh(0.3), tanh(0.5))) - 0.8) < 1e-12, "rapidities should add");
}

static void test_flip_and_burn_trip(void)
{
    double a = ONE_G, dist = 4.2;
    double half_tau = acosh(1.0 + a * dist / 2.0) / a;
    Rocket r = {0, 0, 0, 0};
    int n = 4000;
    for (int k = 0; k < n; k++) rocket_step(&r, a, half_tau / n);
    for (int k = 0; k < n; k++) rocket_step(&r, -a, half_tau / n);
    CHECK(fabs(r.x - dist) < 1e-6, "arrival at %.6f ly instead of %.1f", r.x, dist);
    CHECK(fabs(velocity(&r)) < 1e-9, "should arrive at rest, v = %.2e", velocity(&r));
    CHECK(r.tau < r.t, "traveller must age less: tau %.3f vs t %.3f", r.tau, r.t);
    CHECK(r.t > dist, "cannot beat light: t %.3f", r.t);
}

static void test_render_fits_the_screen(void)
{
    Rocket r = {2.0, 1.0, 1.5, 0.5};
    static char buf[RENDER_COLS * RENDER_ROWS * 2];
    size_t used = render_frame(&r, &r, 1, 4.2, buf, sizeof buf);
    int lines = 0;
    for (size_t i = 0; i < used; i++) lines += buf[i] == '\n';
    CHECK(lines == RENDER_ROWS, "expected %d lines, got %d", RENDER_ROWS, lines);
    CHECK(render_frame(&r, &r, 1, 4.2, buf, 8) == 0, "tiny buffer must be refused");
}

int main(void)
{
    test_hyperbolic_motion_matches_closed_form();
    test_worldline_is_a_hyperbola();
    test_time_dilation_while_coasting();
    test_boost_preserves_the_interval();
    test_velocity_addition_never_exceeds_light();
    test_flip_and_burn_trip();
    test_render_fits_the_screen();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all tests passed");
    return 0;
}
