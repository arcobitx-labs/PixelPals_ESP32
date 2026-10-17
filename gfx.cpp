#include "gfx.h"
#include "anim_core.h"
#include "config.h"
#include <string.h>

void gfxFillEllipse(int cx, int cy, float rx, float ry, uint16_t c) {
  if (rx < 0.6f) rx = 0.6f;
  if (ry < 0.6f) ry = 0.6f;
  int R = (int)ceilf(ry);
  float rxx = rx + 0.35f, ryy = ry + 0.35f;
  for (int dy = -R; dy <= R; dy++) {
    float t = (float)dy / ryy;
    if (t > 1.f || t < -1.f) continue;
    int w = (int)floorf(rxx * sqrtf(1.f - t * t));
    oled.drawFastHLine(cx - w, cy + dy, 2 * w + 1, c);
  }
}

bool gfxGet(int x, int y) {
  if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H) return false;
  return (oled.getBuffer()[x + (y >> 3) * SCREEN_W] & (1 << (y & 7))) != 0;
}

void gfxMask(int x0, int y0, int x1, int y1, uint8_t thrNew, uint8_t thrPrev, void (*cb)(int, int)) {
  x0 = iclamp(x0, 0, SCREEN_W - 1); x1 = iclamp(x1, 0, SCREEN_W - 1);
  y0 = iclamp(y0, 0, SCREEN_H - 1); y1 = iclamp(y1, 0, SCREEN_H - 1);
  uint8_t *b = oled.getBuffer();
  for (int y = y0; y <= y1; y++) {
    uint8_t m = 1 << (y & 7);
    uint8_t *row = b + (y >> 3) * SCREEN_W;
    for (int x = x0; x <= x1; x++) {
      if (!(row[x] & m)) continue;
      uint8_t h = hash8(x, y);
      if (h >= thrNew) {
        if (cb && h < thrPrev) cb(x, y);
        row[x] &= ~m;
      }
    }
  }
}

int gfxSampleWhite(int x0, int y0, int x1, int y1, int16_t *xs, int16_t *ys, int maxN) {
  x0 = iclamp(x0, 0, SCREEN_W - 1); x1 = iclamp(x1, 0, SCREEN_W - 1);
  y0 = iclamp(y0, 0, SCREEN_H - 1); y1 = iclamp(y1, 0, SCREEN_H - 1);
  const uint8_t *b = oled.getBuffer();
  int seen = 0, n = 0;
  for (int y = y0; y <= y1; y++) {
    uint8_t m = 1 << (y & 7);
    const uint8_t *row = b + (y >> 3) * SCREEN_W;
    for (int x = x0; x <= x1; x++) {
      if (!(row[x] & m)) continue;
      seen++;
      if (n < maxN) { xs[n] = x; ys[n] = y; n++; }
      else { int j = (int)random(0, seen); if (j < maxN) { xs[j] = x; ys[j] = y; } }
    }
  }
  return n;
}

void gfxShiftRows(int y0, int h, int dx) {
  uint8_t *b = oled.getBuffer();
  bool tmp[SCREEN_W];
  for (int y = y0; y < y0 + h; y++) {
    if (y < 0 || y >= SCREEN_H) continue;
    uint8_t m = 1 << (y & 7);
    uint8_t *row = b + (y >> 3) * SCREEN_W;
    for (int x = 0; x < SCREEN_W; x++) tmp[x] = (row[x] & m) != 0;
    for (int x = 0; x < SCREEN_W; x++) {
      int sx = (x - dx) % SCREEN_W; if (sx < 0) sx += SCREEN_W;
      if (tmp[sx]) row[x] |= m; else row[x] &= ~m;
    }
  }
}

void gfxInvertRows(int y0, int h) {
  uint8_t *b = oled.getBuffer();
  for (int y = y0; y < y0 + h; y++) {
    if (y < 0 || y >= SCREEN_H) continue;
    uint8_t m = 1 << (y & 7);
    uint8_t *row = b + (y >> 3) * SCREEN_W;
    for (int x = 0; x < SCREEN_W; x++) row[x] ^= m;
  }
}

void gfxInvertAll() {
  uint8_t *b = oled.getBuffer();
  for (int i = 0; i < SCREEN_W * SCREEN_H / 8; i++) b[i] = ~b[i];
}

void gfxTextBold(const char *s, int x, int y) {
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(x, y);     oled.print(s);
  oled.setCursor(x + 1, y); oled.print(s);
}

void gfxTextCentered(const char *s, int cx, int y, uint16_t c, bool bold) {
  int w = (int)strlen(s) * 6;
  oled.setTextSize(1);
  oled.setTextColor(c);
  oled.setCursor(cx - w / 2, y);
  oled.print(s);
  if (bold) { oled.setCursor(cx - w / 2 + 1, y); oled.print(s); }
}
