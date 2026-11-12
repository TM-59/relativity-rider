#include "courier.h"

#include <math.h>
#include <string.h>

#define C_DT 0.001             /* ship years per substep */
#define C_SUBSTEPS 10
#define EARTH_X 0.0
#define GATE_X 0.8
#define DEPOT_X 2.0            /* the Depot on the way out, the Midway buoy on the way home */
#define STAR_X 4.2
#define TANK 3.6               /* rapidity in the tank at the pad */
#define RED_LINE 1.2           /* the gauge turns red below this (a crisis in the bench) */
#define RESCUE_FUEL 1.6        /* a drone from the nearest relay fills a dry tank where it drifts, to this */
#define DRONE_WAIT 0.35        /* ship years the rocket drifts, engine dead, until the drone docks */
#define CATCH_TIME 0.4         /* ship years a tug takes to catch a runaway rocket and bring it to rest */
#define CATCH_MARGIN 1.5       /* a caught rocket's tank is filled to what the hop back to the marker needs, x this */
#define TUG_WAIT 1.0           /* after the silence: ship years a dry rocket drifts until Kestrel's tug finds it */
#define CLEAR_X 0.05           /* the arrival rule: within 0.05 ly of the marker */
#define CLEAR_V 0.05           /* and slower than 0.05 c */
#define LEG_LIMIT 9.0          /* ship years without reaching the marker: caught */
#define OVERSHOOT 0.35         /* ly past the marker (or behind the leg's start), still flying away: caught */
#define BASE_WOBBLE 0.6       /* relative wobble of the brake-distance readout with no echo */
#define POST_FUEL 4.6          /* Kestrel Post fills every rocket that lands, this much */

#define PAD_TANK 4.4           /* CE_PAD: Oren adds what the dish's generator can spare */
#define ECHO_WOBBLE 0.10       /* CE_PAD: his echo steadies the readout (wobble x0.10) */
#define CUE_LEAD 0.02          /* CE_PAD: his "turn now" comes this many ship years of travel early */
#define CALIB_DUST 0.25        /* CE_CALIB: calibrated, the dust shakes the readout less (x this), and his call gets through */
#define CACHE_FUEL 4.0         /* CE_CACHE: what the Depot's cache holds */
#define DUST_WOBBLE 0.1       /* CE_DUST: extra relative wobble in leg 3; uncalibrated, Oren's call does not get through */
#define DUST_LEAK 0.6         /* CE_DUST: rapidity leaking per ship year in leg 3 */
#define TANKER_FUEL 1.8        /* CE_TANKER: the tanker's load */
#define TANKER_HOLD 2.5        /* CE_TANKER: ship years held at rest on the marker while it pumps */
#define TANKER_SOFT 0.01       /* CE_TANKER: the clamp only holds a rocket slower than this */

/* Markers: Earth -> the Gate -> the Depot -> Kestrel Post -> the Midway buoy -> Earth. */
static const double TARGET[C_LEGS] = {GATE_X, DEPOT_X, STAR_X, DEPOT_X, EARTH_X};

/* The dust, the echo from Earth at Kestrel and the silence get the screen to themselves: no banner. */
static const int HEAVY[CS_COUNT] = {0, 0, 0, 1, 1, 1, 0};

int courier_options(int scene) { return scene == CS_GATE || scene == CS_STAR ? 2 : 1; }
int courier_heavy(int scene) { return scene >= 0 && scene < CS_COUNT && HEAVY[scene]; }

int courier_ending(const Courier *c)
{
    return c->scene == CS_HOME || c->leg >= C_LEGS ? c->turned : -1;
}

static int on(const Courier *c, unsigned effect) { return (c->effects & effect) != 0; }

/* 1 once the readout has been calibrated against Oren's echo at the Gate (CE_CALIB). */
static int calibrated(const Courier *c) { return c->gate == 0 && on(c, CE_CALIB); }

double courier_calibration_hold(void)
{
    /* The calibration needs a pulse from the Gate to Earth and back: the light time of 2 x 0.6 ly (c = 1). */
    return 2.0 * (GATE_X - EARTH_X);
}

double courier_brake_distance(const Courier *c)
{
    return (cosh(c->rocket.phi) - 1.0) / ONE_G;
}

double courier_wobble(const Courier *c)
{
    if (c->leg >= C_LEGS) return 0.0;
    double amp = BASE_WOBBLE;
    if (c->echo && on(c, CE_PAD)) amp *= ECHO_WOBBLE;
    if (c->leg == 2 && on(c, CE_DUST)) amp += DUST_WOBBLE * (calibrated(c) ? CALIB_DUST : 1.0);
    /* The same smooth wobble for every pilot who reads the gauge: it depends only on the ship clock. */
    double s = 8.0 * c->rocket.tau;
    return amp * (0.6 * sin(2.3 * s + 0.4) + 0.4 * sin(5.9 * s + 1.7));
}

double courier_leak(const Courier *c)
{
    return c->leg == 2 && on(c, CE_DUST) ? DUST_LEAK : 0.0;
}

double courier_hold_need(const Courier *c)
{
    if (c->held < 0.0) return 0.0;  /* this leg's hold is done */
    if (c->leg == 1 && calibrated(c)) return courier_calibration_hold();
    if (c->leg == 3 && c->turned == 0 && on(c, CE_TANKER)) return TANKER_HOLD;
    return 0.0;
}

static double range(const Courier *c) { return c->target_x - c->rocket.x; }

int courier_call_on(const Courier *c)
{
    if (!c->echo || !on(c, CE_PAD) || c->leg >= C_LEGS) return 0;
    /* In the dust lane the ranging pulses scatter unless the readout was calibrated at the Gate (CE_DUST). */
    return !(c->leg == 2 && on(c, CE_DUST) && !calibrated(c));
}

int courier_cue(const Courier *c)
{
    /* Oren's echo, from the true state (not the wobbling gauge): "turn now" once the distance needed to stop
     * has reached the marker while the rocket still burns toward it. Nothing once the dish is off. */
    if (!courier_call_on(c) || c->scene != CS_NONE) return 0;
    double d = range(c), v = velocity(&c->rocket);
    /* He calls it two frames early (CUE_LEAD of ship time at the current speed), the time a hand takes to let go. */
    double lead = CUE_LEAD * fabs(sinh(c->rocket.phi));
    if (c->command == CC_BURN && d * v > 0.0 && courier_brake_distance(c) >= fabs(d) - lead) return 1;
    return 0;
}

static void open_scene(Courier *c, int scene)
{
    c->scene = scene;
    if (c->order_len < CS_COUNT) c->order[c->order_len++] = scene;
    if (courier_heavy(scene)) { c->banner = CB_NONE; c->banner_left = 0.0; }
}

static void raise_banner(Courier *c, int banner)
{
    c->banner = banner;
    c->banner_left = 0.15;
    if (banner == CB_CLEAR) c->loud++;
}

static void start_leg(Courier *c)
{
    c->from_x = c->rocket.x;
    c->target_x = TARGET[c->leg];
    /* A hold first: on the Gate's marker for the calibration, or on Kestrel's marker for the tanker. */
    if (courier_hold_need(c) > 0.0) c->target_x = c->leg == 1 ? GATE_X : STAR_X;
    c->leg_time = 0.0;
    c->missed = 0;
}

void courier_init(Courier *c, unsigned effects)
{
    memset(c, 0, sizeof *c);
    c->rocket = (Rocket){0.0, EARTH_X, 0.0, 0.0};
    c->effects = effects;
    c->gate = c->turned = -1;
    c->fuel = TANK;
    c->scene = CS_NONE;
    c->from_x = c->target_x = EARTH_X;
    open_scene(c, CS_PAD);
}

static void clear_leg(Courier *c)
{
    static const int next[C_LEGS] = {CS_GATE, CS_DEPOT, CS_STAR, CS_SILENCE, CS_HOME};
    int scene = next[c->leg];
    c->leg++;
    c->held = 0.0;
    /* The heavy scenes and the ending get the screen to themselves: no "marker reached" banner. */
    if (!courier_heavy(scene) && scene != CS_HOME) raise_banner(c, CB_CLEAR);
    open_scene(c, scene);
}

static void fail(Courier *c, int banner)
{
    c->failures++;
    c->fail_leg[c->leg]++;
    c->crisis_leg[c->leg] = 1;
    raise_banner(c, banner);
}

/* What a clean flip-and-burn from rest over the distance d needs, in rapidity. */
static double hop_need(double d)
{
    return 2.0 * acosh(1.0 + ONE_G * fabs(d) / 2.0);
}

/* A tug brings the rocket to rest where it is (the braking is the tug's), then waits beside it while it refuels:
 * CATCH_TIME of ship time at rest, so both clocks run on together. The tank is filled to what the hop back to the
 * marker needs, with a margin. */
static void catch_rocket(Courier *c)
{
    c->rocket.phi = 0.0;
    rocket_step(&c->rocket, 0.0, CATCH_TIME);
    double need = hop_need(range(c)) * CATCH_MARGIN;
    if (c->fuel < need) c->fuel = need;
    c->leg_time = 0.0;
    c->drift = 0.0;
    c->refuel = 0;
}

/* A rescue, always a failure. Flying 0.28 ly past the marker (or behind the leg's start) or taking too long: a tug
 * catches the rocket. A dry tank while Oren's dish ranges the rocket: a relay's drone reaches it where it drifts after
 * DRONE_WAIT and fills the tank to RESCUE_FUEL. A dry tank after the silence: nobody knows where the rocket is until
 * Kestrel's tug finds it after TUG_WAIT of drifting, and catches it (CE_SILENCE). */
static void rescue(Courier *c, int kind)
{
    if (kind == 0) catch_rocket(c);
    else {
        c->drift = kind == 1 ? DRONE_WAIT : TUG_WAIT;
        c->refuel = kind;
    }
    fail(c, CB_RESCUE);
}

void courier_tick(Courier *c, int frames, int command)
{
    if (frames < 0 || c->scene != CS_NONE || c->leg >= C_LEGS) return;
    if (frames > 250) frames = 250;
    if (command < CC_COAST || command > CC_BRAKE) command = CC_COAST;
    for (int f = 0; f < frames && c->scene == CS_NONE && c->leg < C_LEGS; f++) {
        int leg = c->leg;
        c->command = command;
        for (int k = 0; k < C_SUBSTEPS; k++) {
            double a = 0.0, g = ONE_G;
            if (c->drift > 0.0 && (c->drift -= C_DT) <= 1e-12) {
                c->drift = 0.0;
                if (c->refuel == 2) catch_rocket(c);
                else if (c->refuel == 1 && c->fuel < RESCUE_FUEL) c->fuel = RESCUE_FUEL;
                c->refuel = 0;
            }
            if (c->fuel > 0.0 && c->drift <= 0.0 && c->held <= 0.0) {
                if (command == CC_BURN) a = range(c) >= 0.0 ? g : -g;
                if (command == CC_BRAKE && c->rocket.phi != 0.0) {
                    /* Burn against the motion, but no further than rest. */
                    double share = fmin(1.0, fabs(c->rocket.phi) / (g * C_DT));
                    a = (c->rocket.phi > 0.0 ? -g : g) * share;
                }
                double most = c->fuel / C_DT;
                if (fabs(a) > most) a = a > 0.0 ? most : -most;
            }
            rocket_step(&c->rocket, a, C_DT);
            if (command == CC_BRAKE && fabs(c->rocket.phi) < 1e-12) c->rocket.phi = 0.0;
            c->fuel = fmax(0.0, c->fuel - fabs(a) * C_DT - courier_leak(c) * C_DT);
            c->leg_time += C_DT;
            if (c->fuel < RED_LINE) { c->red += C_DT; c->crisis_leg[leg] = 1; }
        }
        c->frames++;
        if (c->banner_left > 0.0 && (c->banner_left -= C_DT * C_SUBSTEPS) <= 1e-12) c->banner = CB_NONE;
        double x = c->rocket.x, v = velocity(&c->rocket), frame = C_DT * C_SUBSTEPS;
        int on_mark = fabs(x - c->target_x) < CLEAR_X && fabs(v) < CLEAR_V;
        double need = courier_hold_need(c);
        if (c->held > 0.0) {
            /* Clamped on the marker: the leg goes on when the hold is over. Both clocks keep running. */
            if ((c->held += frame) >= need - 1e-9) {
                c->held = -1.0;
                start_leg(c);
            }
            continue;
        } else if (on_mark && need <= 0.0) {
            clear_leg(c);
            continue;
        } else if (on_mark && command == CC_COAST && c->drift <= 0.0) {
            /* The tanker reaches only for a rocket at rest on the marker. Too fast and the clamp glances off:
             * a failure, and a drone brings the tanker round again while you wait (the second pass always holds).
             * The Gate's calibration clamp takes any rocket on the marker. */
            if (c->leg == 3 && fabs(v) > TANKER_SOFT && !c->missed) {
                c->missed = 1;
                c->drift = DRONE_WAIT;
                fail(c, CB_GLANCE);
            } else {
                if (c->leg == 3) c->fuel += TANKER_FUEL;
                c->rocket.phi = 0.0;
                c->held = frame;
            }
            continue;
        }
        if (c->refuel == 2) continue;  /* drifting until Kestrel's tug finds the rocket: one failure, already counted */
        double dir = c->target_x >= c->from_x ? 1.0 : -1.0;
        double past = (x - c->target_x) * dir, behind = (c->from_x - x) * dir;
        int away = range(c) * v < 0.0;  /* still flying away from the marker */
        if (((past > OVERSHOOT || behind > OVERSHOOT) && away) || c->leg_time > LEG_LIMIT) rescue(c, 0);
        else if (c->fuel <= 0.0 && c->drift <= 0.0) rescue(c, c->echo ? 1 : 2);
    }
}

int courier_answer(Courier *c, int option)
{
    int scene = c->scene;
    if (scene == CS_NONE || option < 0 || option >= courier_options(scene)) return 1;
    c->scene = CS_NONE;
    switch (scene) {
    case CS_PAD:
        c->echo = 1;
        if (on(c, CE_PAD)) c->fuel = PAD_TANK;
        start_leg(c);
        break;
    case CS_GATE:
        c->gate = option;
        start_leg(c);
        break;
    case CS_DEPOT:
        /* The Depot's cache goes into the tank. */
        if (on(c, CE_CACHE)) c->fuel += CACHE_FUEL;
        open_scene(c, CS_DUST);
        break;
    case CS_DUST:
        start_leg(c);
        break;
    case CS_STAR:
        c->turned = option;
        c->fuel += POST_FUEL;
        start_leg(c);
        break;
    case CS_SILENCE:
        if (on(c, CE_SILENCE)) c->echo = 0;
        start_leg(c);
        break;
    case CS_HOME:
        break;
    default:
        return 1;
    }
    return 0;
}

double courier_value(const Courier *c, int field)
{
    const Rocket *r = &c->rocket;
    double brake = courier_brake_distance(c);
    switch (field) {
    case CV_X: return r->x;
    case CV_T: return r->t;
    case CV_TAU: return r->tau;
    case CV_V: return velocity(r);
    case CV_GAMMA: return lorentz_gamma(r);
    case CV_PHI: return r->phi;
    case CV_FUEL: return c->fuel;
    case CV_RED: return c->red;
    case CV_LEG: return c->leg;
    case CV_SCENE: return c->scene;
    case CV_BANNER: return c->banner;
    case CV_FAILURES: return c->failures;
    case CV_FROM: return c->from_x;
    case CV_TARGET: return c->target_x;
    case CV_RANGE: return range(c);
    case CV_BRAKE: return brake;
    case CV_BRAKE_READ: return brake * (1.0 + courier_wobble(c));
    case CV_WOBBLE: return courier_wobble(c);
    case CV_CUE: return courier_cue(c);
    case CV_ECHO: return c->echo && on(c, CE_PAD);
    case CV_CALL: return courier_call_on(c);
    case CV_GATE: return c->gate;
    case CV_CALIBRATED: return calibrated(c);
    case CV_TURNED: return c->turned;
    case CV_ENDING: return courier_ending(c);
    case CV_LOUD: return c->loud;
    case CV_HEAVY: return courier_heavy(c->scene);
    case CV_COMMAND: return c->command;
    case CV_RED_LINE: return RED_LINE;
    case CV_HELD: return c->held;
    case CV_HOLD_NEED: return courier_hold_need(c);
    case CV_LEAK: return courier_leak(c);
    case CV_DRIFT: return c->drift;
    case CV_GAP: return r->t - r->tau;
    default: return 0.0;
    }
}
