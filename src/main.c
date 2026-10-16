#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "relativity.h"
#include "render.h"
#include "svg.h"

#define DEST 4.2
#define TRAIL 600

/* Flip-and-burn trip to a star 4.2 ly away at 1 g, logged as CSV and SVG charts. */
static int headless(const char *dir)
{
    double a = ONE_G, half_tau = acosh(1.0 + a * DEST / 2.0) / a;
    enum { N = 400 };
    static float tau[2 * N + 1], v[2 * N + 1], gam[2 * N + 1], t[2 * N + 1], x[2 * N + 1];
    Rocket r = {0, 0, 0, 0};
    char path[512];
    snprintf(path, sizeof path, "%s/run.csv", dir);
    FILE *csv = fopen(path, "w");
    if (!csv) return 1;
    fprintf(csv, "tau,t,x,v,gamma\n");
    for (int k = 0; k <= 2 * N; k++) {
        tau[k] = (float)r.tau; t[k] = (float)r.t; x[k] = (float)r.x;
        v[k] = (float)velocity(&r); gam[k] = (float)lorentz_gamma(&r);
        fprintf(csv, "%.4f,%.4f,%.4f,%.5f,%.4f\n", r.tau, r.t, r.x, v[k], gam[k]);
        if (k < 2 * N) rocket_step(&r, k < N ? a : -a, half_tau / N);
    }
    fclose(csv);
    int n = 2 * N + 1;
    snprintf(path, sizeof path, "%s/velocity.svg", dir);
    Series sv[] = {{"v / c", v, "#1f77b4"}};
    if (svg_plot(path, "Velocity during a 1 g flip-and-burn trip to 4.2 ly", "proper time [yr]", "v / c", tau, n, sv, 1)) return 1;
    snprintf(path, sizeof path, "%s/gamma.svg", dir);
    Series sg[] = {{"gamma", gam, "#d62728"}};
    if (svg_plot(path, "Lorentz factor", "proper time [yr]", "gamma", tau, n, sg, 1)) return 1;
    snprintf(path, sizeof path, "%s/clocks.svg", dir);
    Series sc[] = {{"coordinate time t [yr]", t, "#2ca02c"}, {"proper time tau [yr]", tau, "#9467bd"}};
    if (svg_plot(path, "Two clocks: Earth frame vs traveller", "proper time [yr]", "years", tau, n, sc, 2)) return 1;
    snprintf(path, sizeof path, "%s/worldline.svg", dir);
    Series sw[] = {{"t [yr]", t, "#ff7f0e"}};
    if (svg_plot(path, "Worldline (t vs x)", "x [ly]", "t [yr]", x, n, sw, 1)) return 1;
    printf("arrived at %.3f ly: %.2f yr on board, %.2f yr on Earth\n", r.x, r.tau, r.t);
    return 0;
}

static int snapshot(void)
{
    Rocket r = {0, 0, 0, 0};
    Rocket trail[TRAIL];
    int n = 0;
    for (int k = 0; k < 300; k++) {
        rocket_step(&r, ONE_G, 0.01);
        if (k % 10 == 0 && n < TRAIL) trail[n++] = r;
    }
    static char buf[RENDER_COLS * RENDER_ROWS * 4];
    if (!render_frame(&r, trail, n, DEST, buf, sizeof buf)) return 1;
    fputs(buf, stdout);
    return 0;
}

static struct termios saved;
static void restore(void) { tcsetattr(STDIN_FILENO, TCSANOW, &saved); fputs("\033[?25h", stdout); }

static int play(void)
{
    if (!isatty(STDIN_FILENO)) { fputs("interactive mode needs a terminal; try --snapshot\n", stderr); return 1; }
    tcgetattr(STDIN_FILENO, &saved);
    struct termios raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    atexit(restore);
    fputs("\033[2J\033[?25l", stdout);

    Rocket r = {0, 0, 0, 0};
    static Rocket trail[TRAIL];
    int tlen = 0, head = 0, paused = 0, frame = 0;
    static char buf[RENDER_COLS * RENDER_ROWS * 4];
    struct timespec tick = {0, 40 * 1000000L};
    for (;;) {
        char c;
        double a = 0.0;
        while (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 'q') return 0;
            if (c == 'w') a = ONE_G;
            if (c == 's') a = -ONE_G;
            if (c == 'p') paused = !paused;
        }
        if (!paused) {
            rocket_step(&r, a, 0.04);
            if (frame++ % 3 == 0) {
                trail[head] = r;
                head = (head + 1) % TRAIL;
                if (tlen < TRAIL) tlen++;
            }
        }
        render_frame(&r, trail, tlen, DEST, buf, sizeof buf);
        printf("\033[H%s", buf);
        printf("x %.2f ly  v %.4f c  gamma %.2f  on board %.2f yr  Earth %.2f yr   \n",
               r.x, velocity(&r), lorentz_gamma(&r), r.tau, r.t);
        puts("w accelerate  s decelerate (1 g)  p pause  q quit    stop at the # star (4.2 ly)");
        fflush(stdout);
        if (fabs(r.x - DEST) < 0.05 && fabs(velocity(&r)) < 0.05) {
            printf("\nArrived! %.2f yr on board, %.2f yr passed on Earth.\n", r.tau, r.t);
            return 0;
        }
        nanosleep(&tick, NULL);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--headless") == 0) return headless(argc > 2 ? argv[2] : ".");
    if (argc > 1 && strcmp(argv[1], "--snapshot") == 0) return snapshot();
    return play();
}
