#ifndef SKYVIEW_H
#define SKYVIEW_H

/* Browser adapter for "The view from the window" (src/aberration.c): a fixed catalogue of stars at rest in the
 * Earth frame, seen from a ship moving at beta along +x. JavaScript reads plain doubles and draws them.
 *
 * Map: every star is drawn on a disc centred on the direction of motion, at radius theta / pi (0 ahead, 1 behind)
 * and at its azimuth round the direction of motion (an azimuthal equidistant map of the whole sky). */
enum {
    SV_BETA, SV_GAMMA, SV_RAPIDITY, SV_STARS, SV_HALF_CONE, SV_FORWARD, SV_FORWARD_EXPECTED, SV_REST_FORWARD,
    SV_D_AHEAD, SV_D_BEHIND, SV_T_AHEAD, SV_T_BEHIND, SV_BAND_AHEAD, SV_BAND_BEHIND, SV_FLUX_REST, SV_FLUX_SHIP,
    SV_CURVE_N, SV_SUN_T,
    SV_FIELDS
};
/* Per star (sv_star). */
enum {
    SV_X0, SV_Y0, SV_X, SV_Y, SV_COS_REST, SV_COS_SHIP, SV_DOPPLER, SV_TEMP_REST, SV_TEMP_SHIP,
    SV_STAR_FLUX_REST, SV_STAR_FLUX_SHIP, SV_R0, SV_G0, SV_B0, SV_R, SV_G, SV_B,
    SV_STAR_FIELDS
};
/* Curves (sv_curve), on SV_CURVE_POINTS angles 0, 1, ..., 180 degrees. */
enum {
    SV_C_THETA_SHIP,   /* theta' [deg] of a star at Earth-frame angle i deg */
    SV_C_DOPPLER,      /* D of a star seen at ship angle i deg */
    SV_C_DENSITY,      /* stars per solid angle on board over at rest, at ship angle i deg: D^2 */
    SV_C_BAND,         /* visible-band flux of a SV_SUN_T star at ship angle i deg, over at rest */
    SV_C_TEMP,         /* its colour temperature D T [K] */
    SV_CURVES
};
#define SV_STARS_MAX 600
#define SV_CURVE_POINTS 181
#define SV_SEED 0x5eed5ca1ab1eULL
#define SV_SUN 5800.0

/* Recompute the sky for beta in [0, 0.995]. Returns 0, or -1 (and changes nothing) for other values. */
int sv_reset(double beta);
double sv_value(int field);
double sv_star(int i, int field);
double sv_curve(int i, int which);

#endif
