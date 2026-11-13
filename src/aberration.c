#include "aberration.h"

#include <math.h>

double ab_gamma(double beta) { return 1.0 / sqrt((1.0 - beta) * (1.0 + beta)); }

double ab_rapidity(double beta) { return atanh(beta); }

double ab_cos_ship(double beta, double cos_rest)
{
    double c = (cos_rest + beta) / (1.0 + beta * cos_rest);
    return c > 1.0 ? 1.0 : c < -1.0 ? -1.0 : c;
}

double ab_cos_rest(double beta, double cos_ship) { return ab_cos_ship(-beta, cos_ship); }

double ab_theta_ship(double beta, double theta_rest)
{
    /* tan(theta' / 2) = sqrt((1 - beta) / (1 + beta)) tan(theta / 2) = exp(-rapidity) tan(theta / 2). */
    return 2.0 * atan(exp(-ab_rapidity(beta)) * tan(0.5 * theta_rest));
}

double ab_doppler_rest(double beta, double cos_rest) { return ab_gamma(beta) * (1.0 + beta * cos_rest); }

double ab_doppler_ship(double beta, double cos_ship) { return 1.0 / (ab_gamma(beta) * (1.0 - beta * cos_ship)); }

double ab_solid_angle(double beta, double cos_rest)
{
    double D = ab_doppler_rest(beta, cos_rest);
    return 1.0 / (D * D);
}

double ab_cone_fraction(double beta, double cos_edge) { return 0.5 * (1.0 - ab_cos_rest(beta, cos_edge)); }

double ab_half_cone(double beta) { return acos(beta); }

double ab_planck(double nu, double T)
{
    double x = AB_C2 * nu / T;
    return !(nu > 0.0) || x > 700.0 ? 0.0 : nu * nu * nu / expm1(x);
}

double ab_band(double T, double lambda0, double lambda1)
{
    enum { N = 400 }; /* even */
    double a = 1.0 / lambda1, b = 1.0 / lambda0, h = (b - a) / N, sum = ab_planck(a, T) + ab_planck(b, T);
    for (int i = 1; i < N; i++) sum += (i % 2 ? 4.0 : 2.0) * ab_planck(a + i * h, T);
    return sum * h / 3.0;
}

double ab_band_ratio(double D, double T)
{
    double rest = ab_band(T, AB_BAND_LO, AB_BAND_HI);
    return rest > 0.0 ? ab_band(D * T, AB_BAND_LO, AB_BAND_HI) / (D * D * rest) : 0.0;
}

double ab_peak_nm(double T) { return AB_WIEN / T; }

/* B_lambda up to a constant: lambda^-5 / (exp(C2 / (lambda T)) - 1). */
static double planck_lambda(double lambda, double T)
{
    double x = AB_C2 / (lambda * T);
    return x > 700.0 ? 0.0 : 1.0 / (pow(lambda, 5.0) * expm1(x));
}

void ab_colour(double T, double rgb[3])
{
    static const double band[3] = {610.0, 550.0, 465.0};
    double top = 0.0;
    for (int k = 0; k < 3; k++) {
        rgb[k] = planck_lambda(band[k], T) / planck_lambda(band[k], 6500.0);
        if (rgb[k] > top) top = rgb[k];
    }
    for (int k = 0; k < 3; k++) rgb[k] = top > 0.0 ? rgb[k] / top : 0.0;
}
