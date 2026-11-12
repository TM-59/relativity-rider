/* "The Long Echo" story layer: the physics stays the Rocket model, scene order, the numbers each story beat changes,
 * quiet heavy scenes, and both choices. */
#include "courier_run.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int DESIGN[CS_COUNT] = {CS_PAD, CS_GATE, CS_DEPOT, CS_DUST, CS_STAR, CS_SILENCE, CS_HOME};

static int close_to(double a, double b) { return fabs(a - b) <= 1e-9 * fmax(1.0, fabs(b)); }

/* Fly with the pilot, answering scenes with the given choices, until `target` opens. Returns frames flown. */
static int drive(Courier *c, Pilot *p, int target, int gate, int turned)
{
    int frames = 0;
    while (c->scene != target) {
        assert(c->leg < C_LEGS && frames < RUN_GUARD_FRAMES);
        if (c->scene != CS_NONE) {
            assert(courier_answer(c, c->scene == CS_GATE ? gate : c->scene == CS_STAR ? turned : 0) == 0);
            continue;
        }
        double tau = c->rocket.tau, t = c->rocket.t;
        int fails = c->failures;
        courier_tick(c, 1, pilot_command(p, c));
        /* Every frame is 0.01 yr of ship time (a tug's catch adds its 0.4 yr at rest), and Earth time never runs slower. */
        double dtau = c->rocket.tau - tau;
        assert(fabs(dtau - 0.01) < 1e-9 || (c->failures >= fails && fabs(dtau - 0.41) < 1e-9));
        assert(c->rocket.t - t >= dtau - 1e-12 && fabs(velocity(&c->rocket)) < 1.0);
        frames++;
    }
    return frames;
}

static void start(Courier *c, Pilot *p, unsigned effects)
{
    courier_init(c, effects);
    pilot_init(p, play_key(0));
    assert(c->scene == CS_PAD && courier_options(CS_PAD) == 1);
}

static char *slurp(const char *path)
{
    static char text[2][1 << 17];
    static int slot;
    FILE *f = fopen(path, "rb");
    assert(f);
    char *buf = text[slot++ & 1];
    size_t len = fread(buf, 1, sizeof text[0] - 1, f);
    fclose(f);
    buf[len] = 0;
    return buf;
}

/* Two-sided sign test over the seeds: p for the larger of the two one-way counts. */
static double sign_p(int k, int n)
{
    double tail = 0.0;
    for (int x = k; x <= n; x++) {
        double comb = 1.0;
        for (int y = 0; y < x; y++) comb = comb * (n - y) / (y + 1);
        tail += comb;
    }
    return fmin(1.0, 2.0 * tail / pow(2.0, n));
}

enum { METRICS = 4 };
static const char *const METRIC[METRICS] = {"years", "ship", "red", "failures"};

static void read_rows(const char **at, double v[ABLATION_SEEDS][METRICS])
{
    for (int i = 0; i < ABLATION_SEEDS; i++) {
        *at = strstr(*at, "\"years\": ");
        assert(*at && sscanf(*at, "\"years\": %lf, \"ship\": %lf, \"red\": %lf, \"failures\": %lf",
                             &v[i][0], &v[i][1], &v[i][2], &v[i][3]) == 4);
        (*at)++;
    }
}

/* Outcomes where `a` is lower (better) than `b` in most seeds: sign test p <= 0.05 and a relative difference >= 5 %. */
static int better(double a[ABLATION_SEEDS][METRICS], double b[ABLATION_SEEDS][METRICS], int m)
{
    int lower = 0, higher = 0;
    double diff = 0.0, base = 0.0;
    for (int i = 0; i < ABLATION_SEEDS; i++) {
        lower += a[i][m] < b[i][m];
        higher += a[i][m] > b[i][m];
        diff += a[i][m] - b[i][m];
        base += b[i][m];
    }
    double rel = fabs(diff) / fmax(fabs(base), ABLATION_SEEDS);
    return lower > higher && sign_p(lower, ABLATION_SEEDS) <= 0.05 && rel >= 0.05;
}

/* Reads one choice's two paths from ablation.json; each path must win at least one outcome, and neither may win more
 * than one outcome beyond the other. */
static void check_choice(const char *text, const char *head)
{
    static double v[2][ABLATION_SEEDS][METRICS];
    const char *at = strstr(text, head);
    assert(at);
    for (int path = 0; path < 2; path++) {
        char key[16];
        snprintf(key, sizeof key, "\"%d\": [", path);
        at = strstr(at, key);
        assert(at);
        read_rows(&at, v[path]);
    }
    int wins[2] = {0, 0};
    char won[2][64] = {"", ""};
    for (int m = 0; m < METRICS; m++)
        for (int path = 0; path < 2; path++)
            if (better(v[path], v[1 - path], m)) { wins[path]++; strcat(won[path], " "); strcat(won[path], METRIC[m]); }
    assert(wins[0] >= 1 && wins[1] >= 1 && abs(wins[0] - wins[1]) <= 1);
    printf("[choice] %s path 0 wins%s; path 1 wins%s\n", head, won[0], won[1]);
}

/* [beat] one beat's numbers removed: at least `need` outcomes worse without it (a reward) or better (a punishment or a loss). */
static void check_beat(const char *text, const char *beat, int need, int reward)
{
    static double v[2][ABLATION_SEEDS][METRICS];
    char head[64];
    snprintf(head, sizeof head, "\"%s\": {", beat);
    const char *at = strstr(strstr(text, "\"beats\": {"), head);
    assert(at);
    for (int arm = 0; arm < 2; arm++) {
        at = strstr(at, arm ? "\"off\": [" : "\"on\": [");
        assert(at);
        read_rows(&at, v[arm]);
    }
    int hits = 0;
    char list[64] = "";
    for (int m = 0; m < METRICS; m++)
        if (reward ? better(v[0], v[1], m) : better(v[1], v[0], m)) { hits++; strcat(list, " "); strcat(list, METRIC[m]); }
    printf("[beat] %s: removing its numbers makes%s %s\n", beat, list, reward ? "worse" : "better");
    assert(hits >= need);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    /* [physics] the story never moves the rocket by itself: with every story number on or off, holding Burn from rest
     * is rocket_step() at 1 g for 0.001 yr steps, bit for bit, and matches the closed form of hyperbolic motion. */
    for (unsigned effects = 0; effects <= CE_ALL; effects += CE_ALL) {
        Courier c;
        courier_init(&c, effects);
        assert(courier_answer(&c, 0) == 0 && c.target_x > 0.0);
        Rocket r = {0.0, 0.0, 0.0, 0.0};
        courier_tick(&c, 30, CC_BURN);
        for (int k = 0; k < 300; k++) rocket_step(&r, ONE_G, 0.001);
        assert(c.rocket.x == r.x && c.rocket.t == r.t && c.rocket.tau == r.tau && c.rocket.phi == r.phi);
        assert(fabs(r.x - (cosh(ONE_G * r.tau) - 1.0) / ONE_G) < 1e-9 && fabs(r.t - sinh(ONE_G * r.tau) / ONE_G) < 1e-9);
        /* Coasting keeps the speed; Brake comes to rest exactly and stops there. */
        double v = velocity(&c.rocket);
        courier_tick(&c, 5, CC_COAST);
        assert(close_to(velocity(&c.rocket), v));
        courier_tick(&c, 60, CC_BRAKE);
        assert(c.rocket.phi == 0.0 && c.failures == 0);
    }
    puts("[physics] Burn = rocket_step at 1 g, bit for bit with and without the story; Coast keeps v; Brake stops at rest");

    /* [order] the bench's play-order.json (re-checked against the code by story_bench --check): every seed meets the
     * scenes in the designed order (setups before payoffs, the bottom before the end) and both endings are reached. */
    {
        const char *text = slurp("docs/narrative/play-order.json");
        char want[256] = "\"order\": [";
        for (int k = 0; k < CS_COUNT; k++) {
            strcat(want, k ? ", \"" : "\"");
            strcat(want, SCENE_ID[DESIGN[k]]);
            strcat(want, "\"");
        }
        strcat(want, "], \"won\": true");
        int seeds = 0, orders = 0;
        for (const char *c = text; (c = strstr(c, "\"seed\": ")); c++) seeds++;
        for (const char *c = text; (c = strstr(c, want)); c++) orders++;
        assert(seeds >= 8 && orders == seeds);
        static const char all[] = "\"quiet\": [\"dust\", \"star\", \"silence\"]";
        assert(strstr(text, "\"ending\": \"end-waited\", \"quiet\": [\"dust\", \"star\", \"silence\"]"));
        assert(strstr(text, "\"ending\": \"end-turned\", \"quiet\": [\"dust\", \"star\", \"silence\"]"));
        for (const char *c = text; (c = strstr(c, "\"quiet\": [")); c++) assert(strncmp(c, all, sizeof all - 1) == 0);
        printf("[order] %d seeds: pad .. home in the designed order; both endings reached\n", seeds);
        RunResult r;
        run_trip(play_key(3), CE_ALL, 1, 0, &r);
        assert(r.completed && r.order_len == CS_COUNT && r.ending == 0 && r.quiet[CS_DUST] && r.quiet[CS_STAR] && r.quiet[CS_SILENCE]);
        for (int k = 0; k < CS_COUNT; k++) assert(r.order[k] == DESIGN[k]);
    }

    /* [mech] pad: Oren tops up the tank (3.6 -> 4.4); his echo steadies the readout (x0.10) and calls the turn. */
    {
        Courier a, b; Pilot p;
        start(&a, &p, CE_ALL); start(&b, &p, CE_ALL & ~CE_PAD);
        assert(close_to(a.fuel, 3.6) && close_to(b.fuel, 3.6));
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 0) == 0);
        assert(close_to(a.fuel, 4.4) && close_to(b.fuel, 3.6) && a.echo == 1 && b.echo == 1);
        courier_tick(&a, 7, CC_COAST); courier_tick(&b, 7, CC_COAST);
        assert(fabs(courier_wobble(&b)) > 1e-3 && close_to(courier_wobble(&a) / courier_wobble(&b), 0.10));
        assert(courier_call_on(&a) && !courier_call_on(&b) && close_to(a.target_x, 0.8));
        /* The call comes once the distance needed to stop reaches the marker (two frames of travel early). */
        while (!courier_cue(&a)) { courier_tick(&a, 1, CC_BURN); assert(a.rocket.x < 0.8); }
        double lead = 0.02 * sinh(a.rocket.phi);
        assert(courier_brake_distance(&a) >= 0.8 - a.rocket.x - lead && courier_brake_distance(&a) < 0.8 - a.rocket.x);
        puts("[mech] pad: tank 3.6 -> 4.4; Oren's echo: readout wobble x0.10 and a 'turn now' call");
    }

    /* [mech] gate: waiting for the calibration echo holds the rocket on the Gate for a light round trip, 2 x 0.8 yr of
     * both clocks; in the dust the calibrated readout wobbles x0.25 and Oren's call still gets through. Going on: no hold,
     * a wilder readout and no call in the dust. */
    {
        Courier a, b, c; Pilot p, q, s;
        start(&a, &p, CE_ALL); start(&b, &q, CE_ALL); start(&c, &s, CE_ALL & ~CE_CALIB);
        drive(&a, &p, CS_GATE, 0, 1); drive(&b, &q, CS_GATE, 1, 1); drive(&c, &s, CS_GATE, 0, 1);
        assert(courier_options(CS_GATE) == 2 && !courier_heavy(CS_GATE) && a.banner == CB_CLEAR);
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 1) == 0 && courier_answer(&c, 0) == 0);
        assert(close_to(courier_hold_need(&a), 1.6) && courier_hold_need(&b) == 0.0 && courier_hold_need(&c) == 0.0);
        assert(close_to(courier_calibration_hold(), 2.0 * 0.8));
        assert(close_to(a.target_x, 0.8) && close_to(b.target_x, 2.0) && close_to(c.target_x, 2.0));
        double t0 = a.rocket.t, tau0 = a.rocket.tau;
        while (a.target_x < 1.0) courier_tick(&a, 1, pilot_command(&p, &a));
        assert(a.rocket.tau - tau0 >= 1.6 - 1e-9 && a.rocket.t - t0 >= 1.6 - 1e-9 && a.held < 0.0);
        drive(&a, &p, CS_DUST, 0, 1); drive(&b, &q, CS_DUST, 1, 1);
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 0) == 0 && a.leg == 2 && b.leg == 2);
        a.rocket.tau = b.rocket.tau;
        assert(courier_call_on(&a) && !courier_call_on(&b));
        assert(close_to(courier_wobble(&a) / courier_wobble(&b), (0.06 + 0.1 * 0.25) / (0.06 + 0.1)));
        puts("[mech] gate: calibrate = a 1.6 yr hold on the Gate, then x0.25 dust wobble and the call in the dust; go = neither");
    }

    /* [mech] depot: the cache adds 4.0 to the tank, then the dust scene opens.
     * [quiet] dust: the depot keeps its "marker reached" banner, but the dust gets the screen to itself. */
    {
        Courier a, b; Pilot p, q;
        start(&a, &p, CE_ALL); start(&b, &q, CE_ALL & ~CE_CACHE);
        drive(&a, &p, CS_DEPOT, 1, 1); drive(&b, &q, CS_DEPOT, 1, 1);
        assert(a.banner == CB_CLEAR && a.banner_left > 0.0 && !courier_heavy(CS_DEPOT));
        double fa = a.fuel, fb = b.fuel;
        int loud = a.loud;
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 0) == 0);
        assert(close_to(a.fuel - fa, 4.0) && close_to(b.fuel, fb) && a.scene == CS_DUST && b.scene == CS_DUST);
        assert(courier_heavy(CS_DUST) && a.banner == CB_NONE && a.banner_left == 0.0);
        assert(courier_answer(&a, 0) == 0);
        courier_tick(&a, 1, pilot_command(&p, &a));
        assert(a.loud == loud && a.banner != CB_CLEAR);
        puts("[mech] depot: +4.0 fuel from the cache");
        puts("[quiet] dust: shown with no 'marker reached' banner, before or after");
    }

    /* [mech] dust: in leg 3 only, the tank leaks 0.6 per ship year, burning or not, and the readout wobbles more. */
    {
        Courier a, b; Pilot p, q;
        start(&a, &p, CE_ALL); start(&b, &q, CE_ALL & ~CE_DUST);
        drive(&a, &p, CS_DUST, 1, 1); drive(&b, &q, CS_DUST, 1, 1);
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 0) == 0);
        assert(a.leg == 2 && close_to(courier_leak(&a), 0.6) && courier_leak(&b) == 0.0);
        double f = a.fuel;
        courier_tick(&a, 1, CC_COAST);
        assert(close_to(f - a.fuel, 0.6 * 0.01));
        b.rocket.tau = a.rocket.tau;
        assert(fabs(courier_wobble(&a)) > fabs(courier_wobble(&b)) && courier_call_on(&b) && !courier_call_on(&a));
        puts("[mech] dust: leg 3 leaks 0.6 per ship year, a wilder readout, Oren's call scattered");
    }

    /* [mech] star, both paths. Waiting: the rocket holds on Kestrel's marker; at rest the tanker's clamp takes it, adds 1.8
     * and holds it 2.5 yr of both clocks. Come in faster than 0.01 c and the clamp glances off once: a failure and a 0.35 yr
     * drift. Turning for home: no hold. Kestrel fills every rocket with 4.6.
     * [quiet] star: no banner when the echo from Earth arrives, nor after. */
    for (int turned = 0; turned < 2; turned++) {
        Courier a; Pilot p;
        start(&a, &p, CE_ALL);
        drive(&a, &p, CS_STAR, 1, turned);
        assert(courier_options(CS_STAR) == 2 && courier_heavy(CS_STAR) && a.banner == CB_NONE);
        int loud = a.loud;
        double f = a.fuel;
        assert(courier_answer(&a, turned) == 0);
        assert(a.turned == turned && close_to(a.fuel - f, 4.6));
        assert(close_to(a.target_x, turned ? 2.0 : 4.2) && close_to(courier_hold_need(&a), turned ? 0.0 : 2.5));
        courier_tick(&a, 1, pilot_command(&p, &a));
        assert(a.loud == loud && a.banner != CB_CLEAR);
        if (turned == 0) {
            Courier g = a;
            g.rocket.x = 4.2; g.rocket.phi = atanh(0.02);
            int fails = g.failures;
            courier_tick(&g, 1, CC_COAST);
            assert(g.failures == fails + 1 && g.fail_leg[3] == 1 && g.missed && g.banner == CB_GLANCE && g.held == 0.0);
            assert(fabs(g.drift - 0.35) < 1e-9);
            Courier h = a;
            h.rocket.x = 4.2; h.rocket.phi = 0.0;
            f = h.fuel;
            double tau = h.rocket.tau;
            courier_tick(&h, 1, CC_COAST);
            assert(h.held > 0.0 && close_to(h.fuel - f, 1.8) && h.failures == a.failures);
            while (h.target_x > 3.0) courier_tick(&h, 1, CC_BURN);
            assert(h.rocket.tau - tau >= 2.5 - 1e-9 && h.rocket.x == 4.2);
            puts("[quiet] star: shown with no 'marker reached' banner");
        }
    }
    puts("[mech] star: wait = a 2.5 yr hold at rest, +1.8 from the tanker, a fast clamp is a failure; turn = no hold");

    /* [mech] silence: once the dish is off, a dry tank drifts 1.0 yr until Kestrel's tug finds and catches the rocket;
     * before, a relay's drone fills it to 1.6 after 0.35 yr adrift. [quiet] silence: no banner, before or after. */
    {
        Courier a, b; Pilot p, q;
        start(&a, &p, CE_ALL); start(&b, &q, CE_ALL & ~CE_SILENCE);
        drive(&a, &p, CS_SILENCE, 1, 1); drive(&b, &q, CS_SILENCE, 1, 1);
        assert(courier_heavy(CS_SILENCE) && a.banner == CB_NONE && b.banner == CB_NONE);
        int loud = a.loud;
        assert(courier_answer(&a, 0) == 0 && courier_answer(&b, 0) == 0);
        assert(a.echo == 0 && b.echo == 1 && a.leg == 4 && !courier_call_on(&a) && courier_call_on(&b));
        courier_tick(&a, 1, CC_COAST);
        assert(a.loud == loud && a.banner != CB_CLEAR);
        puts("[quiet] silence: shown with no 'marker reached' banner");
        a.fuel = b.fuel = 0.0005;
        courier_tick(&a, 1, CC_BURN); courier_tick(&b, 1, CC_BURN);
        assert(a.fail_leg[4] == 1 && a.refuel == 2 && fabs(a.drift - 1.0) < 0.011);
        assert(b.fail_leg[4] == 1 && b.refuel == 1 && fabs(b.drift - 0.35) < 0.011);
        while (b.drift > 0.0) courier_tick(&b, 1, CC_BURN);
        assert(b.fuel > 1.5 && b.fuel <= 1.6);
        double v = velocity(&a.rocket);
        while (a.drift > 0.0) courier_tick(&a, 1, CC_COAST);
        assert(v != 0.0 && a.rocket.phi == 0.0 && a.fuel > 0.0 && a.fail_leg[4] == 1);
        puts("[mech] silence: a dry tank now drifts a year before Kestrel's tug catches the rocket");
    }

    /* [beat] and [choice] in the bench's ablation.json: each beat's numbers show in the outcomes, and each choice trades. */
    {
        const char *text = slurp("docs/narrative/ablation.json");
        check_beat(text, "pad", 2, 1);
        check_beat(text, "depot", 2, 1);
        check_beat(text, "dust", 2, 0);
        check_beat(text, "silence", 2, 0);
        check_choice(text, "\"gate\": {\"paths\"");
        check_choice(text, "\"star\": {\"paths\"");
    }

    /* [tension] every run of the bench finishes the trip. */
    {
        const char *text = slurp("docs/narrative/tension.json");
        assert(!strstr(text, "\"completed\": false") && strstr(text, "\"completed\": true"));
        puts("[tension] every bench run reaches home");
    }

    /* An ordinary cleared leg is loud, so the quiet checks above are not vacuous; out-of-range answers are refused. */
    {
        Courier a; Pilot p;
        start(&a, &p, CE_ALL);
        drive(&a, &p, CS_GATE, 0, 0);
        assert(a.banner == CB_CLEAR && a.loud == 1);
        assert(courier_answer(&a, 2) == 1 && courier_answer(&a, -1) == 1);
        courier_tick(&a, -1, CC_BURN);
        assert(a.scene == CS_GATE);
    }
    puts("courier tests passed");
    return 0;
}
