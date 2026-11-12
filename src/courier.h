#ifndef COURIER_H
#define COURIER_H

#include "relativity.h"

/* "The Long Echo": a story layer over the unchanged rocket model in relativity.c.
 * Vela flies seed stock out to Kestrel Post at the star (4.2 ly) and home again, in five legs along the line:
 * Earth (0) - the Gate (0.6) - the Depot (2.0) - Kestrel Post (4.2) - the Midway buoy (2.0) - Earth. Each leg ends at rest
 * on a marker (within 0.05 ly, slower than 0.05 c: the arrival rule of the observatory). The player holds
 * Burn (toward the marker), Brake (against the motion, stopping exactly at rest) or neither (coast).
 * Every substep is rocket_step() with the engine's proper acceleration; the story never moves the rocket.
 * Scenes open at fixed progress points (the start and the end of a leg) and change game numbers only:
 * the tank, a calibration of the readout, how much the brake-distance readout wobbles, a leak, Oren's echo
 * (the "turn now" call) and what a rescue costs. */

enum { C_LEGS = 5 };

enum {
    CS_NONE = -1,
    CS_PAD,      /* Oren at the dish: what fuel he can add, Mum's watch, "half, turn, nothing", he will range her */
    CS_GATE,     /* Kestrel is on half rations; choice: wait at the Gate for Oren's calibration echo, or go on now */
    CS_DEPOT,    /* the Depot's cache; Oren's echo: Mum lit the window lamp (the false peak) */
    CS_DUST,     /* a dust lane the charts called clear: the readout jumps, the tank leaks (heavy) */
    CS_STAR,     /* Kestrel Post; the echo from Earth: Mum died; choice: wait for the tanker or turn for home now (heavy) */
    CS_SILENCE,  /* the dish is switched off; no more echo (heavy) */
    CS_HOME,     /* the pad on Earth; ending */
    CS_COUNT
};

/* Numeric story effects, one bit each, so the bench can remove them one by one. */
enum {
    CE_PAD = 1u << 0,      /* Oren tops up the tank, and his echo steadies the readout and calls the turn */
    CE_CALIB = 1u << 1,    /* waiting at the Gate: a light round trip to Earth (1.2 yr), then a steadier readout and the call in the dust */
    CE_CACHE = 1u << 2,    /* the Depot's fuel cache */
    CE_DUST = 1u << 3,     /* the dust lane: in leg 3 the readout wobbles more and the tank leaks */
    CE_TANKER = 1u << 4,   /* waiting at Kestrel: hold on the marker until the tanker has pumped */
    CE_SILENCE = 1u << 5,  /* the echo is gone for the last two legs; a dry tank now means a tow back to the leg's start */
    CE_ALL = (1u << 6) - 1u
};

enum { CC_COAST, CC_BURN, CC_BRAKE };
enum { CB_NONE, CB_CLEAR, CB_RESCUE, CB_GLANCE };

typedef struct {
    Rocket rocket;
    unsigned effects;
    int leg;              /* 0..4; C_LEGS when the trip is over */
    int scene;            /* CS_* open now, or CS_NONE */
    int gate;             /* -1 until chosen; 0 = wait for the calibration echo, 1 = go on now */
    int turned;           /* -1 until chosen; 0 = wait for the tanker, 1 = turn for home now */
    int echo;             /* 1 while Oren's dish ranges the rocket */
    int command;          /* CC_* applied in the last frame */
    double leg_time;      /* ship years in this leg */
    double held;          /* ship years held on the marker (the Gate's calibration, Kestrel's tanker); < 0 once done */
    double drift;         /* ship years left until a drone docks (engine dead meanwhile) */
    int refuel;           /* the drifting rocket is waiting for fuel */
    int missed;           /* the tanker's clamp glanced off once */
    double fuel, red;     /* tank in units of rapidity; ship years flown with the tank in the red */
    double from_x, target_x;
    int failures;
    int fail_leg[C_LEGS];
    int crisis_leg[C_LEGS];  /* 1 if the tank dropped into the red or something failed in the leg */
    int banner;           /* CB_* shown on screen */
    double banner_left;
    int loud;             /* number of "marker reached" banners raised */
    long frames;
    int order[CS_COUNT];
    int order_len;
} Courier;

enum CourierValue {
    CV_X, CV_T, CV_TAU, CV_V, CV_GAMMA, CV_PHI, CV_FUEL, CV_RED, CV_LEG, CV_SCENE, CV_BANNER, CV_FAILURES,
    CV_FROM, CV_TARGET, CV_RANGE, CV_BRAKE, CV_BRAKE_READ, CV_WOBBLE, CV_CUE, CV_ECHO, CV_GATE, CV_CALIBRATED, CV_CALL, CV_TURNED,
    CV_ENDING, CV_LOUD, CV_HEAVY, CV_COMMAND, CV_RED_LINE, CV_HELD, CV_HOLD_NEED, CV_LEAK, CV_DRIFT,
    CV_GAP, CV_FIELDS
};

void courier_init(Courier *c, unsigned effects);
/* frames of ten 0.001 yr steps of ship time under one command (CC_*). Does nothing while a scene is open or after the end. */
void courier_tick(Courier *c, int frames, int command);
/* Close the open scene. Choice scenes take option 0 or 1; others take 0. Returns 0 when accepted. */
int courier_answer(Courier *c, int option);
int courier_options(int scene);
/* 1 for scenes shown without a "marker reached" banner (the heavy scenes). */
int courier_heavy(int scene);
/* 0 = waited for the tanker, 1 = turned for home at once; -1 before the pad. */
int courier_ending(const Courier *c);
/* Relative wobble of the brake-distance readout right now (0 when the trip is over). */
double courier_wobble(const Courier *c);
/* 1 while Oren's call can reach the rocket (his dish on, and not scattered by uncalibrated dust). */
int courier_call_on(const Courier *c);
/* Ship years of the calibration hold at the Gate: a light round trip to Earth, 2 x 0.6 ly at c = 1. */
double courier_calibration_hold(void);
/* Oren's echo: 1 = "turn now" (the brake distance has reached the marker), 0 = nothing. */
int courier_cue(const Courier *c);
/* Fuel leaking per ship year right now (the dust lane). */
double courier_leak(const Courier *c);
/* Ship years the rocket must hold on Kestrel's marker before the leg goes on (the tanker); 0 otherwise. */
double courier_hold_need(const Courier *c);
/* Distance needed to come to rest from the current rapidity at the engine's acceleration, (cosh phi - 1) / a. */
double courier_brake_distance(const Courier *c);
double courier_value(const Courier *c, int field);

#endif
