#ifndef SCHWARZSCHILD_H
#define SCHWARZSCHILD_H

/* Free fall around a non-rotating black hole (the Schwarzschild solution), in the equatorial plane.
 * Geometric units G = c = 1: lengths and times in units of the mass M (for the Sun M = 1.477 km = 4.93 us).
 *
 * With the conserved energy E and angular momentum L per unit mass, the radial motion obeys
 *     (dr/ds)^2 / 2 + V(r) = H   (constant),
 * where s is the proper time (massive), an affine parameter (light) or the time (Newton), and
 *     massive: V = (1 - 2M/r)(1 + L^2/r^2) / 2,  H = E^2 / 2
 *     light:   V = (1 - 2M/r) L^2/r^2 / 2,       H = E^2 / 2
 *     Newton:  V = -M/r + L^2 / (2 r^2),         H = E (the specific energy).
 * The angle and the far-away clock follow from r alone: dphi/ds = L/r^2, dt/ds = E/(1 - 2M/r) (Newton: 1). */
enum { GEO_MASSIVE = 0, GEO_LIGHT = 1, GEO_NEWTON = 2 };
enum { GEO_MOVING = 0, GEO_CAPTURED = 1 };
/* geo_apsides: what an orbit does from its current radius. */
enum { GEO_BOUND = 0, GEO_UNBOUND = 1, GEO_PLUNGES = 2 };

typedef struct {
    int kind;
    double M, E, L;
    double r, pr;     /* radius and dr/ds */
    double phi, t, s; /* angle, far-away clock, proper time / affine parameter / Newton time */
    int state;        /* GEO_MOVING or GEO_CAPTURED (crossed r = 2M (1 + 1e-3), or r = 1e-6 for Newton) */
} Orbit;

double geo_potential(int kind, double M, double L, double r);
double geo_force(int kind, double M, double L, double r); /* -dV/dr */
/* Start at radius r with dr/ds = pr and phi = t = s = 0; E (or Newton's energy) follows from H = pr^2/2 + V. */
void geo_init(Orbit *o, int kind, double M, double L, double r, double pr);
/* The bound orbit with periapsis rp and apoapsis ra (massive or Newton), started at periapsis. Returns -1 if none. */
int geo_from_apsides(Orbit *o, int kind, double M, double rp, double ra);
double geo_energy(const Orbit *o); /* H = pr^2/2 + V(r); constant along the exact orbit */
double geo_rate(const Orbit *o);   /* d(phi)/ds, d(t)/ds are L/r^2 and this: E / (1 - 2M/r) (Newton: 1) */
/* One step of size h: the fourth-order symmetric composition (Yoshida 1990) of the kick-drift-kick leapfrog. */
void geo_step(Orbit *o, double h);
/* One plain leapfrog step (second order), kept for the convergence test. */
void geo_leapfrog(Orbit *o, double h);
/* From a state just before a periapsis (pr < 0 now, pr >= 0 one step of h later), the fraction of h at which pr = 0. */
double geo_apsis_step(const Orbit *before, double h);
/* Periapsis rp and apoapsis ra of the orbit through the current state (massive or Newton), and whether it is bound. */
int geo_apsides(const Orbit *o, double *rp, double *ra);

/* Closed forms. */
double geo_circular_L(double M, double r);      /* L of the circular orbit at r: sqrt(M r^2 / (r - 3M)); -1 inside 3M */
double geo_circular_radius(double M, double L); /* the stable circular radius for L (0 if L^2 < 12 M^2) */
double geo_unstable_radius(double M, double L); /* the unstable circular radius for L (0 if L^2 < 12 M^2) */
double geo_isco(double M);                      /* 6M */
double geo_photon_sphere(double M);             /* 3M */
double geo_critical_b(double M);                /* 3 sqrt(3) M: light with a smaller impact parameter falls in */
double geo_precession_exact(double M, double rp, double ra);  /* per orbit, from the elliptic integral */
double geo_precession_weak(double M, double a, double e);     /* 6 pi M / (a (1 - e^2)) */
/* Angle swept between two turning points (bound) or from radius r_from to the turning point and back (light, unbound),
 * by quadrature of dphi = du / sqrt(2 (H - V(1/u)) / L^2). */
double geo_swept_angle(const Orbit *o, double r_from);
double geo_deflection_weak(double M, double b); /* 4M/b */
double geo_static_rate(double M, double r);     /* d(tau)/dt of a clock held at rest at r: sqrt(1 - 2M/r) */
double geo_circular_rate(double M, double r);   /* d(tau)/dt on the circular orbit at r: sqrt(1 - 3M/r) */
double geo_local_speed(double M, double r);     /* speed of the circular orbit seen by a static observer: sqrt(M / (r - 2M)) */
double geo_fall_time(double M, double R, double r); /* proper time of a radial fall from rest at R down to r */

#endif
