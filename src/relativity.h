#ifndef RELATIVITY_H
#define RELATIVITY_H

/* 1+1 dimensional special relativity in units where c = 1 (years and light-years).
 * A proper acceleration of 1.0323 ly/yr^2 is about 1 g. */
typedef struct {
    double t;    /* coordinate time in the rest frame [yr] */
    double x;    /* position [ly] */
    double tau;  /* proper time on board [yr] */
    double phi;  /* rapidity, v = tanh(phi) */
} Rocket;

#define ONE_G 1.0323

double velocity(const Rocket *r);
double lorentz_gamma(const Rocket *r);
/* Advance by dtau of proper time at constant proper acceleration a (exact hyperbolic motion). */
void rocket_step(Rocket *r, double a, double dtau);
/* Interval t^2 - x^2 between two events. */
double interval2(double dt, double dx);
/* Boost an event into a frame moving at rapidity phi. */
void boost(double phi, double t, double x, double *t2, double *x2);
/* Relativistic velocity addition. */
double add_velocity(double u, double v);

#endif
