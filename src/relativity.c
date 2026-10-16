#include "relativity.h"

#include <math.h>

double velocity(const Rocket *r)
{
    return tanh(r->phi);
}

double lorentz_gamma(const Rocket *r)
{
    return cosh(r->phi);
}

void rocket_step(Rocket *r, double a, double dtau)
{
    double phi2 = r->phi + a * dtau;
    if (fabs(a * dtau) < 1e-12) {
        r->t += cosh(r->phi) * dtau;
        r->x += sinh(r->phi) * dtau;
    } else {
        r->t += (sinh(phi2) - sinh(r->phi)) / a;
        r->x += (cosh(phi2) - cosh(r->phi)) / a;
    }
    r->phi = phi2;
    r->tau += dtau;
}

double interval2(double dt, double dx)
{
    return dt * dt - dx * dx;
}

void boost(double phi, double t, double x, double *t2, double *x2)
{
    double ch = cosh(phi), sh = sinh(phi);
    *t2 = ch * t - sh * x;
    *x2 = ch * x - sh * t;
}

double add_velocity(double u, double v)
{
    return (u + v) / (1.0 + u * v);
}
