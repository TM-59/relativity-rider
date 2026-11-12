#ifndef COURIER_POLICY_H
#define COURIER_POLICY_H

/* Shared by the story test and the story bench: seeds and a human-like pilot. */
#include "../src/courier.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static inline uint64_t mix64(uint64_t z)
{
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/* Seed and index are stirred separately before they are combined (no XOR or sums of raw keys). */
static inline uint64_t sub_seed(uint64_t seed, uint64_t index, uint64_t salt)
{
    return mix64(mix64(seed + salt * 0x632be59bd9b4e019ULL) + (index + 1u) * 0x9e3779b97f4a7c15ULL);
}

typedef struct { uint64_t s; } Rng;
static inline double rng_unit(Rng *r)
{
    r->s += 0x9e3779b97f4a7c15ULL;
    return (double)(mix64(r->s) >> 11) / 9007199254740992.0;
}

/* A pilot who reads the panel, not the code: burns toward the marker until the brake-distance readout reaches
 * the distance left (a little early or late) - or, while Oren's echo is on, until he calls "turn now" - keeps the
 * thumb on a moment too long, coasts a few frames before reacting, then brakes to rest; from there it makes careful
 * short hops (no hesitation, a closer turn) until it is on the marker.
 * On Kestrel's marker while waiting for the tanker it brakes until the speed reads small, then cuts the engine. */
enum { P_BURN, P_COAST, P_BRAKE };

typedef struct {
    Rng rng;
    uint64_t key;
    int leg, fails, phase, late, wait;
    double target, bias, tol;
    int hesitate, overrun;
} Pilot;

static inline void pilot_hand(Pilot *p)
{
    p->bias = -0.06 + 0.20 * rng_unit(&p->rng);    /* turns this far past (or short of) the distance left */
    p->tol = 0.002 + 0.012 * rng_unit(&p->rng);    /* at the tanker: cuts the engine when the speed reads below this */
    p->hesitate = (int)(3.0 * rng_unit(&p->rng));  /* frames of coasting before the brake goes on */
    p->overrun = (int)(2.0 * rng_unit(&p->rng));   /* frames the thumb stays on the burn after the turn */
    p->phase = P_BURN;
    p->late = p->wait = 0;
}

static inline void pilot_init(Pilot *p, uint64_t key)
{
    memset(p, 0, sizeof *p);
    p->key = key;
    p->leg = -1;
    p->rng.s = key;
}

static inline int pilot_command(Pilot *p, const Courier *c)
{
    /* Each leg gets its own stirred stream, so a choice changes the legs it touches and not, by chance, every later one. */
    if (c->leg != p->leg || c->failures != p->fails || c->target_x != p->target) {
        if (c->leg != p->leg) p->rng.s = sub_seed(p->key, (uint64_t)c->leg, 5);
        p->leg = c->leg;
        p->fails = c->failures;
        p->target = c->target_x;
        pilot_hand(p);
    }
    double d = courier_value(c, CV_RANGE), v = courier_value(c, CV_V);
    double speed_read = fabs(v) * (1.0 + courier_wobble(c));
    if (courier_hold_need(c) > 0.0 && fabs(d) < 0.049) {
        p->phase = P_BURN;
        p->late = 0;
        return speed_read > p->tol ? CC_BRAKE : CC_COAST;
    }
    if (p->phase == P_BURN) {
        if (d * v < 0.0 && fabs(v) > 0.002) { p->phase = P_BRAKE; return CC_BRAKE; }
        int done = courier_call_on(c) ? courier_cue(c) == 1
                                             : d * v > 0.0 && courier_value(c, CV_BRAKE_READ) >= fabs(d) * (1.0 + p->bias);
        if (done && ++p->late > p->overrun) { p->phase = P_COAST; p->wait = 0; return CC_COAST; }
        if (!done) p->late = 0;
        return CC_BURN;
    }
    if (p->phase == P_COAST) {
        if (++p->wait > p->hesitate) { p->phase = P_BRAKE; return CC_BRAKE; }
        return CC_COAST;
    }
    if (fabs(v) < 1e-9) {
        /* At rest short of (or past) the marker: a short, careful hop from here (no hesitation, a closer turn). */
        p->bias = -0.03 + 0.08 * rng_unit(&p->rng);
        p->hesitate = p->overrun = 0;
        p->phase = P_BURN;
        p->late = 0;
        return CC_COAST;
    }
    return CC_BRAKE;
}

#endif
