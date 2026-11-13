#ifndef ORBIT_H
#define ORBIT_H

/* Browser adapter for the black-hole experiment: one geodesic of src/schwarzschild.c (G = c = M = 1) and,
 * beside it, the same start without the relativistic correction (Newton's gravity for a particle, a straight
 * line for light). JavaScript reads plain doubles and draws them. */
enum {
    SO_KIND, SO_M, SO_R0, SO_L, SO_E, SO_H, SO_STEPS, SO_S, SO_T, SO_R, SO_PHI, SO_X, SO_Y, SO_PR,
    SO_STATE, SO_BOUND, SO_RP, SO_RA, SO_APSES, SO_PREC, SO_PREC_EXACT, SO_PREC_WEAK, SO_DRIFT,
    SO_CLOCK_RATE, SO_STATIC_RATE, SO_ISCO, SO_PHOTON, SO_BCRIT, SO_CIRC_STABLE, SO_CIRC_UNSTABLE,
    SO_NX, SO_NY, SO_NR, SO_NSTATE, SO_NBOUND, SO_NRP, SO_NRA,
    SO_TRAIL, SO_VIEW, SO_CURVE_N, SO_CURVE_R0, SO_CURVE_R1, SO_LEVEL, SO_NLEVEL,
    SO_DEFLECT, SO_DEFLECT_INNER, SO_DEFLECT_TOTAL, SO_DEFLECT_WEAK,
    SO_FIELDS
};
/* SO_STATE / SO_NSTATE */
enum { SO_MOVING = 0, SO_CAPTURED = 1, SO_ESCAPED = 2, SO_LIMIT = 3 };
#define SO_TRAIL_MAX 6000
#define SO_TRAIL_EVERY 2
#define SO_APSES_MAX 256
#define SO_MAX_STEPS 400000
#define SO_CALL_MAX 2000
#define SO_CURVE_POINTS 240
#define SO_LIGHT_START 30.0

/* kind 0: a particle at rest radially at r0 (a turning point) with angular momentum L.
 * kind 1: light from SO_LIGHT_START with impact parameter b (= L, with E = 1). Returns 0, or -1 for bad input. */
int so_reset(int kind, double r0, double L);
void so_step(int steps); /* at most SO_CALL_MAX per call */
double so_value(int field);
double so_trail_x(int i); /* oldest first; the geodesic */
double so_trail_y(int i);
double so_ntrail_x(int i); /* the same start without the correction */
double so_ntrail_y(int i);
double so_apsis(int i);   /* angle of the i-th periapsis passage (latest SO_APSES_MAX) */
double so_curve(int i);   /* 2V(r) of the geodesic on SO_CURVE_POINTS radii; the motion stays where it is below SO_LEVEL */
double so_ncurve(int i);  /* the same for the comparison (Newton: 1 + 2V, flat light: L^2 / r^2) */

#endif
