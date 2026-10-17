// Boot, Love reaction, Happy reaction, Sleep/wake.
#include "events.h"

// =============================================================== BOOT
// pixel flickers on -> grows to an orb -> bursts into particles that fly onto the mascot's
// pixels -> the mascot is revealed pixel by pixel -> eyes open.
namespace boot {
const uint32_t T_GROW = 450, T_BURST = 1000, T_REVEAL = 1750, T_REVEAL_DUR = 700, T_EYES = 2550, T_END = 3500;
uint32_t t0; bool eyesDone;

void begin(uint32_t now) {
  t0 = now; eyesDone = false;
  sceneReset(now);
  emotion.begin(now, EMO_ASLEEP);
  emotion.setBlinkEnabled(false);
  character.showFx = false;
  // Render the closed-eye mascot once, sample 150 of its pixels as particle targets.
  int16_t tx[150], ty[150];
  oled.clearDisplay();
  character.update(now);
  character.draw(now);
  int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1);
  int n = gfxSampleWhite(x0, y0, x1, y1, tx, ty, 150);
  oled.clearDisplay();
  for (int i = 0; i < n; i++)
    particles.seek(64 + rndi(-2, 3), 34 + rndi(-2, 3), tx[i], ty[i], (int)(T_BURST + rndi(0, 300)), 650);
}

bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (!eyesDone && t >= T_EYES) { eyesDone = true; emotion.setBlinkEnabled(true); character.showFx = true; emotion.set(EMO_NEUTRAL, now, 700); }
  if (t >= T_END) return false;
  return true;
}

void draw(uint32_t now) {
  uint32_t t = now - t0;
  oled.clearDisplay();
  const int CX = 64, CY = 34;
  if (t < T_GROW) {
    bool on = (t >= 100 && t < 190) || t >= 260;
    if (on) oled.drawPixel(CX, CY, SSD1306_WHITE);
  } else if (t < T_BURST) {
    float r = 1.f + 6.f * easeOutBack(progress(t, T_GROW, T_BURST - T_GROW));
    oled.fillCircle(CX, CY, (int)r, SSD1306_WHITE);
  } else {
    float orb = 7.f * (1.f - easeOutCubic(progress(t, T_BURST, 300)));
    if (orb >= 1.f) oled.fillCircle(CX, CY, (int)orb, SSD1306_WHITE);
  }
  if (t >= T_REVEAL) {
    float p = progress(t, T_REVEAL, T_REVEAL_DUR);
    character.draw(now);
    if (p < 1.f) { int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1); gfxMask(x0, y0, x1, y1, (uint8_t)(p * 255.f), 255, nullptr); }
  }
  particles.draw();
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefBoot = {"boot", false, boot::begin, boot::update, boot::draw, boot::cancel};

// =============================================================== LOVE
namespace love {
uint32_t t0, nextHeart; bool d1, d2, d3;
void begin(uint32_t now) {
  t0 = now; d1 = d2 = d3 = false; nextHeart = now + 500;
  emotion.set(EMO_LOVE, now);
  character.kickBounce(now, 3.f, 380);
  character.wiggleSprout(now + 200);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (t < 2500 && timeReached(now, nextHeart)) {
    particles.heart(character.centerX() + rndf(-14, 14), character.centerY() - 20);
    nextHeart = now + rndMs(220, 380);
  }
  if (!d1 && t > 900)  { d1 = true; character.pulseScale(now, 0.08f, 560); }
  if (!d2 && t > 1900) { d2 = true; character.kickBounce(now, 2.5f, 360); character.wiggleSprout(now); }
  if (!d3 && t > 2600) { d3 = true; emotion.set(EMO_NEUTRAL, now, 650); }
  return t < 3300;
}
void draw(uint32_t) { sceneDrawBase(); }
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefLove = {"love", true, love::begin, love::update, love::draw, love::cancel};

// =============================================================== HAPPY
namespace happy {
uint32_t t0; bool d1, d2, d3, d4;
void begin(uint32_t now) {
  t0 = now; d1 = d2 = d3 = d4 = false;
  emotion.set(EMO_JOY, now);
  character.kickBounce(now, 5.f, 400);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (!d1 && t > 160) { d1 = true; particles.ring(character.centerX(), character.centerY() - 2, 24.f, 12, 34.f, 800, PK_SPARK); }
  if (!d2 && t > 540) { d2 = true; character.kickBounce(now, 3.5f, 380); character.wiggleSprout(now); }
  if (!d3 && t > 1700) { d3 = true; emotion.set(EMO_HAPPY, now, 450); }
  if (!d4 && t > 2300) { d4 = true; emotion.set(EMO_NEUTRAL, now, 650); }
  return t < 2900;
}
void draw(uint32_t) { sceneDrawBase(); }
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefHappy = {"happy", true, happy::begin, happy::update, happy::draw, happy::cancel};

// =============================================================== SLEEP / WAKE
namespace slp {
uint32_t t0, wakeAt, holdUntil; bool forever = false, wakeReq = false, waking = false, deep = false;
bool s1, s2, w1, w2;
void begin(uint32_t now) {
  t0 = now; wakeReq = false; waking = false; deep = false; s1 = s2 = w1 = w2 = false;
  holdUntil = 0;
  emotion.set(EMO_SLEEPY, now, 1500);
  character.breathRate = 0.75f;
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (!waking) {
    if (!s1 && t > 2200) { s1 = true; emotion.set(EMO_ASLEEP, now, 1000); character.breathRate = 0.5f; }
    if (!s2 && t > 3300) { s2 = true; deep = true; holdUntil = now + rndMs(4500, 7500); }
    bool wake = forever ? wakeReq : (s2 && timeReached(now, holdUntil));
    if (wake && s2) {
      waking = true; wakeAt = now; deep = false; forever = false;
      emotion.set(EMO_SLEEPY, now, 600); character.breathRate = 1.f; character.wiggleSprout(now + 300);
    }
    return true;
  }
  uint32_t w = now - wakeAt;
  if (!w1 && w > 700)  { w1 = true; emotion.set(EMO_HAPPY, now, 300); character.kickBounce(now, 3.f, 380); }
  if (!w2 && w > 1700) { w2 = true; emotion.set(EMO_NEUTRAL, now, 600); }
  return w < 2400;
}
void draw(uint32_t) { sceneDrawBase(); }
void cancel(uint32_t now) { deep = false; sceneReset(now); }
}
const EventDef evdefSleep = {"sleep", true, slp::begin, slp::update, slp::draw, slp::cancel};

void sleepSetForever(bool f) { slp::forever = f; }
void sleepWake()             { slp::wakeReq = true; }
bool sleepIsDeep()           { return slp::deep; }
