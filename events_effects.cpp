// Signature effects: pixel transformation, cyberpunk glitch, space sequence.
#include "events.h"

// =============================================================== TRANSFORM
// The mascot dissolves into particles (pixel by pixel), they drift, then reassemble into the
// mascot wearing a different expression.
namespace xf {
const uint32_t T_DISS = 1300, T_REASM = 1750, T_REVEAL = 2300, T_REVEAL_DUR = 750, T_NEUTRAL = 4300, T_END = 4900;
uint32_t t0; Emo target; bool reassembling; uint8_t lastThr;

void spawnCb(int x, int y) {
  static uint8_t k = 0;
  if ((k++ & 1) == 0)
    particles.spawn(PK_DOT, x, y, rndf(-24, 24) + (x - 64) * 0.7f, rndf(-28, 8) + (y - 34) * 0.4f, 2500, 0);
}

void begin(uint32_t now) {
  t0 = now; reassembling = false; lastThr = 255;
  static const Emo pool[] = {EMO_HAPPY, EMO_CURIOUS, EMO_EXCITED, EMO_SURPRISED, EMO_WINK, EMO_LOVE};
  do { target = pool[rndi(0, 6)]; } while (target == emotion.target());
  character.kickShake(now, 1.3f, 900);
  particles.clear();
}

bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (!reassembling && t >= T_REASM) {
    reassembling = true;
    emotion.set(target, now, 0);                  // snap to the new face (it is hidden behind the mask)
    emotion.update(now);
    int16_t tx[150], ty[150];
    oled.clearDisplay();
    character.draw(now);
    int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1);
    int n = gfxSampleWhite(x0, y0, x1, y1, tx, ty, 150);
    particles.retargetAll(tx, ty, n, 450, 750);
    emotion.set(target, now, 0);
  }
  if (reassembling && t >= T_NEUTRAL && emotion.target() == target) emotion.set(EMO_NEUTRAL, now, 650);
  if (reassembling && t >= T_REVEAL + T_REVEAL_DUR && t < T_REVEAL + T_REVEAL_DUR + 60) character.kickBounce(now, 3.f, 380);
  return t < T_END;
}

void draw(uint32_t now) {
  uint32_t t = now - t0;
  oled.clearDisplay();
  if (t < T_DISS) {
    float p = easeInQuad((float)t / T_DISS);
    uint8_t thr = (uint8_t)((1.f - p) * 255.f);
    character.draw(now);
    int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1);
    gfxMask(x0, y0, x1, y1, thr, lastThr, spawnCb);
    lastThr = thr;
  } else if (t >= T_REVEAL) {
    float p = progress(t, T_REVEAL, T_REVEAL_DUR);
    character.draw(now);
    if (p < 1.f) { int x0, y0, x1, y1; character.bounds(x0, y0, x1, y1); gfxMask(x0, y0, x1, y1, (uint8_t)(p * 255.f), 255, nullptr); }
  }
  particles.draw();
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefTransform = {"transform", true, xf::begin, xf::update, xf::draw, xf::cancel};

// =============================================================== GLITCH
// Scanning eyes, horizontal tearing, scan bar, then a clean recovery.
namespace gl {
const uint32_t T_END = 2000;
uint32_t t0; float k; bool flash1, flash2;
void begin(uint32_t now) {
  t0 = now; k = 0; flash1 = flash2 = false;
  emotion.set(EMO_NEUTRAL, now, 200);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  if (t < 400)        k = 0.8f * (t / 400.f);
  else if (t < 1450)  { float s = sinf(t * 0.022f); k = 0.55f + 0.45f * s * s; }
  else                k = 0.9f * (1.f - progress(t, 1450, 450));
  if (t >= 1900) k = 0;
  // scanning eyes sweep left -> right -> back
  if (t < 1300) { float u = (float)t / 1300.f; emotion.overrideGaze(sinf(u * 6.2831853f * 1.5f), (u > 0.5f ? 0.2f : -0.1f), 120, now); }
  else emotion.clearGazeOverride();
  if (t >= 1450 && !flash2) { flash2 = true; character.kickShake(now, 1.0f, 250); }
  return t < T_END;
}
void draw(uint32_t now) {
  uint32_t t = now - t0;
  sceneDrawBase();
  glitchApply(now, k);
  if (!flash1 && t > 1000) { flash1 = true; gfxInvertAll(); }                // one-frame invert flicker
  if (t >= 1450 && t < 1500) gfxInvertAll();
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefGlitch = {"glitch", true, gl::begin, gl::update, gl::draw, gl::cancel};

// =============================================================== SPACE
// Stars drift in, the mascot floats, a ringed planet slides in, a shooting star passes,
// warp speed, then everything eases back to the normal view.
namespace sp {
const uint32_t T_ENTER = 1100, T_PLANET = 800, T_SHOOT = 2600, T_WARP = 5300, T_EXIT = 6600, T_END = 7700;
uint32_t t0, last; float density, warp, speed, planetX; bool shot, surprised, excited, neut;
void begin(uint32_t now) {
  t0 = last = now; density = warp = 0; speed = 1; planetX = 150; shot = surprised = excited = neut = false;
  starsInit();
  emotion.set(EMO_CURIOUS, now, 500);
}
bool update(uint32_t now) {
  uint32_t t = now - t0;
  float dt = (now - last) * 0.001f; last = now;
  // character pose
  if (t < T_ENTER) {
    float p = easeInOutCubic((float)t / T_ENTER);
    character.x = lerpf(64, 44, p); character.y = lerpf(34, 37, p); character.scale = lerpf(1.f, 0.8f, p); character.floatAmt = p;
    density = easeOutCubic((float)t / T_ENTER);
  } else if (t < T_EXIT) {
    float w = progress(t, T_WARP, 900);
    character.x = lerpf(44, 52, w); character.y = 37; character.scale = 0.8f; character.floatAmt = 1.f;
  } else {
    float p = easeInOutCubic(progress(t, T_EXIT, T_END - T_EXIT - 100));
    character.x = lerpf(52, 64, p); character.y = lerpf(37, 34, p); character.scale = lerpf(0.8f, 1.f, p); character.floatAmt = 1.f - p;
    density = 1.f - easeInQuad(progress(t, T_EXIT, 900));
  }
  // planet
  if (t >= T_PLANET && t < T_WARP + 400) planetX = lerpf(150, 99, easeOutCubic(progress(t, T_PLANET, 1300)));
  else if (t >= T_WARP + 400) planetX = lerpf(99, 160, easeInQuad(progress(t, T_WARP + 400, 800)));
  // shooting star + reaction
  if (!shot && t > T_SHOOT) { shot = true; shooterLaunch(now); }
  if (shot && t > T_SHOOT && t < T_SHOOT + 800) emotion.overrideGaze(-0.6f + (t - T_SHOOT) * 0.0014f, -0.5f, 120, now);
  else if (t > T_SHOOT + 800) emotion.clearGazeOverride();
  if (!surprised && t > T_SHOOT + 250) { surprised = true; emotion.set(EMO_SURPRISED, now); }
  if (!excited && t > T_SHOOT + 1100)  { excited = true;  emotion.set(EMO_EXCITED, now); }
  // warp
  if (t >= T_WARP && t < T_EXIT) {
    float p = progress(t, T_WARP, 900);
    warp = easeInQuad(p); speed = 1.f + 20.f * easeInQuad(p);
  } else if (t >= T_EXIT) {
    float p = progress(t, T_EXIT, 700);
    warp = 1.f - easeOutCubic(p); speed = 1.f + 20.f * (1.f - p);
  }
  if (!neut && t > 7000) { neut = true; emotion.set(EMO_HAPPY, now, 500); }
  starsUpdate(dt, speed);
  return t < T_END;
}
void draw(uint32_t now) {
  oled.clearDisplay();
  starsDraw(density, warp, now);
  if (planetX < 140) drawPlanet(roundi(planetX), 17, 8);
  shooterDraw(now);
  character.draw(now);
  particles.draw();
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefSpace = {"space", true, sp::begin, sp::update, sp::draw, sp::cancel};
