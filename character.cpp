#include "character.h"
#include "display.h"
#include "config.h"
#include "gfx.h"
#include "anim_core.h"
#include "assets.h"

Character character;

#define W SSD1306_WHITE
#define B SSD1306_BLACK

void Character::reset() {
  x = 64; y = 34; scale = 1.f; style = STYLE_HELMET;
  breathRate = 1.f; floatAmt = 0.f; crown = false; showFx = true; showBody = true;
  bounceAmp_ = 0; shakeAmp_ = 0; wiggle_ = false; earSide_ = 0; pulseAmp_ = 0;
}

void Character::update(uint32_t now) {
  float dt = (now - lastUpdate_) * 0.001f;
  if (dt > 0.1f) dt = 0.1f;
  lastUpdate_ = now;
  breathPhase_ += dt * breathRate * (6.2831853f / 3.8f);
  if (breathPhase_ > 6.2831853f * 64.f) breathPhase_ -= 6.2831853f * 64.f;
}

void Character::kickBounce(uint32_t now, float amp, uint16_t period) { bounceStart_ = now; bounceAmp_ = amp; bouncePeriod_ = period; }
void Character::kickShake(uint32_t now, float amp, uint16_t dur)     { shakeStart_ = now; shakeAmp_ = amp; shakeDur_ = dur; }
void Character::wiggleSprout(uint32_t now)                           { wiggleStart_ = now; wiggle_ = true; }
void Character::twitchEar(uint32_t now, int side)                    { earStart_ = now; earSide_ = side; }
void Character::pulseScale(uint32_t now, float amp, uint16_t dur)    { pulseStart_ = now; pulseAmp_ = amp; pulseDur_ = dur; }

void Character::bounds(int &x0, int &y0, int &x1, int &y1) const {
  x0 = cx_ - roundi(30 * S_) - 2; x1 = cx_ + roundi(30 * S_) + 2;
  y0 = cy_ - roundi(31 * S_) - 2; y1 = cy_ + roundi(29 * S_) + 2;
}

// ------------------------------------------------------------------ parts
void Character::drawEar(int side, float ear, bool thin) {
  float tipX = side * (23.5f + (ear < 0 ? -ear * 2.5f : 0.f));
  float tipY = -24.f - ear * 4.f + (ear < 0 ? -ear * 1.5f : 0.f);
  float bx0 = side * 22.f, by0 = -2.5f, bx1 = side * 7.5f, by1 = -17.2f;
  if (side > 0) { tipX *= 0.95f; tipY += 1.5f; }               // right ear is a touch smaller: gives character
  int ax = PX(bx0), ay = PY(by0), tx = PX(tipX), ty = PY(tipY), cx2 = PX(bx1), cy2 = PY(by1);
  oled.fillTriangle(ax, ay, tx, ty, cx2, cy2, W);
  if (S_ >= 0.5f) {
    float k = thin ? 0.76f : 0.5f;
    float mx = (ax + tx + cx2) / 3.f, my = (ay + ty + cy2) / 3.f;
    oled.fillTriangle(roundi(mx + (ax - mx) * k), roundi(my + (ay - my) * k),
                      roundi(mx + (tx - mx) * k), roundi(my + (ty - my) * k),
                      roundi(mx + (cx2 - mx) * k), roundi(my + (cy2 - my) * k), B);
  }
}

void Character::drawSprout(float sprout, uint32_t now) {
  float sway = sinf(now * 0.0021f) * 0.3f;
  if (wiggle_) {
    float p = progress(now, wiggleStart_, 900);
    if (p >= 1.f) wiggle_ = false; else sway += sinf(p * 6.2831853f * 3.f) * (1.f - p) * 1.1f;
  }
  float lift = clampf(sprout, -1.f, 1.f);
  float sx = sway * 2.2f;
  int x0 = PX(0), y0 = PY(-17.5f), x1 = PX(sx), y1 = PY(-22.5f);
  oled.drawLine(x0, y0, x1, y1, W);
  if (S_ >= 0.85f) oled.drawLine(x0 + 1, y0, x1 + 1, y1, W);
  float ly = -25.f - 5.f * lift;
  float lx = 8.f;
  for (int side = -1; side <= 1; side += 2) {
    float ax = sx, ay = -22.5f, tx = sx + side * lx, ty = ly;
    float dx = tx - ax, dy = ty - ay, len = sqrtf(dx * dx + dy * dy);
    float px = -dy / len * 2.1f, py = dx / len * 2.1f;
    float mx = (ax + tx) * 0.5f, my = (ay + ty) * 0.5f;
    oled.fillTriangle(PX(ax), PY(ay), PX(tx), PY(ty), PX(mx + px), PY(my + py), W);
    oled.fillTriangle(PX(ax), PY(ay), PX(tx), PY(ty), PX(mx - px), PY(my - py), W);
  }
}

void Character::drawEye(int side, const FaceParams &f, float blink, float gx, float gy, uint32_t now) {
  float sk = 1.f + 0.22f * f.skew * side;
  int offX = roundi(gx * 1.6f * S_), offY = roundi(gy * 1.0f * S_);     // face turns slightly toward the gaze
  int ex = PX(side * f.eyeDX) + offX, ey = PY(f.eyeY) + offY;
  const float boost = (S_ < 0.8f) ? 1.3f : 1.f;                 // keep eyes readable when the mascot is small
  float rx = f.eyeW * S_ * sk * 0.5f * boost, ry = f.eyeH * S_ * sk * 0.5f * boost;

  // --- heart eyes ---
  if (f.heartEye > 0.5f) {
    float pulse = 1.f + 0.12f * sinf(now * 0.011f);
    float hw = f.eyeW * S_ * 1.15f * pulse;
    int r = imax(1, roundi(hw * 0.27f));
    int ox = roundi(hw * 0.24f), oy = roundi(hw * 0.13f);
    oled.fillCircle(ex - ox, ey - oy, r, W);
    oled.fillCircle(ex + ox, ey - oy, r, W);
    oled.fillTriangle(ex - roundi(hw * 0.5f), ey - roundi(hw * 0.03f), ex + roundi(hw * 0.5f), ey - roundi(hw * 0.03f),
                      ex, ey + roundi(hw * 0.52f), W);
    return;
  }

  // --- closed-arc eyes (joy ^ / sleep u / wink) ---
  float arcAmt = f.arc, dir = f.arcDir;
  if (side > 0 && f.wink > arcAmt) { arcAmt = f.wink; dir = 1.f; }
  if (arcAmt > 0.5f) {
    float A = imax(2, roundi(f.eyeH * S_ * 0.36f));
    int hw = roundi(rx);
    int th = (S_ >= 0.8f) ? 2 : 1;
    for (int i = -hw; i <= hw; i++) {
      float u = hw ? (float)i / hw : 0.f;
      int yy = ey + roundi(dir * (-A * (1.f - u * u) + A * 0.5f));
      for (int t = 0; t < th; t++) oled.drawPixel(ex + i, yy + t, W);
    }
    return;
  }
  if (arcAmt > 0.f) ry *= clampf(1.f - arcAmt * 1.7f, 0.12f, 1.f);

  // --- blink squashes the eye toward a slit ---
  float bl = clamp01(blink);
  ry = ry * (1.f - bl * 0.94f);
  if (ry < 0.7f) ry = 0.7f;

  gfxFillEllipse(ex, ey, rx, ry, W);

  // pupil follows gaze inside the eye
  if (ry > 1.6f) {
    float pk = (S_ < 0.8f) ? 0.62f : 1.f;
    float prx = fmaxf(0.9f, rx * f.pupil * pk), pry = fmaxf(1.1f, ry * f.pupil * 1.15f * pk);
    int px = ex + roundi(gx * (rx - prx) * 0.9f);
    int py = ey + roundi(gy * (ry - pry) * 0.9f);
    gfxFillEllipse(px, py, prx, pry, B);
    if (prx >= 2.f && S_ >= 0.75f) {
      int hx = px - roundi(prx * 0.4f), hy = py - roundi(pry * 0.45f);
      oled.drawPixel(hx, hy, W);
      if (S_ >= 1.f && prx >= 2.8f) { oled.drawPixel(hx + 1, hy, W); oled.drawPixel(hx, hy + 1, W); oled.drawPixel(hx + 1, hy + 1, W); }
    }
  }

  // eyelids (black cut-outs): top lid with tilt, bottom lid lifted into a smile
  int ix = (int)ceilf(rx) + 1;
  int yTop = ey - (int)ceilf(ry) - 1, yBot = ey + (int)ceilf(ry) + 1;
  for (int i = -ix; i <= ix; i++) {
    float u = rx > 0.1f ? (float)i / rx : 0.f;
    float t = -side * u;                                   // +1 at the inner corner
    float topCut = ey - ry + f.lidTop * 2.f * ry + f.lidTilt * ry * 0.9f * t;
    int len = (int)ceilf(topCut) - yTop;
    if (len > 0) oled.drawFastVLine(ex + i, yTop, len, B);
    bool lidOn = f.lidTop > 0.03f || fabsf(f.lidTilt) > 0.05f;
    if (lidOn && ry > 2.f) {                                // white lid edge so half-closed eyes read clearly
      int ly = (int)ceilf(topCut);
      float v = (ly - ey) / ry;
      if (u * u + v * v <= 1.f) oled.drawPixel(ex + i, ly, W);
    }
    if (f.lidBot > 0.01f && S_ >= 0.8f) {
      float botCut = ey + ry - f.lidBot * 2.f * ry * (1.f - 0.7f * u * u);
      int y0 = (int)floorf(botCut) + 1;
      if (yBot - y0 + 1 > 0) oled.drawFastVLine(ex + i, y0, yBot - y0 + 1, B);
      float v = ((y0 - 1) - ey) / ry;
      if (ry > 2.f && u * u + v * v <= 1.f) oled.drawPixel(ex + i, y0 - 1, W);
    }
  }
}

void Character::drawMouth(const FaceParams &f) {
  // (offset is applied by the caller through cx_/cy_ adjustments)
  int mx = cx_, my = PY(f.mouthY);
  float w = f.mouthW * S_;
  if (f.mouthOpen > 0.2f) {
    float h = f.mouthOpen * 3.4f * S_;
    float rx = fmaxf(1.f, w * 0.5f * (0.8f + 0.2f * f.mouthOpen));
    if (f.mouthCurve > 0.3f) {                              // D-shaped open smile
      for (int dy = 0; dy <= (int)ceilf(h); dy++) {
        float t = dy / (h + 0.4f);
        int ww = (int)floorf(rx * sqrtf(fmaxf(0.f, 1.f - t * t)) + 0.5f);
        oled.drawFastHLine(mx - ww, my + dy, 2 * ww + 1, W);
      }
    } else gfxFillEllipse(mx, my, rx * 0.8f, fmaxf(1.f, h * 0.6f), W);
  } else {
    int n = imax(3, roundi(w));
    for (int i = 0; i <= n; i++) {
      float u = (float)i / n;
      int xx = mx + roundi((u - 0.5f) * w);
      int yy = my + roundi(f.mouthCurve * 2.0f * S_ * (1.f - (2.f * u - 1.f) * (2.f * u - 1.f)));
      oled.drawPixel(xx, yy, W);
    }
  }
}

// ------------------------------------------------------------------ main draw
void Character::drawWith(const FaceParams &f0, float blink, float gx, float gy, uint32_t now) {
  FaceParams f = f0;

  // transient layers
  float by = sinf(breathPhase_) * 0.9f * f.bob;
  float bx = sinf(breathPhase_ * 0.5f) * 0.4f * f.bob;
  if (bounceAmp_ > 0.f) { float p = progress(now, bounceStart_, bouncePeriod_); if (p < 1.f) by -= bounceAmp_ * sinf(p * 3.14159f); }
  float sx = 0;
  if (shakeAmp_ > 0.f) { float p = progress(now, shakeStart_, shakeDur_); if (p < 1.f) sx = shakeAmp_ * sinf(p * 3.14159f * 7.f) * (1.f - p); }
  float fx = 0, fy = 0;
  if (floatAmt > 0.f) { fx = sinf(now * 0.0011f) * 3.2f * floatAmt; fy = sinf(now * 0.0016f + 1.f) * 2.6f * floatAmt; }
  float sc = scale;
  if (pulseAmp_ > 0.f) { float p = progress(now, pulseStart_, pulseDur_); if (p < 1.f) sc *= 1.f + pulseAmp_ * sinf(p * 3.14159f); }
  if (earSide_ != 0) {
    float p = progress(now, earStart_, 380);
    if (p >= 1.f) earSide_ = 0;
    else { float k = sinf(p * 3.14159f) * 1.1f; if (earSide_ < 0) f.earL += k; else f.earR += k; }
  }

  S_ = sc;
  cx_ = roundi(x + bx + sx + fx);
  cy_ = roundi(y + by + fy);

  const bool helmet = (style == STYLE_HELMET);
  const bool blob   = (style == STYLE_BLOB);

  // 1) ears (behind the head)
  if (!blob) { drawEar(-1, f.earL, !helmet); drawEar(+1, f.earR, !helmet); }

  // 2) head: helmet ring (thick) or thin outline
  float rxo = blob ? 22.f : 21.f, ryo = blob ? 17.f : 18.f;
  float t = helmet ? fmaxf(2.0f, 3.0f * S_) : fmaxf(1.0f, 1.4f * S_);
  gfxFillEllipse(cx_, cy_, rxo * S_, ryo * S_, W);
  gfxFillEllipse(cx_, cy_, rxo * S_ - t, ryo * S_ - t * 0.95f, B);

  if (helmet) {
    // chin plate
    gfxFillEllipse(cx_, PY(15.8f), 9.f * S_, 4.7f * S_, W);
    if (S_ >= 0.85f) {
      // visor glints: little gaps in the ring
      oled.drawLine(PX(-15.5f), PY(-11.5f), PX(-11.5f), PY(-15.f), B);
      oled.drawLine(PX(-15.f), PY(-9.f), PX(-13.f), PY(-10.5f), B);
      oled.drawLine(PX(14.f), PY(13.f), PX(16.f), PY(10.f), B);
      oled.drawPixel(PX(-2.5f), PY(15.5f), B); oled.drawPixel(PX(2.5f), PY(15.5f), B);
    }
  }

  // 3) face
  drawEye(-1, f, blink, gx, gy, now);
  drawEye(+1, f, blink, gx, gy, now);
  {
    int sx2 = cx_, sy2 = cy_;
    int offX = roundi(gx * 1.6f * S_), offY = roundi(gy * 1.0f * S_);
    cx_ += offX; cy_ += offY;
    drawMouth(f);
    if (f.blush > 0.15f && S_ >= 0.8f) {
      for (int side = -1; side <= 1; side += 2) {
        int bx0 = PX(side * 13.5f) - 2, by0 = PY(4.2f);
        for (int yy = 0; yy < 2; yy++) for (int xx = 0; xx < 5; xx++) if (((xx + yy) & 1) == 0) oled.drawPixel(bx0 + xx, by0 + yy, W);
      }
    }
    cx_ = sx2; cy_ = sy2;
  }

  // 4) paws and foot
  if (showBody) {
    float pawY = 22.5f - 6.5f * f.paw, pawX = 14.5f + 3.f * f.paw;
    int r = imax(1, roundi(3.f * S_));
    if (blob) {
      gfxFillEllipse(PX(-7.f), PY(19.5f), 3.f * S_, 2.f * S_, W);
      gfxFillEllipse(PX(7.f), PY(19.5f), 3.f * S_, 2.f * S_, W);
    } else {
      oled.fillCircle(PX(-pawX), PY(pawY), r, W);
      oled.fillCircle(PX(pawX), PY(pawY), r, W);
      gfxFillEllipse(cx_, PY(25.2f), 6.5f * S_, 2.6f * S_, W);
    }
  }

  // 5) sprout / crown
  if (crown) {
    int cxw = cx_ - CROWN_W / 2, cyw = PY(-18.f) - CROWN_H + 1;
    oled.fillRect(cxw, cyw, CROWN_W, CROWN_H, B);
    oled.drawBitmap(cxw, cyw, CROWN, CROWN_W, CROWN_H, W);
  } else drawSprout(f.sprout, now);
}

void Character::draw(uint32_t now) {
  drawWith(emotion.face(), emotion.blink(), emotion.gazeX(), emotion.gazeY(), now);
  if (showFx) drawFx(emotion.fx(), emotion.fxStrength(now), now);
}

// ------------------------------------------------------------------ supporting fx (hearts, ?, !, Zz ...)
static inline void bmp(int x, int y, const uint8_t *b, int w, int h) {
  if (x < -w || y < -h || x >= SCREEN_W || y >= SCREEN_H) return;
  oled.drawBitmap(x, y, b, w, h, W);
}
static void boldChar(int x, int y, char c) {
  oled.setTextSize(1); oled.setTextColor(W);
  oled.setCursor(x, y);     oled.write(c);
  oled.setCursor(x + 1, y); oled.write(c);
}

void Character::drawFx(Fx fx, float strength, uint32_t now) {
  if (fx == FX_NONE || strength < 0.3f || S_ < 0.75f) return;
  float bob = sinf(now * 0.004f) * 1.4f;
  switch (fx) {
    case FX_HEART: {
      bool big = sinf(now * 0.009f) > -0.2f;
      if (big) bmp(PX(27) - 3, PY(-20 + bob) - 3, HEART7, HEART7_W, HEART7_H);
      else     bmp(PX(27) - 2, PY(-20 + bob) - 2, HEART5, HEART5_W, HEART5_H);
    } break;
    case FX_HEARTS: {
      static const int8_t hx[4] = {-32, 31, -35, 34}, hy[4] = {-18, -20, 3, 7};
      for (int i = 0; i < 4; i++) {
        float b2 = sinf(now * 0.0035f + i * 1.7f) * 1.6f;
        bool big = (i < 2) ? true : (sinf(now * 0.008f + i) > 0);
        if (big) bmp(PX(hx[i]) - 3, PY(hy[i] + b2) - 3, HEART7, HEART7_W, HEART7_H);
        else     bmp(PX(hx[i]) - 2, PY(hy[i] + b2) - 2, HEART5, HEART5_W, HEART5_H);
      }
    } break;
    case FX_SPARKLE: {
      static const int8_t sx[4] = {28, -30, 26, -27}, sy[4] = {-18, -10, 8, 12};
      int idx = (now / 260) & 3;
      for (int i = 0; i < 4; i++) {
        if (i == idx) bmp(PX(sx[i]) - 2, PY(sy[i]) - 2, SPARK5, SPARK5_W, SPARK5_H);
        else if (i == ((idx + 2) & 3)) bmp(PX(sx[i]) - 1, PY(sy[i]) - 1, SPARK3, SPARK3_W, SPARK3_H);
      }
    } break;
    case FX_QUESTION: boldChar(PX(25) - 3, PY(-27 + bob), '?'); break;
    case FX_EXCLAIM:  boldChar(PX(27) - 3, PY(-26 + (sinf(now * 0.02f) > 0 ? 1 : 0)), '!'); break;
    case FX_ZZZ: {
      uint32_t ph = now % 3000;
      if (ph > 300)  boldChar(PX(24), PY(-10 - (ph - 300) * 0.002f), 'z');
      if (ph > 1100) boldChar(PX(31), PY(-20 - (ph - 1100) * 0.0025f), 'Z');
    } break;
    case FX_DOTS: {
      int n = (int)((now / 380) % 4);
      for (int i = 0; i < n && i < 3; i++) oled.fillRect(PX(22) + i * 4, PY(-20), 2, 2, W);
    } break;
    case FX_TEAR: {
      float ph = (float)(now % 2200) / 2200.f;
      if (ph < 0.75f) bmp(PX(-14.5f) - 1, PY(3.f + ph * 8.f), TEAR, TEAR_W, TEAR_H);
    } break;
    case FX_ANGER:
      if ((now / 320) & 1) bmp(PX(24) - 2, PY(-21) - 2, ANGER, ANGER_W, ANGER_H);
      break;
    default: break;
  }
}
