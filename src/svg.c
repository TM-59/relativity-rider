#include "svg.h"

#include <stdio.h>

#define W 720
#define H 400
#define ML 70
#define MR 20
#define MT 44
#define MB 56

static void range(const float *a, int n, float *lo, float *hi)
{
    *lo = *hi = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] < *lo) *lo = a[i];
        if (a[i] > *hi) *hi = a[i];
    }
}

int svg_plot(const char *path, const char *title, const char *xlabel, const char *ylabel,
             const float *x, int count, const Series *series, int nseries)
{
    if (count < 2 || nseries < 1) return -1;
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    float x0, x1, y0, y1;
    range(x, count, &x0, &x1);
    range(series[0].y, count, &y0, &y1);
    for (int s = 1; s < nseries; s++) {
        float lo, hi;
        range(series[s].y, count, &lo, &hi);
        if (lo < y0) y0 = lo;
        if (hi > y1) y1 = hi;
    }
    if (x1 <= x0) x1 = x0 + 1.0f;
    if (y1 <= y0) { y0 -= 0.5f; y1 += 0.5f; }
    float pad = 0.05f * (y1 - y0);
    y0 -= pad;
    y1 += pad;

    float pw = W - ML - MR, ph = H - MT - MB;
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\" "
                "font-family=\"sans-serif\" font-size=\"12\">\n", W, H, W, H);
    fprintf(fp, "<rect width=\"%d\" height=\"%d\" fill=\"#ffffff\"/>\n", W, H);
    fprintf(fp, "<text x=\"%d\" y=\"26\" font-size=\"16\" font-weight=\"bold\" fill=\"#222\">%s</text>\n", ML, title);
    for (int k = 0; k <= 5; k++) {
        float fx = (float)k / 5.0f;
        float px = ML + fx * pw, py = MT + ph - fx * ph;
        fprintf(fp, "<line x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" y2=\"%d\" stroke=\"#e3e3e3\"/>\n", px, MT, px, MT + (int)ph);
        fprintf(fp, "<line x1=\"%d\" y1=\"%.1f\" x2=\"%d\" y2=\"%.1f\" stroke=\"#e3e3e3\"/>\n", ML, py, ML + (int)pw, py);
        fprintf(fp, "<text x=\"%.1f\" y=\"%d\" text-anchor=\"middle\" fill=\"#444\">%.3g</text>\n",
                px, MT + (int)ph + 18, x0 + fx * (x1 - x0));
        fprintf(fp, "<text x=\"%d\" y=\"%.1f\" text-anchor=\"end\" fill=\"#444\">%.3g</text>\n",
                ML - 8, py + 4, y0 + fx * (y1 - y0));
    }
    fprintf(fp, "<rect x=\"%d\" y=\"%d\" width=\"%.0f\" height=\"%.0f\" fill=\"none\" stroke=\"#888\"/>\n", ML, MT, pw, ph);
    fprintf(fp, "<text x=\"%.0f\" y=\"%d\" text-anchor=\"middle\" fill=\"#222\">%s</text>\n", ML + pw / 2, H - 12, xlabel);
    fprintf(fp, "<text transform=\"translate(16 %.0f) rotate(-90)\" text-anchor=\"middle\" fill=\"#222\">%s</text>\n", MT + ph / 2, ylabel);
    for (int s = 0; s < nseries; s++) {
        fprintf(fp, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2\" points=\"", series[s].color);
        for (int i = 0; i < count; i++)
            fprintf(fp, "%.1f,%.1f ", ML + (x[i] - x0) / (x1 - x0) * pw, MT + ph - (series[s].y[i] - y0) / (y1 - y0) * ph);
        fprintf(fp, "\"/>\n");
        fprintf(fp, "<rect x=\"%d\" y=\"%d\" width=\"12\" height=\"12\" fill=\"%s\"/><text x=\"%d\" y=\"%d\" fill=\"#222\">%s</text>\n",
                ML + 8 + s * 150, MT + 8, series[s].color, ML + 26 + s * 150, MT + 19, series[s].name);
    }
    fprintf(fp, "</svg>\n");
    return fclose(fp) == 0 ? 0 : -1;
}

int svg_xy(const char *path, const char *title, const char *unit, double extent,
           const XYSeries *series, int nseries, const Ring *rings, int nrings)
{
    const int ml = 60, mt = 44, plot = 440, w = ml + plot + 200, h = mt + plot + 50;
    if (!(extent > 0.0) || nseries < 0 || nrings < 0) return -1;
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    double k = plot / (2.0 * extent);
    double cx = ml + plot / 2.0, cy = mt + plot / 2.0;
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\" "
                "font-family=\"sans-serif\" font-size=\"12\">\n", w, h, w, h);
    fprintf(fp, "<rect width=\"%d\" height=\"%d\" fill=\"#ffffff\"/>\n", w, h);
    fprintf(fp, "<text x=\"%d\" y=\"26\" font-size=\"16\" font-weight=\"bold\" fill=\"#222\">%s</text>\n", ml, title);
    fprintf(fp, "<clipPath id=\"frame\"><rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/></clipPath>\n", ml, mt, plot, plot);
    for (int i = 0; i <= 4; i++) {
        double v = -extent + i * extent / 2.0, p = ml + i * plot / 4.0, q = mt + plot - i * plot / 4.0;
        fprintf(fp, "<line x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" y2=\"%d\" stroke=\"#e3e3e3\"/>\n", p, mt, p, mt + plot);
        fprintf(fp, "<line x1=\"%d\" y1=\"%.1f\" x2=\"%d\" y2=\"%.1f\" stroke=\"#e3e3e3\"/>\n", ml, q, ml + plot, q);
        fprintf(fp, "<text x=\"%.1f\" y=\"%d\" text-anchor=\"middle\" fill=\"#444\">%.3g</text>\n", p, mt + plot + 18, v);
        fprintf(fp, "<text x=\"%d\" y=\"%.1f\" text-anchor=\"end\" fill=\"#444\">%.3g</text>\n", ml - 8, q + 4, v);
    }
    fprintf(fp, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"none\" stroke=\"#888\"/>\n", ml, mt, plot, plot);
    fprintf(fp, "<text x=\"%.0f\" y=\"%d\" text-anchor=\"middle\" fill=\"#222\">x [%s]</text>\n", cx, h - 10, unit);
    fprintf(fp, "<text transform=\"translate(16 %.0f) rotate(-90)\" text-anchor=\"middle\" fill=\"#222\">y [%s]</text>\n", cy, unit);
    int row = 0;
    fprintf(fp, "<g clip-path=\"url(#frame)\">\n");
    for (int i = 0; i < nrings; i++)
        fprintf(fp, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.2f\" fill=\"%s\" stroke=\"%s\" stroke-width=\"1.5\"%s/>\n", cx, cy, rings[i].r * k,
                rings[i].fill ? rings[i].color : "none", rings[i].color, rings[i].fill ? "" : " stroke-dasharray=\"5 4\"");
    for (int s = 0; s < nseries; s++) {
        fprintf(fp, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"1.6\"%s points=\"", series[s].color,
                series[s].dashed ? " stroke-dasharray=\"6 4\"" : "");
        for (int i = 0; i < series[s].count; i++) fprintf(fp, "%.1f,%.1f ", cx + series[s].x[i] * k, cy - series[s].y[i] * k);
        fprintf(fp, "\"/>\n");
    }
    fprintf(fp, "</g>\n");
    for (int s = 0; s < nseries; s++, row++)
        fprintf(fp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"%s\" stroke-width=\"2\"%s/><text x=\"%d\" y=\"%d\" fill=\"#222\">%s</text>\n",
                ml + plot + 14, mt + 10 + 20 * row, ml + plot + 34, mt + 10 + 20 * row, series[s].color,
                series[s].dashed ? " stroke-dasharray=\"6 4\"" : "", ml + plot + 40, mt + 14 + 20 * row, series[s].name);
    for (int i = 0; i < nrings; i++, row++)
        fprintf(fp, "<circle cx=\"%d\" cy=\"%d\" r=\"6\" fill=\"%s\" stroke=\"%s\"/><text x=\"%d\" y=\"%d\" fill=\"#222\">%s</text>\n",
                ml + plot + 24, mt + 10 + 20 * row, rings[i].fill ? rings[i].color : "none", rings[i].color, ml + plot + 40, mt + 14 + 20 * row, rings[i].name);
    fprintf(fp, "</svg>\n");
    return fclose(fp) == 0 ? 0 : -1;
}
