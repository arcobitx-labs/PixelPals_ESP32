#include "particles.h"
#include "display.h"
#include "anim_core.h"
#include "assets.h"
#include "config.h"

Particles particles;

void Particles::clear() { for (int i = 0; i < N; i++) p[i].kind = PK_NONE; last_ = millis(); }

int Particles::count() const { int n = 0; for (int i = 0; i < N; i++) if (p[i].kind != PK_NONE) n++; return n; }

bool Particles::spawn(uint8_t kind, float x, float y, float vx, float vy, uint16_t life, uint8_t aux) {
  for (int i = 0; i < N; i++) if (p[i].kind == PK_NONE) {
    Particle &q = p[i];
    q.kind = kind; q.x = x; q.y = y; q.vx = vx; q.vy = vy; q.age = 0; q.life = life; q.aux = aux; q.tx = q.ty = 0;
    return true;
  }
  return false;
}

void Particles::burst(float x, float y, int n, float speed, uint16_t life, uint8_t kind) {
  for (int i = 0; i < n; i++) {
    float a = rndf(0.f, 6.2831853f), s = speed * rndf(0.45f, 1.0f);
    spawn(kind, x, y, cosf(a) * s, sinf(a) * s, (uint16_t)(life * rndf(0.7f, 1.1f)), 0);
  }
}

void Particles::ring(float x, float y, float r, int n, float speed, uint16_t life, uint8_t kind) {
  float a0 = rndf(0.f, 6.2831853f);
  for (int i = 0; i < n; i++) {
    float a = a0 + i * (6.2831853f / n) + rndf(-0.15f, 0.15f), s = speed * rndf(0.6f, 1.0f);
    spawn(kind, x + cosf(a) * r * 1.15f, y + sinf(a) * r, cosf(a) * s, sinf(a) * s, (uint16_t)(life * rndf(0.75f, 1.1f)), 0);
  }
}

void Particles::seek(float sx, float sy, int tx, int ty, int delayMs, uint16_t durMs) {
  for (int i = 0; i < N; i++) if (p[i].kind == PK_NONE) {
    Particle &q = p[i];
    q.kind = PK_SEEK; q.x = sx; q.y = sy; q.vx = q.vy = 0;
    q.tx = tx; q.ty = ty; q.age = (int16_t)(-delayMs); q.life = durMs; q.aux = 0;
    return;
  }
}

void Particles::heart(float x, float y) {
  spawn(PK_HEART, x, y, rndf(-4.f, 4.f), rndf(-17.f, -11.f), (uint16_t)rndMs(1500, 2200), (uint8_t)rndi(0, 64));
}

int Particles::retargetAll(const int16_t *tx, const int16_t *ty, int nTargets, int maxDelayMs, uint16_t durMs) {
  int k = 0;
  for (int i = 0; i < N && k < nTargets; i++) {
    Particle &q = p[i];
    if (q.kind != PK_DOT && q.kind != PK_SPARK) continue;
    q.kind = PK_SEEK; q.tx = tx[k]; q.ty = ty[k]; q.age = (int16_t)(-rndi(0, maxDelayMs + 1)); q.life = durMs; k++;
  }
  // not enough free-floating particles? spawn extra seekers from random edges
  for (; k < nTargets; k++) {
    float sx = rndf(0, SCREEN_W), sy = rndf(0, SCREEN_H);
    seek(sx, sy, tx[k], ty[k], (int)rndi(0, maxDelayMs + 1), durMs);
  }
  return k;
}

void Particles::update(uint32_t now) {
  int dtMs = (int)(now - last_);
  last_ = now;
  if (dtMs > 100) dtMs = 100;
  if (dtMs < 0) dtMs = 0;
  float dt = dtMs * 0.001f;
  float drag = powf(0.20f, dt);            // ~80% velocity lost per second

  for (int i = 0; i < N; i++) {
    Particle &q = p[i];
    if (q.kind == PK_NONE) continue;
    q.age += dtMs;
    if (q.kind == PK_SEEK) {
      if (q.age >= (int)q.life) q.kind = PK_NONE;
      continue;
    }
    if (q.age < 0) continue;
    if (q.age >= (int)q.life) { q.kind = PK_NONE; continue; }
    if (q.kind == PK_HEART) {
      q.x += (q.vx + sinf(q.age * 0.006f + q.aux) * 9.f) * dt;
      q.y += q.vy * dt;
    } else {
      q.x += q.vx * dt; q.y += q.vy * dt;
      q.vx *= drag; q.vy *= drag;
    }
  }
}

void Particles::draw() {
  for (int i = 0; i < N; i++) {
    const Particle &q = p[i];
    if (q.kind == PK_NONE) continue;
    if (q.kind == PK_SEEK) {
      if (q.age < 0) continue;
      float e = easeOutBack(clamp01((float)q.age / (float)q.life));
      oled.drawPixel(roundi(lerpf(q.x, (float)q.tx, e)), roundi(lerpf(q.y, (float)q.ty, e)), SSD1306_WHITE);
      continue;
    }
    if (q.age < 0) continue;
    float f = (float)q.age / (float)q.life;
    int x = roundi(q.x), y = roundi(q.y);
    switch (q.kind) {
      case PK_DOT:
        if (f > 0.7f && ((q.age / 50) & 1)) break;       // flicker out
        oled.drawPixel(x, y, SSD1306_WHITE);
        break;
      case PK_SPARK:
        if (f > 0.75f) { oled.drawPixel(x, y, SSD1306_WHITE); break; }
        if (f < 0.55f) oled.drawBitmap(x - 2, y - 2, SPARK5, SPARK5_W, SPARK5_H, SSD1306_WHITE);
        else           oled.drawBitmap(x - 1, y - 1, SPARK3, SPARK3_W, SPARK3_H, SSD1306_WHITE);
        break;
      case PK_HEART:
        if (f < 0.65f)      oled.drawBitmap(x - 3, y - 3, HEART7, HEART7_W, HEART7_H, SSD1306_WHITE);
        else if (f < 0.88f) oled.drawBitmap(x - 2, y - 2, HEART5, HEART5_W, HEART5_H, SSD1306_WHITE);
        else                oled.drawBitmap(x - 1, y - 1, HEART3, HEART3_W, HEART3_H, SSD1306_WHITE);
        break;
    }
  }
}
