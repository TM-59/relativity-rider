#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "gif.h"
#include "orbit.h"
#include "relativity.h"
#include "render.h"
#include "skyview.h"
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

static int gif_colour(char c)
{
    if (c == '#') return 2;
    if (c == '@') return 3;
    if (c == '.') return 4;
    if (c == '/' || c == '\\') return 5;
    return 1;
}

/* Scripted 1 g flip-and-burn trip to the star as an animated GIF. */
static int gif_demo(const char *path)
{
    double a = ONE_G, half_tau = acosh(1.0 + a * DEST / 2.0) / a;
    enum { FRAMES = 100 };
    unsigned char pal[GIF_COLOURS][3] = {{8, 10, 24}, {235, 235, 235}, {255, 205, 60}, {255, 120, 90},
                                         {95, 110, 150}, {110, 120, 175}};
    const int scale = 3, cw = RENDER_COLS * 6 * scale, ch = RENDER_ROWS * 9 * scale;
    static unsigned char px[RENDER_COLS * 6 * 3 * RENDER_ROWS * 9 * 3];
    static char buf[RENDER_COLS * RENDER_ROWS * 4];
    static Rocket trail[TRAIL];
    Rocket r = {0, 0, 0, 0};
    int tlen = 0;
    Gif *g = gif_open(path, cw, ch, pal, 6);
    if (!g) return 1;
    for (int k = 0; k <= FRAMES; k++) {
        if (tlen < TRAIL) trail[tlen++] = r;
        render_frame(&r, trail, tlen, DEST, buf, sizeof buf);
        gif_draw_text(px, cw, ch, buf, scale, gif_colour);
        if (gif_frame(g, px, k == FRAMES ? 250 : 0)) return 1;
        if (k < FRAMES) rocket_step(&r, k < FRAMES / 2 ? a : -a, 2.0 * half_tau / FRAMES);
    }
    if (gif_close(g)) return 1;
    printf("arrived at %.3f ly: %.2f yr on board, %.2f yr on Earth\n", r.x, r.tau, r.t);
    return 0;
}

/* A particle released at 20 M with L = 4.2 M around a black hole: the precessing orbit, Newton's ellipse from the
 * same start, the horizon, the photon sphere and the innermost stable circular orbit, as docs/orbit.svg. */
static int orbit_svg(const char *dir)
{
    enum { ORBITS = 5 };
    static float gx[SO_TRAIL_MAX], gy[SO_TRAIL_MAX], nx[SO_TRAIL_MAX], ny[SO_TRAIL_MAX];
    so_reset(0, 20.0, 4.2);
    while (so_value(SO_APSES) < ORBITS && so_value(SO_STATE) == SO_MOVING) so_step(1);
    int n = (int)so_value(SO_TRAIL);
    for (int i = 0; i < n; i++) { gx[i] = (float)so_trail_x(i); gy[i] = (float)so_trail_y(i); nx[i] = (float)so_ntrail_x(i); ny[i] = (float)so_ntrail_y(i); }
    XYSeries s[] = {{"general relativity", gx, gy, n, "#e69f00", 0}, {"Newton, same start", nx, ny, n, "#0072b2", 1}};
    Ring rings[] = {{"horizon 2M", 2.0, "#111111", 1}, {"photon sphere 3M", 3.0, "#cc79a7", 0}, {"innermost stable 6M", 6.0, "#009e73", 0}};
    char path[512];
    snprintf(path, sizeof path, "%s/orbit.svg", dir);
    if (svg_xy(path, "Orbit around a black hole: r0 = 20 M, L = 4.2 M (5 turns)", "M", 22.0, s, 2, rings, 3)) return 1;
    printf("%d periapses: the orbit turns %.6f rad per lap (elliptic integral %.6f, weak-field 6 pi M / p %.6f)\n",
           (int)so_value(SO_APSES), so_value(SO_PREC), so_value(SO_PREC_EXACT), so_value(SO_PREC_WEAK));
    return 0;
}

/* The whole sky at rest and from a ship at 0.9 c (docs/sky.svg): every catalogue star placed, coloured and sized by
 * the C model, on discs centred on the direction of motion (radius = angle from it / 180 deg). */
static int sky_svg(const char *dir)
{
    const double betas[2] = {0.0, 0.9};
    const int R = 200, top = 70, gap = 70, w = 2 * (2 * R) + 3 * gap, h = top + 2 * R + 110;
    char path[512];
    snprintf(path, sizeof path, "%s/sky.svg", dir);
    FILE *fp = fopen(path, "w");
    if (!fp) return 1;
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\" font-family=\"sans-serif\" font-size=\"13\">\n", w, h, w, h);
    fprintf(fp, "<rect width=\"%d\" height=\"%d\" fill=\"#ffffff\"/>\n", w, h);
    fprintf(fp, "<text x=\"%d\" y=\"28\" font-size=\"17\" font-weight=\"bold\" fill=\"#222\">The whole sky seen from the ship: at rest and at 0.9 c (600 catalogue stars)</text>\n", gap);
    for (int p = 0; p < 2; p++) {
        sv_reset(betas[p]);
        double cx = gap + R + p * (2 * R + gap), cy = top + R;
        fprintf(fp, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%d\" fill=\"#0b0e20\"/>\n", cx, cy, R);
        for (int deg = 30; deg < 180; deg += 30)
            fprintf(fp, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" fill=\"none\" stroke=\"#ffffff30\"/>\n", cx, cy, R * deg / 180.0);
        for (int i = 0; i < SV_STARS_MAX; i++) {
            double f = sv_star(i, SV_STAR_FLUX_SHIP), size = 0.8 + 1.1 * fmax(0.0, log10(f) + 2.5);
            fprintf(fp, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"rgb(%d,%d,%d)\"/>\n", cx + R * sv_star(i, SV_X), cy - R * sv_star(i, SV_Y),
                    fmin(size, 7.0), (int)(255 * sv_star(i, SV_R)), (int)(255 * sv_star(i, SV_G)), (int)(255 * sv_star(i, SV_B)));
        }
        if (p) fprintf(fp, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" fill=\"none\" stroke=\"#f0e442\" stroke-width=\"2\" stroke-dasharray=\"6 4\"/>\n",
                       cx, cy, R * sv_value(SV_HALF_CONE) / 3.14159265358979323846);
        fprintf(fp, "<text x=\"%.1f\" y=\"%d\" text-anchor=\"middle\" font-weight=\"bold\" fill=\"#222\">%s</text>\n", cx, top + 2 * R + 26,
                p ? "beta = 0.9: stars crowd ahead, bluer; behind, sparse and red" : "beta = 0: the sky at rest");
        fprintf(fp, "<text x=\"%.1f\" y=\"%d\" text-anchor=\"middle\" fill=\"#444\">ahead %d of 600 stars (expected %.0f)%s</text>\n", cx, top + 2 * R + 48,
                (int)sv_value(SV_FORWARD), sv_value(SV_FORWARD_EXPECTED), p ? "; dashed ring: half of all stars" : "");
    }
    fprintf(fp, "<text x=\"%d\" y=\"%d\" fill=\"#444\">Centre: the direction of motion; rim: straight behind; rings every 30 deg. Dot size: visible-band flux; colour: the black body at D T.</text>\n", gap, h - 18);
    fprintf(fp, "</svg>\n");
    if (fclose(fp)) return 1;
    sv_reset(0.9);
    printf("at 0.9 c: %d of %d stars ahead (expected %.1f); half of them inside %.2f deg; D ahead %.4f, behind %.4f\n", (int)sv_value(SV_FORWARD), SV_STARS_MAX,
           sv_value(SV_FORWARD_EXPECTED), sv_value(SV_HALF_CONE) * 180.0 / 3.14159265358979323846, sv_value(SV_D_AHEAD), sv_value(SV_D_BEHIND));
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
    if (argc > 1 && strcmp(argv[1], "--sky") == 0) return sky_svg(argc > 2 ? argv[2] : ".");
    if (argc > 1 && strcmp(argv[1], "--orbit") == 0) return orbit_svg(argc > 2 ? argv[2] : ".");
    if (argc > 1 && strcmp(argv[1], "--gif") == 0) return gif_demo(argc > 2 ? argv[2] : "demo.gif");
    return play();
}
