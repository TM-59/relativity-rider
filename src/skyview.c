#include "skyview.h"
#include "aberration.h"

#include <math.h>

#define PI 3.14159265358979323846

/* The catalogue is the only random thing here. Its stream (positions, then colours and magnitudes from separate
 * sub-seeds) is drawn once from SV_SEED and never depends on beta: the physics takes no random input. */
typedef struct {
    double cos_rest, azimuth, temp, flux;
} Star;

static Star cat[SV_STARS_MAX];
static int catalogued;
static double beta = 0.0, values[SV_FIELDS], stars[SV_STARS_MAX][SV_STAR_FIELDS], curves[SV_CURVES][SV_CURVE_POINTS];

static unsigned long long splitmix(unsigned long long *s)
{
    unsigned long long z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static double uniform(unsigned long long *s) { return (double)(splitmix(s) >> 11) * (1.0 / 9007199254740992.0); }

static void catalogue(void)
{
    unsigned long long where = SV_SEED, what = SV_SEED;
    splitmix(&what); /* sub-seed: the colour stream starts one draw later of a re-seeded copy */
    what = splitmix(&what);
    for (int i = 0; i < SV_STARS_MAX; i++) {
        cat[i].cos_rest = 2.0 * uniform(&where) - 1.0; /* even over the sphere */
        cat[i].azimuth = 2.0 * PI * uniform(&where);
        cat[i].temp = 3000.0 * pow(5.0, uniform(&what)); /* 3000 K to 15000 K, even in log T */
        cat[i].flux = pow(10.0, -0.4 * 5.0 * uniform(&what)); /* magnitudes 0 to 5 at rest */
    }
    catalogued = 1;
}

static void place(double cos_theta, double azimuth, double *x, double *y)
{
    double rho = acos(cos_theta) / PI;
    *x = rho * cos(azimuth);
    *y = rho * sin(azimuth);
}

int sv_reset(double next)
{
    if (!(next >= 0.0) || next > 0.995) return -1;
    if (!catalogued) catalogue();
    beta = next;
    double g = ab_gamma(beta), rgb[3];
    int forward = 0, rest_forward = 0;
    double flux_rest = 0.0, flux_ship = 0.0;
    for (int i = 0; i < SV_STARS_MAX; i++) {
        const Star *s = &cat[i];
        double *o = stars[i], c = ab_cos_ship(beta, s->cos_rest), D = ab_doppler_rest(beta, s->cos_rest);
        place(s->cos_rest, s->azimuth, &o[SV_X0], &o[SV_Y0]);
        place(c, s->azimuth, &o[SV_X], &o[SV_Y]);
        o[SV_COS_REST] = s->cos_rest;
        o[SV_COS_SHIP] = c;
        o[SV_DOPPLER] = D;
        o[SV_TEMP_REST] = s->temp;
        o[SV_TEMP_SHIP] = D * s->temp;
        o[SV_STAR_FLUX_REST] = s->flux;
        o[SV_STAR_FLUX_SHIP] = s->flux * ab_band_ratio(D, s->temp);
        ab_colour(s->temp, rgb);
        o[SV_R0] = rgb[0]; o[SV_G0] = rgb[1]; o[SV_B0] = rgb[2];
        ab_colour(D * s->temp, rgb);
        o[SV_R] = rgb[0]; o[SV_G] = rgb[1]; o[SV_B] = rgb[2];
        forward += c >= 0.0;
        rest_forward += s->cos_rest >= 0.0;
        flux_rest += o[SV_STAR_FLUX_REST];
        flux_ship += o[SV_STAR_FLUX_SHIP];
    }
    double ahead = g * (1.0 + beta), behind = g * (1.0 - beta);
    values[SV_BETA] = beta;
    values[SV_GAMMA] = g;
    values[SV_RAPIDITY] = ab_rapidity(beta);
    values[SV_STARS] = SV_STARS_MAX;
    values[SV_HALF_CONE] = ab_half_cone(beta);
    values[SV_FORWARD] = forward;
    values[SV_FORWARD_EXPECTED] = SV_STARS_MAX * ab_cone_fraction(beta, 0.0);
    values[SV_REST_FORWARD] = rest_forward;
    values[SV_D_AHEAD] = ahead;
    values[SV_D_BEHIND] = behind;
    values[SV_T_AHEAD] = ahead * SV_SUN;
    values[SV_T_BEHIND] = behind * SV_SUN;
    values[SV_BAND_AHEAD] = ab_band_ratio(ahead, SV_SUN);
    values[SV_BAND_BEHIND] = ab_band_ratio(behind, SV_SUN);
    values[SV_FLUX_REST] = flux_rest;
    values[SV_FLUX_SHIP] = flux_ship;
    values[SV_CURVE_N] = SV_CURVE_POINTS;
    values[SV_SUN_T] = SV_SUN;
    for (int i = 0; i < SV_CURVE_POINTS; i++) {
        double th = i * PI / 180.0, cs = cos(th), D = ab_doppler_ship(beta, cs);
        curves[SV_C_THETA_SHIP][i] = acos(ab_cos_ship(beta, cs)) * 180.0 / PI;
        curves[SV_C_DOPPLER][i] = D;
        curves[SV_C_DENSITY][i] = D * D;
        curves[SV_C_BAND][i] = ab_band_ratio(D, SV_SUN);
        curves[SV_C_TEMP][i] = D * SV_SUN;
    }
    return 0;
}

double sv_value(int field) { return field >= 0 && field < SV_FIELDS ? values[field] : NAN; }

double sv_star(int i, int field)
{
    return i >= 0 && i < SV_STARS_MAX && field >= 0 && field < SV_STAR_FIELDS ? stars[i][field] : NAN;
}

double sv_curve(int i, int which)
{
    return i >= 0 && i < SV_CURVE_POINTS && which >= 0 && which < SV_CURVES ? curves[which][i] : NAN;
}
