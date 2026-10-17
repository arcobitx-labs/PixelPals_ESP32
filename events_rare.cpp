// Rare surprises (long cooldown): Morph, Power-up (energy ring), Heart pulse, Peek-a-boo.
#include "events.h"

// =============================================================== MORPH
namespace mo {
uint32_t t0, puffAt; bool p1, p2, p3, p4;
void puff(uint32_t now) {
  puffAt = now;
  particles.ring(character.centerX(), character.centerY(), 22.f, 22, 60.f, 550, PK_DOT);
}
void begin(uint32_t now) {
  t0 = now; p1 = p2 = p3 = p4 = false; puffAt = 0;
  emotion.set(EMO_EXCITED, now, 300);
  character.kickShake(now, 1.5f, 600);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (!p1 && t > 650)  { p1 = true; puff(now); character.style = STYLE_CAT;    emotion.set(EMO_CURIOUS, now); }
  if (!p2 && t > 1600) { p2 = true; puff(now); character.style = STYLE_BLOB;   emotion.set(EMO_SURPRISED, now); character.kickBounce(now, 3.f, 360); }
  if (!p3 && t > 2550) { p3 = true; puff(now); character.style = STYLE_HELMET; emotion.set(EMO_JOY, now); character.crown = true; character.kickBounce(now, 4.f, 400); }
  if (!p4 && t > 3900) { p4 = true; character.crown = false; emotion.set(EMO_NEUTRAL, now, 650); }
  if (p3 && t < 3800 && (t / 120) % 3 == 0 && chance(30)) particles.spawn(PK_SPARK, character.centerX() + rndf(-26, 26), character.centerY() + rndf(-24, 8), 0, -8, 500, 0);
  return t < 4500;
}
void draw(uint32_t now) {
  sceneDrawBase();
  if (puffAt && now - puffAt < 320) {
    float p = (float)(now - puffAt) / 320.f;
    oled.drawCircle(character.centerX(), character.centerY(), 8 + roundi(30 * easeOutCubic(p)), SSD1306_WHITE);
  }
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefMorph = {"morph", true, mo::begin, mo::update, mo::draw, mo::cancel};

// =============================================================== POWER-UP (energy ring)
namespace pu {
uint32_t t0; bool r1, r2, r3; float ringScale;
void begin(uint32_t now) {
  t0 = now; r1 = r2 = r3 = false; ringScale = 0;
  emotion.set(EMO_EXCITED, now, 250);
  character.kickShake(now, 1.2f, 900);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (t < 2300) ringScale = 0.5f + 0.5f * easeOutCubic(progress(t, 300, 900));
  else          ringScale = 1.f + 0.9f * easeInQuad(progress(t, 2300, 450));
  if (!r1 && t > 1300) { r1 = true; character.pulseScale(now, 0.07f, 600); }
  if (!r2 && t > 2300) { r2 = true; particles.ring(character.centerX(), character.centerY(), 26.f, 22, 80.f, 800, PK_SPARK); emotion.set(EMO_JOY, now); character.kickBounce(now, 5.f, 400); }
  if (!r3 && t > 3300) { r3 = true; emotion.set(EMO_NEUTRAL, now, 650); }
  return t < 3900;
}
void draw(uint32_t now) {
  uint32_t t = now - t0;
  sceneDrawBase();
  if (t > 300 && t < 2750) drawEnergyRing(character.centerX(), character.centerY(), now, ringScale);
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefPowerUp = {"powerup", true, pu::begin, pu::update, pu::draw, pu::cancel};

// =============================================================== HEART PULSE
namespace hp {
const uint32_t T_DISS = 700, T_HEART_END = 3100, T_REVEAL = 3100, T_END = 4300;
uint32_t t0; uint8_t lastThr; bool back;
void spawnCb(int x, int y) {
  static uint8_t k = 0;
  if ((k++ % 3) == 0) particles.spawn(PK_DOT, x, y, (64 - x) * 1.5f, (32 - y) * 1.5f, 650, 0);
}
void begin(uint32_t now) {
  t0 = now; lastThr = 255; back = false;
  emotion.set(EMO_LOVE, now, 300);
  particles.clear();
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (t > 900 && t < 2900 && chance(7)) particles.spawn(PK_SPARK, 64 + rndf(-30, 30), 32 + rndf(-22, 22), 0, -6, 600, 0);
  if (!back && t >= T_REVEAL) { back = true; character.kickBounce(now, 3.f, 380); }
  if (back && t > 3800 && emotion.target() == EMO_LOVE) emotion.set(EMO_NEUTRAL, now, 650);
  return t < T_END;
}
void draw(uint32_t now) {
  uint32_t t = now - t0;
  oled.clearDisplay();
  if (t < T_DISS) {
    uint8_t thr = (uint8_t)((1.f - easeInQuad((float)t / T_DISS)) * 255.f);
    character.draw(now);
    int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1);
    gfxMask(x0, y0, x1, y1, thr, lastThr, spawnCb); lastThr = thr;
  }
  if (t >= 500 && t < T_HEART_END + 250) {
    // heartbeat: lub-dub every 900 ms
    uint32_t ph = (t - 500) % 900;
    float pulse = ph < 140 ? ph / 140.f : (ph < 280 ? 1.f - (ph - 140) / 280.f : (ph < 420 ? 0.5f + 0.5f * ((ph - 280) / 140.f) : 0.2f));
    float appear = easeOutBack(progress(t, 500, 400));
    if (t > T_HEART_END - 100) pulse *= 1.f - progress(t, T_HEART_END - 100, 350);
    if (appear > 0.3f) drawBigHeart(64, 32, pulse);
  }
  if (t >= T_REVEAL) {
    float p = progress(t, T_REVEAL, 700);
    character.draw(now);
    if (p < 1.f) { int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1); gfxMask(x0, y0, x1, y1, (uint8_t)(p * 255.f), 255, nullptr); }
  }
  particles.draw();
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefHeartPulse = {"heartpulse", true, hp::begin, hp::update, hp::draw, hp::cancel};

// =============================================================== PEEK-A-BOO
namespace pk {
uint32_t t0; bool g1, g2, g3, g4, g5;
void begin(uint32_t now) {
  t0 = now; g1 = g2 = g3 = g4 = g5 = false;
  emotion.set(EMO_CURIOUS, now, 300);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  float y;
  if (t < 550)       y = lerpf(34, 80, easeInOutCubic(progress(t, 0, 550)));           // duck down
  else if (t < 1500) y = 80;                                                            // hiding
  else if (t < 2650) y = lerpf(80, 63, easeOutCubic(progress(t, 1500, 700)));          // eyes peek over the edge
  else if (t < 3200) y = lerpf(63, 34, easeOutBack(progress(t, 2650, 550)));           // pop up
  else y = 34;
  character.y = y;
  if (!g1 && t > 900)  { g1 = true; emotion.set(EMO_CURIOUS, now); }
  if (t > 1700 && t < 2300) emotion.overrideGaze(t < 2000 ? -0.9f : 0.9f, 0.f, 150, now);
  else if (t >= 2300 && t < 2650) emotion.overrideGaze(0.f, -0.2f, 150, now);
  else if (t >= 2650) emotion.clearGazeOverride();
  if (!g2 && t > 2700) { g2 = true; emotion.set(EMO_JOY, now); character.kickBounce(now, 4.f, 400); particles.ring(64, 32, 24.f, 14, 48.f, 700, PK_SPARK); }
  if (!g3 && t > 3800) { g3 = true; emotion.set(EMO_NEUTRAL, now, 650); }
  return t < 4400;
}
void draw(uint32_t) { sceneDrawBase(); }
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefPeek = {"peek", true, pk::begin, pk::update, pk::draw, pk::cancel};
