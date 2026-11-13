#include "schwarzschild.h"

#include <math.h>

#define PI 3.14159265358979323846
#define HORIZON_MARGIN 1e-3
#define NEWTON_CORE 1e-6

double geo_potential(int kind, double M, double L, double r)
{
    if (kind == GEO_NEWTON) return -M / r + L * L / (2.0 * r * r);
    double f = 1.0 - 2.0 * M / r;
    return 0.5 * f * ((kind == GEO_LIGHT ? 0.0 : 1.0) + L * L / (r * r));
}

double geo_force(int kind, double M, double L, double r)
{
    double r2 = r * r, r3 = r2 * r, L2 = L * L;
    if (kind == GEO_NEWTON) return -M / r2 + L2 / r3;
    if (kind == GEO_LIGHT) return L2 / r3 - 3.0 * M * L2 / (r3 * r);
    return -M / r2 + L2 / r3 - 3.0 * M * L2 / (r3 * r);
}

static double inside(const Orbit *o)
{
    return o->kind == GEO_NEWTON ? NEWTON_CORE : 2.0 * o->M * (1.0 + HORIZON_MARGIN);
}

void geo_init(Orbit *o, int kind, double M, double L, double r, double pr)
{
    o->kind = kind;
    o->M = M;
    o->L = L;
    o->r = r;
    o->pr = pr;
    o->phi = o->t = o->s = 0.0;
    double H = 0.5 * pr * pr + geo_potential(kind, M, L, r);
    o->E = kind == GEO_NEWTON ? H : sqrt(2.0 * H);
    o->state = r > inside(o) ? GEO_MOVING : GEO_CAPTURED;
}

int geo_from_apsides(Orbit *o, int kind, double M, double rp, double ra)
{
    if (!(rp > 0.0) || ra < rp) return -1;
    double L2;
    if (kind == GEO_NEWTON) {
        L2 = 2.0 * M * rp * ra / (rp + ra);
    } else if (kind == GEO_MASSIVE) {
        double u1 = 1.0 / ra, u2 = 1.0 / rp, u3 = 0.5 / M - u1 - u2;
        if (u3 <= u2) return -1; /* the periapsis would be inside the unstable circular orbit */
        L2 = 1.0 / (u1 * u2 + u1 * u3 + u2 * u3);
    } else {
        return -1;
    }
    geo_init(o, kind, M, sqrt(L2), rp, 0.0);
    return 0;
}

double geo_energy(const Orbit *o)
{
    return 0.5 * o->pr * o->pr + geo_potential(o->kind, o->M, o->L, o->r);
}

double geo_rate(const Orbit *o)
{
    return o->kind == GEO_NEWTON ? 1.0 : o->E / (1.0 - 2.0 * o->M / o->r);
}

/* The kick: r fixed, so pr, phi and t advance exactly. The drift: pr fixed, r advances exactly. */
static void kick(Orbit *o, double dt)
{
    o->pr += dt * geo_force(o->kind, o->M, o->L, o->r);
    o->phi += dt * o->L / (o->r * o->r);
    o->t += dt * geo_rate(o);
}

static int drift(Orbit *o, double dt)
{
    o->r += dt * o->pr;
    o->s += dt;
    if (!(o->r > inside(o))) { o->state = GEO_CAPTURED; return 1; }
    return 0;
}

static int kdk(Orbit *o, double h)
{
    kick(o, 0.5 * h);
    if (drift(o, h)) return 1;
    kick(o, 0.5 * h);
    return 0;
}

void geo_leapfrog(Orbit *o, double h)
{
    if (o->state == GEO_MOVING) kdk(o, h);
}

void geo_step(Orbit *o, double h)
{
    static const double c = 1.25992104989487316477; /* 2^(1/3) */
    const double w1 = 1.0 / (2.0 - c), w0 = -c / (2.0 - c);
    if (o->state != GEO_MOVING) return;
    if (kdk(o, w1 * h) || kdk(o, w0 * h)) return;
    kdk(o, w1 * h);
}

double geo_apsis_step(const Orbit *before, double h)
{
    double a = 0.0, fa = before->pr, b = h, fb;
    Orbit o = *before;
    geo_step(&o, h);
    fb = o.pr;
    if (fa >= 0.0) return 0.0;
    if (fb < 0.0) return h;
    for (int k = 0, side = 0; k < 60 && b - a > 1e-15 * h; k++) {
        double x = (a * fb - b * fa) / (fb - fa);
        o = *before;
        geo_step(&o, x);
        if (o.pr < 0.0) {
            a = x; fa = o.pr;
            if (side == -1) fb *= 0.5; /* Illinois: keep the bracket shrinking from both sides */
            side = -1;
        } else {
            b = x; fb = o.pr;
            if (side == 1) fa *= 0.5;
            side = 1;
        }
    }
    return fa == 0.0 ? a : b - a < 1e-15 * h ? 0.5 * (a + b) : (a * fb - b * fa) / (fb - fa);
}

/* Turning points: zeros of g(u) = 2 (H - V(1/u)) / L^2 = (du/dphi)^2.
 *   GR:     g = 2M u^3 - u^2 + 2 M k u / L^2 + (E^2 - k) / L^2  (k = 1 massive, 0 light)
 *   Newton: g = -u^2 + 2 M u / L^2 + 2 E / L^2  (also GR with M = 0). */
typedef struct {
    int n, cubic;
    double M, u[3];
} Roots;

static double polish(double M, double a1, double a0, double u)
{
    for (int k = 0; k < 3; k++) {
        double g = ((2.0 * M * u - 1.0) * u + a1) * u + a0, dg = (6.0 * M * u - 2.0) * u + a1;
        if (dg != 0.0) u -= g / dg;
    }
    return u;
}

static Roots roots(const Orbit *o)
{
    Roots R = {0, 0, o->M, {0, 0, 0}};
    double L2 = o->L * o->L, kap = o->kind == GEO_LIGHT ? 0.0 : 1.0;
    if (o->kind == GEO_NEWTON || o->M == 0.0) {
        double a1 = 2.0 * o->M / L2, a0 = o->kind == GEO_NEWTON ? 2.0 * o->E / L2 : (o->E * o->E - kap) / L2;
        double disc = 0.25 * a1 * a1 + a0;
        if (disc < 0.0) return R;
        R.n = 2;
        R.u[0] = 0.5 * a1 - sqrt(disc);
        R.u[1] = 0.5 * a1 + sqrt(disc);
        return R;
    }
    R.cubic = 1;
    double a1 = 2.0 * o->M * kap / L2, a0 = (o->E * o->E - kap) / L2;
    double b = -0.5 / o->M, c = a1 / (2.0 * o->M), d = a0 / (2.0 * o->M);
    double p = c - b * b / 3.0, q = 2.0 * b * b * b / 27.0 - b * c / 3.0 + d;
    double disc = 0.25 * q * q + p * p * p / 27.0;
    if (disc < 0.0) {
        double m = 2.0 * sqrt(-p / 3.0), th = acos(fmax(-1.0, fmin(1.0, 3.0 * q / (p * m)))) / 3.0;
        for (int k = 0; k < 3; k++) R.u[k] = polish(o->M, a1, a0, m * cos(th - 2.0 * PI * k / 3.0) - b / 3.0);
        for (int i = 0; i < 3; i++)
            for (int j = i + 1; j < 3; j++)
                if (R.u[j] < R.u[i]) { double t = R.u[i]; R.u[i] = R.u[j]; R.u[j] = t; }
        R.n = 3;
    } else if (disc <= 1e-10 * (0.25 * q * q - p * p * p / 27.0)) {
        /* a double root (a circular orbit): the trigonometric form loses it to rounding */
        double y = cbrt(-0.5 * q);
        R.u[0] = 2.0 * y - b / 3.0;
        R.u[1] = R.u[2] = -y - b / 3.0;
        if (R.u[0] > R.u[1]) { double t = R.u[0]; R.u[0] = R.u[2] = R.u[1]; R.u[2] = t; }
        R.n = 3;
    } else {
        double s = sqrt(disc);
        R.u[0] = polish(o->M, a1, a0, cbrt(-0.5 * q + s) + cbrt(-0.5 * q - s) - b / 3.0);
        R.n = 1;
    }
    return R;
}

/* g as a product of its factors: accurate near the turning points. */
static double g_of(const Roots *R, double u)
{
    if (!R->cubic) return -(u - R->u[0]) * (u - R->u[1]);
    return 2.0 * R->M * (u - R->u[0]) * (u - R->u[1]) * (u - R->u[2]);
}

int geo_apsides(const Orbit *o, double *rp, double *ra)
{
    *rp = *ra = -1.0;
    if (o->L == 0.0) return GEO_PLUNGES;
    Roots R = roots(o);
    double u = 1.0 / o->r, tol = 1e-6 * u; /* near a double root (a circular orbit) the roots are good to about sqrt(eps) */
    if (R.n < 2 || u < R.u[0] - tol || u > R.u[1] + tol) return GEO_PLUNGES;
    *rp = 1.0 / R.u[1];
    if (R.u[0] <= 0.0) return GEO_UNBOUND;
    *ra = 1.0 / R.u[0];
    return GEO_BOUND;
}

/* 2 * integral of du / sqrt(g) from ua to ub; g vanishes at ub (and at ua when both_ends). Gauss-Legendre, 5 points
 * on 2000 panels, after a sine substitution that removes the inverse square roots at the turning points. */
static double sweep(const Roots *R, double ua, double ub, int both_ends)
{
    static const double x[5] = {0.0, 0.53846931010568309104, -0.53846931010568309104, 0.90617984593866399280, -0.90617984593866399280};
    static const double w[5] = {0.56888888888888888889, 0.47862867049936646804, 0.47862867049936646804, 0.23692688505618908751, 0.23692688505618908751};
    enum { PANELS = 2000 };
    double th0 = both_ends ? -0.5 * PI : 0.0, th1 = 0.5 * PI, span = (th1 - th0) / PANELS, sum = 0.0;
    for (int k = 0; k < PANELS; k++) {
        double mid = th0 + (k + 0.5) * span;
        for (int j = 0; j < 5; j++) {
            double th = mid + 0.5 * span * x[j], u, du;
            if (both_ends) { u = 0.5 * (ua + ub) + 0.5 * (ub - ua) * sin(th); du = 0.5 * (ub - ua) * cos(th); }
            else { u = ub - (ub - ua) * (1.0 - sin(th)); du = (ub - ua) * cos(th); }
            sum += w[j] * 0.5 * span * du / sqrt(g_of(R, u));
        }
    }
    return 2.0 * sum;
}

double geo_swept_angle(const Orbit *o, double r_from)
{
    double rp, ra;
    int kind = geo_apsides(o, &rp, &ra);
    if (kind == GEO_PLUNGES) return -1.0;
    Roots R = roots(o);
    if (kind == GEO_BOUND) return sweep(&R, 1.0 / ra, 1.0 / rp, 1);
    return sweep(&R, 1.0 / r_from, 1.0 / rp, 0);
}

double geo_circular_L(double M, double r)
{
    return r > 3.0 * M ? sqrt(M * r * r / (r - 3.0 * M)) : -1.0;
}

static double circular(double M, double L, double sign)
{
    double L2 = L * L;
    if (L2 < 12.0 * M * M * (1.0 - 1e-12)) return 0.0;
    return L2 / (2.0 * M) * (1.0 + sign * sqrt(fmax(0.0, 1.0 - 12.0 * M * M / L2)));
}

double geo_circular_radius(double M, double L) { return circular(M, L, 1.0); }
double geo_unstable_radius(double M, double L) { return circular(M, L, -1.0); }
double geo_isco(double M) { return 6.0 * M; }
double geo_photon_sphere(double M) { return 3.0 * M; }
double geo_critical_b(double M) { return 3.0 * sqrt(3.0) * M; }

/* Complete elliptic integral of the first kind by the arithmetic-geometric mean. */
static double ellip_k(double k2)
{
    double a = 1.0, b = sqrt(1.0 - k2);
    for (int i = 0; i < 40 && fabs(a - b) > 1e-16 * a; i++) {
        double m = 0.5 * (a + b);
        b = sqrt(a * b);
        a = m;
    }
    return PI / (a + b);
}

double geo_precession_exact(double M, double rp, double ra)
{
    double u1 = 1.0 / ra, u2 = 1.0 / rp, u3 = 0.5 / M - u1 - u2;
    if (u3 <= u2) return -1.0;
    return 4.0 * ellip_k((u2 - u1) / (u3 - u1)) / sqrt(2.0 * M * (u3 - u1)) - 2.0 * PI;
}

double geo_precession_weak(double M, double a, double e) { return 6.0 * PI * M / (a * (1.0 - e * e)); }
double geo_deflection_weak(double M, double b) { return 4.0 * M / b; }
double geo_static_rate(double M, double r) { return sqrt(1.0 - 2.0 * M / r); }
double geo_circular_rate(double M, double r) { return sqrt(1.0 - 3.0 * M / r); }
double geo_local_speed(double M, double r) { return sqrt(M / (r - 2.0 * M)); }

double geo_fall_time(double M, double R, double r)
{
    double q = r / R;
    return sqrt(R * R * R / (2.0 * M)) * (sqrt(q * (1.0 - q)) + acos(sqrt(q)));
}
