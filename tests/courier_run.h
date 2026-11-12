#ifndef COURIER_RUN_H
#define COURIER_RUN_H

/* One whole trip with the human-like pilot; shared by the story test and bench. */
#include "courier_policy.h"

#include <string.h>

enum { RUN_GUARD_FRAMES = 20000 };  /* 200 ship years: counted as "did not finish" */

typedef struct {
    double years, ship, red;
    int failures, completed, ending;
    int crisis[C_LEGS], failure[C_LEGS];
    int order[CS_COUNT], order_len;
    int quiet[CS_COUNT];  /* heavy scene shown with no "marker reached" banner, before or after it */
} RunResult;

static const char *const SCENE_ID[CS_COUNT] = {"pad", "gate", "depot", "dust", "star", "silence", "home"};

static inline void run_trip(uint64_t key, unsigned effects, int gate, int turned, RunResult *out)
{
    Courier c;
    Pilot p;
    memset(out, 0, sizeof *out);
    out->ending = -1;
    courier_init(&c, effects);
    pilot_init(&p, key);
    int frames = 0;
    while (c.leg < C_LEGS && frames < RUN_GUARD_FRAMES) {
        if (c.scene != CS_NONE) {
            int scene = c.scene, loud = c.loud, calm = c.banner == CB_NONE;
            int option = scene == CS_GATE ? gate : scene == CS_STAR ? turned : 0;
            courier_answer(&c, option);
            if (courier_heavy(scene)) {
                courier_tick(&c, 1, pilot_command(&p, &c));  /* the first frame after the scene closes */
                frames++;
                out->quiet[scene] = calm && c.loud == loud && c.banner != CB_CLEAR;
            }
            continue;
        }
        courier_tick(&c, 1, pilot_command(&p, &c));
        frames++;
    }
    out->completed = c.leg >= C_LEGS;
    out->years = c.rocket.t;
    out->ship = c.rocket.tau;
    out->red = c.red;
    out->failures = c.failures;
    out->ending = courier_ending(&c);
    for (int l = 0; l < C_LEGS; l++) { out->crisis[l] = c.crisis_leg[l]; out->failure[l] = c.fail_leg[l]; }
    out->order_len = c.order_len;
    memcpy(out->order, c.order, sizeof c.order);
}

/* Seeds: every row and run gets its own stirred key (no XOR or sums of raw keys). */
#define STORY_BASE 20261113ULL
enum { SALT_SEED = 1, SALT_PLAY = 2, SALT_ABLATION = 3, SALT_TENSION = 4 };
static inline uint64_t story_seed(int i) { return sub_seed(STORY_BASE, (uint64_t)i, SALT_SEED); }
static inline uint64_t play_key(int i) { return sub_seed(story_seed(i), 0, SALT_PLAY); }
static inline uint64_t ablation_key(int i) { return sub_seed(story_seed(i), 0, SALT_ABLATION); }
static inline uint64_t tension_key(int i, int run) { return sub_seed(story_seed(i), (uint64_t)run, SALT_TENSION); }

enum { PLAY_SEEDS = 12, ABLATION_SEEDS = 24, TENSION_SEEDS = 16, TENSION_RUNS = 12 };
/* Default choices for the ablation rows: going on from the Gate, and turning for home at once. */
enum { DEFAULT_GATE = 1, DEFAULT_TURNED = 1 };
static inline void play_choices(int i, int *gate, int *turned) { *gate = i % 2; *turned = (i / 2) % 2; }
static inline void tension_choices(int run, int *gate, int *turned) { *gate = run % 2; *turned = (run / 2) % 2; }

#endif
