#pragma once
// Small drawing / framebuffer helpers on top of Adafruit_GFX (no extra RAM).
#include "display.h"

void gfxFillEllipse(int cx, int cy, float rx, float ry, uint16_t c = SSD1306_WHITE);
bool gfxGet(int x, int y);
// Dissolve / reveal: clears every lit pixel in the box whose hash >= thrNew.
// If cb is given it is called for pixels removed that were still "alive" at thrPrev (to spawn particles).
void gfxMask(int x0, int y0, int x1, int y1, uint8_t thrNew, uint8_t thrPrev, void (*cb)(int, int));
// Reservoir-sample up to maxN lit pixels inside a box. Returns how many were stored.
int  gfxSampleWhite(int x0, int y0, int x1, int y1, int16_t *xs, int16_t *ys, int maxN);
// Cyberpunk glitch helpers
void gfxShiftRows(int y0, int h, int dx);     // horizontal wrap-shift of a band of rows
void gfxInvertRows(int y0, int h);
void gfxInvertAll();
// Text
void gfxTextCentered(const char *s, int cx, int y, uint16_t c = SSD1306_WHITE, bool bold = false);
void gfxTextBold(const char *s, int x, int y);
