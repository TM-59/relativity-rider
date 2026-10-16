#include "render.h"

#include <math.h>

#define LY_PER_COL 0.25
#define YR_PER_ROW 0.5
#define ROCKET_ROW (RENDER_ROWS - 6)

static void put(char g[RENDER_ROWS][RENDER_COLS], const Rocket *c, double x, double t, char ch)
{
    int col = RENDER_COLS / 2 + (int)lround((x - c->x) / LY_PER_COL);
    int row = ROCKET_ROW - (int)lround((t - c->t) / YR_PER_ROW);
    if (col >= 0 && col < RENDER_COLS && row >= 0 && row < RENDER_ROWS) g[row][col] = ch;
}

size_t render_frame(const Rocket *r, const Rocket *trail, int trail_len, double dest_x, char *buf, size_t cap)
{
    if (cap < (size_t)(RENDER_COLS + 1) * RENDER_ROWS + 1) return 0;
    char g[RENDER_ROWS][RENDER_COLS];
    for (int i = 0; i < RENDER_ROWS; i++)
        for (int j = 0; j < RENDER_COLS; j++) g[i][j] = ' ';
    double t_top = r->t + ROCKET_ROW * YR_PER_ROW, t_bottom = r->t - (RENDER_ROWS - 1 - ROCKET_ROW) * YR_PER_ROW;
    for (double t = t_bottom; t <= t_top; t += YR_PER_ROW / 2.0) {
        put(g, r, 0.0, t, '|');
        put(g, r, dest_x, t, '#');
        if (t > 0.0) {
            put(g, r, t, t, '/');
            put(g, r, -t, t, '\\');
        }
    }
    for (int i = 0; i < trail_len; i++) put(g, r, trail[i].x, trail[i].t, '.');
    put(g, r, r->x, r->t, '@');
    size_t used = 0;
    for (int i = 0; i < RENDER_ROWS; i++) {
        for (int j = 0; j < RENDER_COLS; j++) buf[used++] = g[i][j];
        buf[used++] = '\n';
    }
    buf[used] = '\0';
    return used;
}
