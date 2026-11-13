#ifndef SVG_H
#define SVG_H

typedef struct {
    const char *name;
    const float *y;
    const char *color;
} Series;

/* Write a self-contained line chart. Returns 0 on success. */
int svg_plot(const char *path, const char *title, const char *xlabel, const char *ylabel,
             const float *x, int count, const Series *series, int nseries);

/* A curve in the plane with its own points (orbits, light rays). dashed: 0 solid, 1 dashed. */
typedef struct {
    const char *name;
    const float *x, *y;
    int count;
    const char *color;
    int dashed;
} XYSeries;

/* A circle about the origin (horizon, photon sphere, innermost stable orbit). fill: 0 outline, 1 filled. */
typedef struct {
    const char *name;
    double r;
    const char *color;
    int fill;
} Ring;

/* Write a square chart with equal axes from -extent to extent, curves clipped to the frame. Returns 0 on success. */
int svg_xy(const char *path, const char *title, const char *unit, double extent,
           const XYSeries *series, int nseries, const Ring *rings, int nrings);

#endif
