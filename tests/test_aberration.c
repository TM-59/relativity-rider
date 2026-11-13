/* Checks for the view from the window (src/aberration.c) and its browser adapter (src/skyview.c): known values,
 * the round trip, composition like velocity addition, the Doppler factor against an independent Lorentz boost,
 * the solid angle and the star count, the powers of D for intensity and flux, Wien's law and the colours.
 * With --trace, prints the native samples that scripts/smoke.cjs compares with the WASM build. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/aberration.h"
#include "../src/relativity.h"
#include "../src/skyview.h"

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

static const double BETAS[] = {0.0, 1e-9, 0.1, 0.5, 0.9, 0.99, 0.995};
#define NBETAS (int)(sizeof BETAS / sizeof BETAS[0])

static void test_known_values(void)
{
    /* beta = 0.5: a star square to the motion is seen at 60 degrees, blue-shifted by gamma. */
    CHECK(fabs(ab_cos_ship(0.5, 0.0) - 0.5) < 1e-15, "cos theta' at 90 deg, beta 0.5: %.17g", ab_cos_ship(0.5, 0.0));
    CHECK(fabs(ab_doppler_rest(0.5, 0.0) - 2.0 / sqrt(3.0)) < 1e-15, "D at 90 deg rest, beta 0.5");
    /* beta = 0.9: 90 deg comes forward to acos 0.9 = 25.84 deg; D ahead sqrt(19), behind 1 / sqrt(19). */
    CHECK(fabs(acos(ab_cos_ship(0.9, 0.0)) * 180.0 / PI - 25.841932763167) < 1e-9, "theta' at 90 deg, beta 0.9");
    CHECK(fabs(ab_doppler_rest(0.9, 1.0) - sqrt(19.0)) < 1e-13, "D ahead at 0.9: %.17g", ab_doppler_rest(0.9, 1.0));
    CHECK(fabs(ab_doppler_rest(0.9, -1.0) - 1.0 / sqrt(19.0)) < 1e-14, "D behind at 0.9");
    CHECK(fabs(ab_doppler_ship(0.9, 0.0) - sqrt(0.19)) < 1e-14, "D at 90 deg on board is 1 / gamma");
    CHECK(fabs(ab_half_cone(0.9) - acos(0.9)) < 1e-15 && ab_cone_fraction(0.9, 0.9) == 0.5, "half of the stars inside acos(beta)");
    /* The directions ahead and behind stay put. */
    for (int k = 0; k < NBETAS; k++) {
        double b = BETAS[k];
        CHECK(ab_cos_ship(b, 1.0) == 1.0 && ab_cos_ship(b, -1.0) == -1.0, "fixed points at beta %g", b);
    }
}

static void test_identity_and_round_trip(void)
{
    double worst0 = 0.0, worstD = 0.0, worst = 0.0, worstD2 = 0.0, worstT = 0.0;
    for (int i = 0; i <= 2000; i++) {
        double c = -1.0 + i / 1000.0;
        worst0 = fmax(worst0, fabs(ab_cos_ship(1e-12, c) - c));
        worstD = fmax(worstD, fabs(ab_doppler_rest(1e-12, c) - 1.0));
        for (int k = 0; k < NBETAS; k++) {
            double b = BETAS[k], cs = ab_cos_ship(b, c);
            worst = fmax(worst, fabs(ab_cos_rest(b, cs) - c));
            /* The two forms of D agree, and going back multiplies by 1 / D. */
            worstD2 = fmax(worstD2, fabs(ab_doppler_ship(b, cs) / ab_doppler_rest(b, c) - 1.0));
            worstD2 = fmax(worstD2, fabs(ab_doppler_rest(b, c) * ab_doppler_rest(-b, cs) - 1.0));
            if (c > -0.999 && c < 0.999) worstT = fmax(worstT, fabs(cos(ab_theta_ship(b, acos(c))) - cs));
        }
    }
    CHECK(worst0 < 2e-12 && worstD < 2e-12, "beta -> 0 is the identity: %.3g, %.3g", worst0, worstD);
    CHECK(worst < 1e-13, "round trip beta then -beta: %.3g", worst);
    CHECK(worstD2 < 1e-13, "two forms of D, and D(beta) D(-beta) = 1: %.3g", worstD2);
    CHECK(worstT < 1e-13, "tan(theta'/2) = exp(-rapidity) tan(theta/2): %.3g", worstT);
}

static void test_composition(void)
{
    /* Two boosts along the same line compose like velocities: the aberrations compose likewise. */
    double worst = 0.0, worstD = 0.0;
    for (double b1 = -0.9; b1 <= 0.91; b1 += 0.3)
        for (double b2 = -0.6; b2 <= 0.91; b2 += 0.5)
            for (int i = 0; i <= 100; i++) {
                double c = -1.0 + i / 50.0, b = add_velocity(b1, b2);
                worst = fmax(worst, fabs(ab_cos_ship(b2, ab_cos_ship(b1, c)) - ab_cos_ship(b, c)));
                worstD = fmax(worstD, fabs(ab_doppler_rest(b1, c) * ab_doppler_rest(b2, ab_cos_ship(b1, c)) / ab_doppler_rest(b, c) - 1.0));
            }
    CHECK(worst < 1e-14 && worstD < 1e-14, "aberration and D compose like velocity addition: %.3g %.3g", worst, worstD);
}

static void test_against_boost(void)
{
    /* Independent check: a photon from a star at angle theta travels along -(cos, sin) with 4-momentum
     * omega (1, -cos theta, -sin theta). The Lorentz boost of src/relativity.c gives omega' and k'_x. */
    double worst = 0.0, worstD = 0.0, worstR = 0.0;
    for (int k = 2; k < NBETAS; k++)
        for (int i = 0; i <= 180; i++) {
            double b = BETAS[k], th = i * PI / 180.0, w2, kx2, ky = -sin(th);
            boost(atanh(b), 1.0, -cos(th), &w2, &kx2);
            worstD = fmax(worstD, fabs(w2 / ab_doppler_rest(b, cos(th)) - 1.0));
            worst = fmax(worst, fabs(-kx2 / w2 - ab_cos_ship(b, cos(th))));
            worst = fmax(worst, fabs(kx2 * kx2 + ky * ky - w2 * w2)); /* still null */
            /* The emission event, a light-travel time r = 1 back along the ray, is D r away on board. */
            double t2, x2;
            boost(atanh(b), -1.0, cos(th), &t2, &x2);
            worstR = fmax(worstR, fabs(-t2 / ab_doppler_rest(b, cos(th)) - 1.0));
        }
    CHECK(worstD < 1e-13, "D = omega' / omega from the Lorentz boost: %.3g", worstD);
    CHECK(worst < 1e-12, "aberration from the boosted wave vector: %.3g", worst);
    CHECK(worstR < 1e-13, "the distance to the emission event becomes D r: %.3g", worstR);
}

static void test_solid_angle_and_counts(void)
{
    /* d cos theta' / d cos theta = 1 / D^2 (central differences), and the sky still adds up to 4 pi. */
    double worst = 0.0;
    for (int k = 2; k < NBETAS - 1; k++)
        for (int i = 1; i < 200; i++) {
            double b = BETAS[k], c = -1.0 + i / 100.0, h = 1e-6;
            double fd = (ab_cos_ship(b, c + h) - ab_cos_ship(b, c - h)) / (2.0 * h);
            worst = fmax(worst, fabs(fd / ab_solid_angle(b, c) - 1.0));
        }
    CHECK(worst < 1e-6, "dOmega'/dOmega = 1/D^2 against finite differences: %.3g", worst);
    for (int k = 0; k < NBETAS; k++) {
        enum { N = 20000 };
        double b = BETAS[k], sum = ab_solid_angle(b, -1.0) + ab_solid_angle(b, 1.0), h = 2.0 / N;
        for (int i = 1; i < N; i++) sum += (i % 2 ? 4.0 : 2.0) * ab_solid_angle(b, -1.0 + i * h);
        CHECK(fabs(sum * h / 3.0 - 2.0) < 1e-7, "integral of dOmega' over the sky is 4 pi at beta %g: %.12g", b, 2.0 * PI * sum * h / 3.0);
        CHECK(fabs(ab_cone_fraction(b, 0.0) - 0.5 * (1.0 + b)) < 1e-15, "fraction ahead (1 + beta) / 2 at %g", b);
    }
    /* The catalogue: counts ahead stay within 4 standard deviations of N (1 + beta) / 2; the number of stars is kept. */
    for (int k = 0; k < NBETAS; k++) {
        double b = BETAS[k];
        sv_reset(b);
        double n = sv_value(SV_STARS), p = 0.5 * (1.0 + b), sd = sqrt(n * p * (1.0 - p));
        CHECK(fabs(sv_value(SV_FORWARD) - sv_value(SV_FORWARD_EXPECTED)) <= 4.0 * fmax(sd, 0.5),
              "ahead at beta %g: %g stars, expected %g", b, sv_value(SV_FORWARD), sv_value(SV_FORWARD_EXPECTED));
        int inside = 0;
        for (int i = 0; i < SV_STARS_MAX; i++) inside += sv_star(i, SV_COS_SHIP) >= b - 1e-12;
        CHECK(inside == (int)sv_value(SV_REST_FORWARD), "the cone acos(beta) holds the stars of the rest-frame front half (%d vs %g)", inside, sv_value(SV_REST_FORWARD));
    }
}

/* Integral of f(nu) over [0, top] with Simpson's rule on n intervals. */
static double integrate(double (*f)(double, double, double), double a1, double a2, double top, int n)
{
    double h = top / n, sum = f(0.0, a1, a2) + f(top, a1, a2);
    for (int i = 1; i < n; i++) sum += (i % 2 ? 4.0 : 2.0) * f(i * h, a1, a2);
    return sum * h / 3.0;
}
static double seen(double nu, double D, double T) { return D * D * D * ab_planck(nu / D, T); }

static void test_intensity_powers(void)
{
    /* I_nu / nu^3 is invariant: the shifted black body at T is the black body at D T. */
    double worst = 0.0;
    for (int k = 2; k < NBETAS; k++)
        for (int i = 1; i <= 100; i++) {
            double D = ab_doppler_rest(BETAS[k], -1.0 + i / 50.0), nu = i * 4e-5, T = 5800.0;
            worst = fmax(worst, fabs(seen(nu, D, T) / ab_planck(nu, D * T) - 1.0));
        }
    CHECK(worst < 1e-12, "D^3 B(nu/D, T) = B(nu, D T): %.3g", worst);
    /* Bolometric intensity D^4 (Stefan), flux of a point star D^2, over the whole spectrum. */
    for (int k = 2; k < NBETAS; k++) {
        double D = ab_doppler_rest(BETAS[k], 1.0), T = 5800.0, top = 40.0 * D * T / AB_C2;
        double rest = integrate(seen, 1.0, T, 40.0 * T / AB_C2, 200000), moving = integrate(seen, D, T, top, 200000);
        double stefan = pow(T / AB_C2, 4.0) * pow(PI, 4.0) / 15.0;
        CHECK(fabs(rest / stefan - 1.0) < 1e-6, "integral of B_nu is (T / C2)^4 pi^4 / 15: %.3g", rest / stefan - 1.0);
        CHECK(fabs(moving / rest / pow(D, 4.0) - 1.0) < 1e-6, "bolometric intensity grows as D^4 at D %g", D);
    }
    /* The visible-band ratio: for D = 1 it is 1; ahead (blue) it is above 1 at moderate D; it tends to D^2 only
     * when the band covers the whole spectrum (the D^4 integral above, over the D^-2 of the solid angle). */
    CHECK(fabs(ab_band_ratio(1.0, 5800.0) - 1.0) < 1e-15, "no shift, no change");
    for (double T = 3000.0; T < 70001.0; T *= 2.0) {
        enum { N = 40000 };
        double a = 1.0 / AB_BAND_HI, b = 1.0 / AB_BAND_LO, h = (b - a) / N, sum = ab_planck(a, T) + ab_planck(b, T);
        for (int i = 1; i < N; i++) sum += (i % 2 ? 4.0 : 2.0) * ab_planck(a + i * h, T);
        CHECK(fabs(ab_band(T, AB_BAND_LO, AB_BAND_HI) / (sum * h / 3.0) - 1.0) < 1e-10, "visible band with 400 Simpson intervals at %g K", T);
    }
    CHECK(ab_band_ratio(1.5, 5800.0) > 1.0 && ab_band_ratio(1.0 / 1.5, 5800.0) < 1.0, "brighter ahead, fainter behind");
    CHECK(ab_band_ratio(sqrt(19.0) * 10.0, 5800.0) < ab_band_ratio(sqrt(19.0), 5800.0), "a very large D pushes the light out of the visible band");
}

static double planck_lambda(double lambda, double T) { return 1.0 / (pow(lambda, 5.0) * expm1(AB_C2 / (lambda * T))); }

static void test_wien_and_colour(void)
{
    for (double T = 3000.0; T <= 30001.0; T *= 1.7) {
        double a = 50.0, b = 3000.0, g = 0.5 * (sqrt(5.0) - 1.0);
        for (int i = 0; i < 200; i++) {
            double x1 = b - g * (b - a), x2 = a + g * (b - a);
            if (planck_lambda(x1, T) > planck_lambda(x2, T)) b = x2; else a = x1;
        }
        CHECK(fabs(0.5 * (a + b) / ab_peak_nm(T) - 1.0) < 1e-7, "peak of B_lambda at %g K: %.6f nm vs %.6f", T, 0.5 * (a + b), ab_peak_nm(T));
    }
    double cool[3], white[3], hot[3];
    ab_colour(3000.0, cool);
    ab_colour(6500.0, white);
    ab_colour(20000.0, hot);
    CHECK(cool[0] == 1.0 && cool[2] < 0.3, "3000 K is red: %g %g %g", cool[0], cool[1], cool[2]);
    CHECK(fabs(white[0] - 1.0) < 1e-15 && fabs(white[1] - 1.0) < 1e-15 && fabs(white[2] - 1.0) < 1e-15, "6500 K is white");
    CHECK(hot[2] == 1.0 && hot[0] < 0.8, "20000 K is blue: %g %g %g", hot[0], hot[1], hot[2]);
}

static void test_adapter(void)
{
    CHECK(sv_reset(-0.1) == -1 && sv_reset(0.996) == -1 && sv_reset(NAN) == -1, "bad beta is refused");
    sv_reset(0.0);
    double worst = 0.0;
    for (int i = 0; i < SV_STARS_MAX; i++)
        for (int f = 0; f < 2; f++) worst = fmax(worst, fabs(sv_star(i, SV_X0 + f) - sv_star(i, SV_X + f)));
    CHECK(worst == 0.0 && sv_value(SV_FORWARD) == sv_value(SV_REST_FORWARD) && sv_value(SV_FLUX_SHIP) == sv_value(SV_FLUX_REST),
          "at rest the window shows the rest-frame sky");
    double x7 = sv_star(7, SV_X0), t7 = sv_star(7, SV_TEMP_REST);
    sv_reset(0.9);
    CHECK(sv_star(7, SV_X0) == x7 && sv_star(7, SV_TEMP_REST) == t7, "the catalogue does not depend on beta");
    worst = 0.0;
    double rmax = 0.0;
    for (int i = 0; i < SV_STARS_MAX; i++) {
        double c = sv_star(i, SV_COS_REST), cs = sv_star(i, SV_COS_SHIP), rho = hypot(sv_star(i, SV_X), sv_star(i, SV_Y));
        worst = fmax(worst, fabs(cs - ab_cos_ship(0.9, c)) + fabs(sv_star(i, SV_DOPPLER) - ab_doppler_rest(0.9, c)));
        worst = fmax(worst, fabs(rho - acos(cs) / PI) + fabs(sv_star(i, SV_TEMP_SHIP) - sv_star(i, SV_DOPPLER) * sv_star(i, SV_TEMP_REST)));
        rmax = fmax(rmax, rho);
        CHECK(sv_star(i, SV_TEMP_REST) >= 3000.0 && sv_star(i, SV_TEMP_REST) <= 15000.0, "temperature range");
    }
    CHECK(worst < 1e-12 && rmax <= 1.0, "every star placed by the closed forms: %.3g", worst);
    CHECK(fabs(sv_value(SV_D_AHEAD) - sqrt(19.0)) < 1e-13 && fabs(sv_value(SV_T_AHEAD) - sqrt(19.0) * SV_SUN) < 1e-9, "ahead at 0.9");
    CHECK(fabs(sv_curve(90, SV_C_THETA_SHIP) - 25.841932763167) < 1e-9 && fabs(sv_curve(90, SV_C_DOPPLER) - sqrt(0.19)) < 1e-14, "curves at 90 deg");
    CHECK(fabs(sv_curve(0, SV_C_DENSITY) - 19.0) < 1e-12, "19 times as many stars per solid angle dead ahead at 0.9");
    CHECK(isnan(sv_star(-1, 0)) && isnan(sv_star(0, SV_STAR_FIELDS)) && isnan(sv_value(SV_FIELDS)) && isnan(sv_curve(181, 0)), "out of range is NaN");
    double again = sv_star(123, SV_STAR_FLUX_SHIP);
    sv_reset(0.3);
    sv_reset(0.9);
    CHECK(sv_star(123, SV_STAR_FLUX_SHIP) == again, "a reset is deterministic");
}

static const double TRACE_BETAS[] = {0.0, 0.3, 0.6, 0.9, 0.99};
static const int TRACE_STARS[] = {0, 137, 599};
static const int TRACE_ANGLES[] = {0, 45, 90, 135, 180};

static void trace(void)
{
    printf("beta");
    for (int f = 0; f < SV_FIELDS; f++) printf(",f%d", f);
    for (int s = 0; s < 3; s++)
        for (int f = 0; f < SV_STAR_FIELDS; f++) printf(",s%d_%d", TRACE_STARS[s], f);
    for (int a = 0; a < 5; a++)
        for (int c = 0; c < SV_CURVES; c++) printf(",c%d_%d", TRACE_ANGLES[a], c);
    printf("\n");
    for (int k = 0; k < 5; k++) {
        sv_reset(TRACE_BETAS[k]);
        printf("%.17g", TRACE_BETAS[k]);
        for (int f = 0; f < SV_FIELDS; f++) printf(",%.17g", sv_value(f));
        for (int s = 0; s < 3; s++)
            for (int f = 0; f < SV_STAR_FIELDS; f++) printf(",%.17g", sv_star(TRACE_STARS[s], f));
        for (int a = 0; a < 5; a++)
            for (int c = 0; c < SV_CURVES; c++) printf(",%.17g", sv_curve(TRACE_ANGLES[a], c));
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--trace") == 0) {
        trace();
        return 0;
    }
    test_known_values();
    test_identity_and_round_trip();
    test_composition();
    test_against_boost();
    test_solid_angle_and_counts();
    test_intensity_powers();
    test_wien_and_colour();
    test_adapter();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
