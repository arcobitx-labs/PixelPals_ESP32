#include "effects.h"
#include "display.h"
#include "gfx.h"
#include "anim_core.h"
#include "assets.h"
#include "config.h"

// ------------------------------------------------------------------ starfield
struct Star { float x, y; uint8_t layer, phase; };
static Star stars[32];

void starsInit() {
  for (int i = 0; i < 32; i++) {
    stars[i].x = rndf(0, SCREEN_W); stars[i].y = rndf(0, SCREEN_H);
    stars[i].layer = (uint8_t)(i % 3); stars[i].phase = (uint8_t)rndi(0, 255);
  }
}

void starsUpdate(float dt, float speedMul) {
  static const float base[3] = {5.f, 11.f, 22.f};
  for (int i = 0; i < 32; i++) {
    stars[i].x -= base[stars[i].layer] * speedMul * dt;
    if (stars[i].x < -40.f * (speedMul > 8 ? 1 : 0)) {
      if (stars[i].x < 0) { stars[i].x += SCREEN_W + (speedMul > 8 ? 40 : 0); stars[i].y = rndf(0, SCREEN_H); }
    }
  }
}

void starsDraw(float density, float warp, uint32_t now) {
  int n = (int)(32 * clamp01(density));
  for (int i = 0; i < n; i++) {
    int x = roundi(stars[i].x), y = roundi(stars[i].y);
    if (warp > 0.05f) {
      int len = roundi(warp * (stars[i].layer + 1) * 11.f);
      oled.drawFastHLine(x, y, len + 1, SSD1306_WHITE);
    } else if (stars[i].layer == 2 && (((now >> 8) + stars[i].phase) & 7) == 0) {
      oled.drawBitmap(x - 2, y - 2, SPARK5, SPARK5_W, SPARK5_H, SSD1306_WHITE);   // twinkle
    } else if (stars[i].layer == 1 && (((now >> 9) + stars[i].phase) & 7) == 0) {
      oled.drawBitmap(x - 1, y - 1, SPARK3, SPARK3_W, SPARK3_H, SSD1306_WHITE);
    } else oled.drawPixel(x, y, SSD1306_WHITE);
  }
}

// ------------------------------------------------------------------ planet with ring
void drawPlanet(int cx, int cy, int r) {
  const float tilt = -0.38f, ct = cosf(tilt), st = sinf(tilt);
  const float rxr = r * 1.8f, ryr = r * 0.42f;
  const int N = 56;
  int px[N + 1], py[N + 1]; bool front[N + 1];
  for (int k = 0; k <= N; k++) {
    float th = k * (6.2831853f / N);
    float x = rxr * cosf(th), y = ryr * sinf(th);
    px[k] = cx + roundi(x * ct - y * st); py[k] = cy + roundi(x * st + y * ct);
    front[k] = sinf(th) > 0;
  }
  // back half of the ring (only where it is outside the planet)
  for (int k = 0; k < N; k++) {
    if (front[k]) continue;
    int dx = px[k] - cx, dy = py[k] - cy;
    if (dx * dx + dy * dy > r * r) oled.drawLine(px[k], py[k], px[k + 1], py[k + 1], SSD1306_WHITE);
  }
  oled.fillCircle(cx, cy, r, SSD1306_BLACK);
  oled.drawCircle(cx, cy, r, SSD1306_WHITE);
  for (int dy = -r + 1; dy < r; dy++) for (int dx = -r + 1; dx < r; dx++)       // dithered shading, lower right
    if (dx * dx + dy * dy < (r - 1) * (r - 1) && ((dx + dy) & 1) == 0 && (dx + dy) > r * 0.25f) oled.drawPixel(cx + dx, cy + dy, SSD1306_WHITE);
  for (int k = 0; k < N; k++) if (front[k]) oled.drawLine(px[k], py[k], px[k + 1], py[k + 1], SSD1306_WHITE);
}

// ------------------------------------------------------------------ shooting star
static float shX, shY, shVX, shVY; static uint32_t shStart; static bool shOn;
void shooterLaunch(uint32_t now) {
  shX = SCREEN_W + 6; shY = rndf(4, 18); shVX = -120.f; shVY = 38.f; shStart = now; shOn = true;
}
void shooterDraw(uint32_t now) {
  if (!shOn) return;
  float t = (now - shStart) * 0.001f;
  float x = shX + shVX * t, y = shY + shVY * t;
  if (x < -20 || y > SCREEN_H + 4) { shOn = false; return; }
  for (int i = 0; i < 9; i++) {
    int tx = roundi(x - shVX * 0.008f * i), ty = roundi(y - shVY * 0.008f * i);
    if (i < 6 || (i & 1)) oled.drawPixel(tx, ty, SSD1306_WHITE);
  }
  oled.drawPixel(roundi(x) + 1, roundi(y), SSD1306_WHITE);
}

// ------------------------------------------------------------------ energy ring
void drawEnergyRing(int cx, int cy, uint32_t now, float rs) {
  const int N = 72;
  float ph = now * 0.0030f;
  int step = (int)(now / 70);
  for (int o = 0; o < 2; o++) {
    float tilt = o ? 0.5f : -0.5f, ct = cosf(tilt), st = sinf(tilt);
    float rx = 40.f * rs, ry = 13.f * rs, dir = o ? -1.f : 1.f;
    int lx = 0, ly = 0;
    for (int k = 0; k <= N; k++) {
      float th = ph * dir + k * (6.2831853f / N);
      float x = rx * cosf(th), y = ry * sinf(th);
      int px = cx + roundi(x * ct - y * st), py = cy + roundi(x * st + y * ct);
      if (k > 0 && ((k + step * (o ? -1 : 1)) % 7) < 5) oled.drawLine(lx, ly, px, py, SSD1306_WHITE);   // shimmering dashed orbit
      if ((k % 18) == 0) oled.fillRect(px - 1, py - 1, 3, 3, SSD1306_WHITE);                               // orbiting energy beads
      lx = px; ly = py;
    }
  }
}

// ------------------------------------------------------------------ big dithered heart (reference "Heart Pulse")
void drawBigHeart(int cx, int cy, float pulse) {
  const uint8_t *bm; int w, h;
  if (pulse < 0.33f)      { bm = HEART_BIG_S; w = HEART_BIG_S_W; h = HEART_BIG_S_H; }
  else if (pulse < 0.66f) { bm = HEART_BIG_M; w = HEART_BIG_M_W; h = HEART_BIG_M_H; }
  else                    { bm = HEART_BIG_L; w = HEART_BIG_L_W; h = HEART_BIG_L_H; }
  oled.drawBitmap(cx - w / 2, cy - h / 2, bm, w, h, SSD1306_WHITE);
}

// ------------------------------------------------------------------ glitch post-process
void glitchApply(uint32_t now, float k) {
  if (k <= 0.01f) return;
  int bands = 1 + (int)(k * 5.f);
  for (int b = 0; b < bands; b++) {
    int y = (int)rndi(0, SCREEN_H - 2), h = (int)rndi(1, 7);
    int dx = (int)(rndf(-1.f, 1.f) * (3.f + 15.f * k));
    gfxShiftRows(y, h, dx);
  }
  int dashes = (int)(k * 7.f);
  for (int i = 0; i < dashes; i++) {
    int y = (int)rndi(0, SCREEN_H), x = (int)rndi(0, 100), len = (int)rndi(4, 30);
    oled.drawFastHLine(x, y, len, SSD1306_WHITE);
  }
  int sy = (int)((now / 8) % (SCREEN_H + 10)) - 5;                         // scan bar
  gfxInvertRows(sy, 2);
}
