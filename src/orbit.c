#include "orbit.h"
#include "schwarzschild.h"

#include <math.h>
#include <stddef.h>

#define PI 3.14159265358979323846
#define M_HOLE 1.0
#define VIEW_MAX 60.0

static Orbit gr, flat;
static int kind, steps, gstate, nstate, apses, trail_len, trail_head;
static double r0, ell, h, H0, drift_max, view, phi0, prec, deflect;
static double apsis[SO_APSES_MAX];
static float tx[SO_TRAIL_MAX], ty[SO_TRAIL_MAX], nx[SO_TRAIL_MAX], ny[SO_TRAIL_MAX];

static void record(void)
{
    int k = (trail_head + trail_len) % SO_TRAIL_MAX;
    tx[k] = (float)(gr.r * cos(gr.phi));
    ty[k] = (float)(gr.r * sin(gr.phi));
    nx[k] = (float)(flat.r * cos(flat.phi));
    ny[k] = (float)(flat.r * sin(flat.phi));
    if (trail_len < SO_TRAIL_MAX) trail_len++;
    else trail_head = (trail_head + 1) % SO_TRAIL_MAX;
}

static void add_apsis(double phi)
{
    if (apses > 0) prec = phi - apsis[(apses - 1) % SO_APSES_MAX] - 2.0 * PI;
    apsis[apses % SO_APSES_MAX] = phi;
    apses++;
}

int so_reset(int next_kind, double start, double L)
{
    if (!(L >= 0.0) || L > 20.0) return -1;
    kind = next_kind == 1 ? 1 : 0;
    ell = L;
    steps = gstate = nstate = apses = trail_len = trail_head = 0;
    drift_max = 0.0;
    prec = deflect = NAN;
    if (kind == 0) {
        if (!(start > 2.5 * M_HOLE) || start > 50.0) return -1;
        r0 = start;
        phi0 = 0.0;
        geo_init(&gr, GEO_MASSIVE, M_HOLE, L, r0, 0.0);
        geo_init(&flat, GEO_NEWTON, M_HOLE, L, r0, 0.0);
        double rp, ra, rmin = 2.0 * M_HOLE;
        int b = geo_apsides(&gr, &rp, &ra);
        if (b != GEO_PLUNGES) rmin = rp;
        h = 2.0 * PI * pow(fmax(rmin, 2.0 * M_HOLE), 1.5) / 600.0;
        view = fmin(VIEW_MAX, 1.15 * (b == GEO_BOUND ? fmax(ra, r0) : r0));
        if (geo_force(GEO_MASSIVE, M_HOLE, L, r0) > 0.0) add_apsis(0.0); /* started at the periapsis */
    } else {
        if (L > 0.95 * SO_LIGHT_START) return -1;
        r0 = SO_LIGHT_START;
        phi0 = PI + asin(L / r0);
        geo_init(&gr, GEO_LIGHT, M_HOLE, L, r0, 0.0);
        gr.pr = -sqrt(1.0 - (1.0 - 2.0 * M_HOLE / r0) * L * L / (r0 * r0));
        gr.E = 1.0;
        geo_init(&flat, GEO_LIGHT, 0.0, L, r0, 0.0);
        flat.pr = -sqrt(1.0 - L * L / (r0 * r0));
        flat.E = 1.0;
        gr.phi = flat.phi = phi0;
        h = 0.05;
        view = r0 * 1.07;
    }
    H0 = geo_energy(&gr);
    record();
    return 0;
}

static void advance(Orbit *o, int *state, double *returned_phi)
{
    if (*state != SO_MOVING) return;
    Orbit before = *o;
    geo_step(o, h);
    if (o->state == GEO_CAPTURED) { *state = SO_CAPTURED; return; }
    if (o == &gr && kind == 0 && before.pr < 0.0 && o->pr >= 0.0) {
        Orbit at = before;
        geo_step(&at, geo_apsis_step(&before, h));
        add_apsis(at.phi);
    }
    if (o->pr > 0.0 && before.r < r0 && o->r >= r0 && returned_phi) /* back out through the start radius */
        *returned_phi = before.phi + (o->phi - before.phi) * (r0 - before.r) / (o->r - before.r);
    if (o->pr > 0.0 && o->r > 1.6 * view) *state = SO_ESCAPED;
}

void so_step(int n)
{
    if (n > SO_CALL_MAX) n = SO_CALL_MAX;
    for (int k = 0; k < n; k++) {
        if (steps >= SO_MAX_STEPS) { if (gstate == SO_MOVING) gstate = SO_LIMIT; if (nstate == SO_MOVING) nstate = SO_LIMIT; return; }
        if (gstate != SO_MOVING && nstate != SO_MOVING) return;
        double back = NAN;
        advance(&gr, &gstate, kind == 1 ? &back : NULL);
        advance(&flat, &nstate, NULL);
        if (kind == 1 && !isnan(back) && isnan(deflect))
            deflect = (back - phi0) - (PI - 2.0 * asin(ell / r0));
        if (gstate == SO_MOVING) drift_max = fmax(drift_max, fabs(geo_energy(&gr) / H0 - 1.0));
        steps++;
        if (steps % SO_TRAIL_EVERY == 0) record();
    }
}

static double curve_r(int i)
{
    double lo = 2.05 * M_HOLE;
    return lo + (view - lo) * i / (SO_CURVE_POINTS - 1);
}

double so_curve(int i)
{
    if (i < 0 || i >= SO_CURVE_POINTS) return 0.0;
    return 2.0 * geo_potential(kind == 0 ? GEO_MASSIVE : GEO_LIGHT, M_HOLE, ell, curve_r(i));
}

double so_ncurve(int i)
{
    if (i < 0 || i >= SO_CURVE_POINTS) return 0.0;
    double r = curve_r(i);
    return kind == 0 ? 1.0 + 2.0 * geo_potential(GEO_NEWTON, M_HOLE, ell, r) : ell * ell / (r * r);
}

static double trail_at(const float *a, int i)
{
    if (i < 0 || i >= trail_len) return 0.0;
    return a[(trail_head + i) % SO_TRAIL_MAX];
}

double so_trail_x(int i) { return trail_at(tx, i); }
double so_trail_y(int i) { return trail_at(ty, i); }
double so_ntrail_x(int i) { return trail_at(nx, i); }
double so_ntrail_y(int i) { return trail_at(ny, i); }

double so_apsis(int i)
{
    int first = apses > SO_APSES_MAX ? apses - SO_APSES_MAX : 0;
    if (i < 0 || first + i >= apses) return 0.0;
    return apsis[(first + i) % SO_APSES_MAX];
}

double so_value(int field)
{
    double rp, ra, nrp, nra;
    int bound = geo_apsides(&gr, &rp, &ra), nbound = kind == 0 ? geo_apsides(&flat, &nrp, &nra) : GEO_UNBOUND;
    if (kind == 1) { nrp = ell; nra = -1.0; }
    switch (field) {
    case SO_KIND: return kind;
    case SO_M: return M_HOLE;
    case SO_R0: return r0;
    case SO_L: return ell;
    case SO_E: return gr.E;
    case SO_H: return h;
    case SO_STEPS: return steps;
    case SO_S: return gr.s;
    case SO_T: return gr.t;
    case SO_R: return gr.r;
    case SO_PHI: return gr.phi;
    case SO_X: return gr.r * cos(gr.phi);
    case SO_Y: return gr.r * sin(gr.phi);
    case SO_PR: return gr.pr;
    case SO_STATE: return gstate;
    case SO_BOUND: return bound;
    case SO_RP: return rp;
    case SO_RA: return ra;
    case SO_APSES: return apses;
    case SO_PREC: return isnan(prec) ? -99.0 : prec;
    case SO_PREC_EXACT: return kind == 0 && bound == GEO_BOUND ? geo_precession_exact(M_HOLE, rp, ra) : -99.0;
    case SO_PREC_WEAK: return kind == 0 && bound == GEO_BOUND ? geo_precession_weak(M_HOLE, 0.5 * (rp + ra), (ra - rp) / (ra + rp)) : -99.0;
    case SO_DRIFT: return drift_max;
    case SO_CLOCK_RATE: return kind == 0 && gstate != SO_CAPTURED ? 1.0 / geo_rate(&gr) : 0.0;
    case SO_STATIC_RATE: return gr.r > 2.0 * M_HOLE ? geo_static_rate(M_HOLE, gr.r) : 0.0;
    case SO_ISCO: return geo_isco(M_HOLE);
    case SO_PHOTON: return geo_photon_sphere(M_HOLE);
    case SO_BCRIT: return geo_critical_b(M_HOLE);
    case SO_CIRC_STABLE: return geo_circular_radius(M_HOLE, ell);
    case SO_CIRC_UNSTABLE: return geo_unstable_radius(M_HOLE, ell);
    case SO_NX: return flat.r * cos(flat.phi);
    case SO_NY: return flat.r * sin(flat.phi);
    case SO_NR: return flat.r;
    case SO_NSTATE: return nstate;
    case SO_NBOUND: return nbound;
    case SO_NRP: return nrp;
    case SO_NRA: return nra;
    case SO_TRAIL: return trail_len;
    case SO_VIEW: return view;
    case SO_CURVE_N: return SO_CURVE_POINTS;
    case SO_CURVE_R0: return curve_r(0);
    case SO_CURVE_R1: return curve_r(SO_CURVE_POINTS - 1);
    case SO_LEVEL: return gr.E * gr.E;
    case SO_NLEVEL: return kind == 0 ? 1.0 + 2.0 * flat.E : 1.0;
    case SO_DEFLECT: return isnan(deflect) ? -99.0 : deflect;
    case SO_DEFLECT_INNER: {
        if (kind != 1 || bound == GEO_PLUNGES) return -99.0;
        Orbit start;
        geo_init(&start, GEO_LIGHT, M_HOLE, ell, r0, 0.0);
        start.pr = -1.0;
        start.E = 1.0;
        return geo_swept_angle(&start, r0) - (PI - 2.0 * asin(ell / r0));
    }
    case SO_DEFLECT_TOTAL: {
        if (kind != 1 || bound == GEO_PLUNGES) return -99.0;
        Orbit start = gr;
        return geo_swept_angle(&start, INFINITY) - PI;
    }
    case SO_DEFLECT_WEAK: return kind == 1 ? geo_deflection_weak(M_HOLE, ell) : -99.0;
    default: return 0.0;
    }
}
