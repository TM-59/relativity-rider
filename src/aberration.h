#ifndef ABERRATION_H
#define ABERRATION_H

/* The sky seen from a moving ship: aberration of starlight, the Doppler factor, the bunching of stars ahead,
 * the change of brightness and the change of colour. Units c = 1.
 *
 * The ship moves with speed beta along +x through stars at rest in the Earth frame. Angles are measured from
 * the direction of motion to the direction in which a star is seen: theta in the Earth (rest) frame, theta' on
 * board. A photon arriving from the star travels along -(cos theta, sin theta).
 *     aberration:      cos theta' = (cos theta + beta) / (1 + beta cos theta)
 *     Doppler factor:  D = nu' / nu = gamma (1 + beta cos theta) = 1 / (gamma (1 - beta cos theta'))
 *     solid angle:     dOmega' = dOmega / D^2   (stars crowd ahead, thin out behind)
 *     intensity:       I_nu / nu^3 is invariant, so I'_nu'(nu') = D^3 I_nu(nu' / D) and the bolometric I' = D^4 I. */

#define AB_C2 1.438776877e7   /* second radiation constant h c / k [nm K] */
#define AB_WIEN 2.897771955e6 /* Wien's displacement constant: lambda_max T for B_lambda [nm K] */
#define AB_BAND_LO 380.0      /* the visible band used for the brightness on screen [nm] */
#define AB_BAND_HI 750.0

double ab_gamma(double beta);
double ab_rapidity(double beta);                     /* atanh(beta) */
double ab_cos_ship(double beta, double cos_rest);    /* aberration: the angle seen on board */
double ab_cos_rest(double beta, double cos_ship);    /* its inverse, the same formula with -beta */
double ab_theta_ship(double beta, double theta_rest); /* the same through tan(theta' / 2) = exp(-rapidity) tan(theta / 2) */
double ab_doppler_rest(double beta, double cos_rest); /* D from the Earth-frame angle: gamma (1 + beta cos theta) */
double ab_doppler_ship(double beta, double cos_ship); /* D from the ship angle: 1 / (gamma (1 - beta cos theta')) */
double ab_solid_angle(double beta, double cos_rest);  /* dOmega' / dOmega = d cos theta' / d cos theta = 1 / D^2 */
/* Fraction of stars spread evenly over the Earth-frame sky that the ship sees with cos theta' >= cos_edge. */
double ab_cone_fraction(double beta, double cos_edge);
double ab_half_cone(double beta);                     /* acos(beta): half of all stars are seen inside this angle */
double ab_planck(double nu, double T);                /* B_nu up to a constant: nu^3 / (exp(C2 nu / T) - 1), nu = 1 / lambda [1/nm] */
double ab_band(double T, double lambda0, double lambda1); /* integral of B_nu over the band [lambda0, lambda1] nm (Simpson) */
/* Flux of a black-body star in the visible band seen with Doppler factor D, over the same at rest:
 * D^-2 band(D T) / band(T). Over all frequencies it tends to D^2. */
double ab_band_ratio(double D, double T);
double ab_peak_nm(double T);                          /* AB_WIEN / T */
/* Display colour of a black body at T: B_lambda at 610, 550 and 465 nm divided by the same at 6500 K, scaled so
 * the largest channel is 1. */
void ab_colour(double T, double rgb[3]);

#endif
