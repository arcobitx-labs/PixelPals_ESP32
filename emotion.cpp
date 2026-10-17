#include "emotion.h"
#include "anim_core.h"

EmotionState emotion;

static_assert(sizeof(FaceParams) % sizeof(float) == 0, "FaceParams must be all floats");
static const int FACE_N = sizeof(FaceParams) / sizeof(float);
static inline float       *arr(FaceParams &f)       { return reinterpret_cast<float *>(&f); }
static inline const float *arr(const FaceParams &f) { return reinterpret_cast<const float *>(&f); }

// Field order:
// eyeW,eyeH,eyeDX,eyeY, skew, lidTop,lidTilt,lidBot, arc,arcDir, pupil, gazeX,gazeY,
// mouthW,mouthCurve,mouthOpen,mouthY, sprout,earL,earR, bob,paw, blush,heartEye,wink
const EmotionDef EMOTIONS[EMO_COUNT] = {
 // name        face                                                                                              fx          blink       gaze        wand  trans pop  slow
 {"neutral",  {9,11,9.5f,-3,   0,   0,0,0,     0,1,  0.58f, 0,0,        5,0.25f,0,6.5f,   0.3f,0,0,       1.0f,0,    0,0,0}, FX_NONE,    2200,5600,  900,3200,  0.8f,  380,false,1.0f},
 {"happy",    {9.5f,11,9.5f,-3,0,   0,0,0.28f, 0,1,  0.62f,0,-0.05f,   6,1,0.0f,6.5f,    0.8f,0.4f,0.4f,  1.3f,0.2f, 0.25f,0,0},FX_HEART,   2000,5000,  900,2800,  0.6f,  380,false,1.0f},
 {"love",     {10,10,9.5f,-3,  0,   0,0,0,     0,1,  0.6f,  0,0,        5,0.9f,0,6.5f,    1.0f,0.5f,0.5f,  1.4f,0.3f, 0.7f,1,0},  FX_HEARTS,  2400,4800,  1500,3500, 0.25f, 450,false,1.0f},
 {"excited",  {10,14,9.5f,-3.5f,0,  0,0,0.05f,0,1,  0.72f,0,-0.1f,     6,1,0.6f,6.5f,    1.0f,0.8f,0.8f,  2.2f,1.0f, 0.2f,0,0}, FX_SPARKLE, 1500,3500,  500,1600,  0.9f,  260,true, 0.8f},
 {"curious",  {9,11,9.5f,-3,   0.8f,0,0,0,     0,1,  0.6f,  0.55f,-0.45f,5,0.1f,0,6.5f,  0.9f,0.0f,0.9f,  1.0f,0,    0,0,0},  FX_QUESTION,1800,4200,  600,1800,  0.9f,  420,false,1.0f},
 {"shy",      {8.5f,9.5f,9,-1.5f,0, 0.15f,-0.25f,0.1f,0,1, 0.5f,-0.6f,0.6f, 3,0.3f,0,7,    -0.3f,-0.3f,-0.3f,0.8f,-0.2f,1.0f,0,0},FX_NONE,    2000,4500,  1400,3200, 0.35f, 520,false,1.1f},
 {"surprised",{11,15,9.5f,-3.5f,0,  0,0,0,     0,1,  0.32f, 0,-0.1f,    4,0,0.7f,6.5f,    1.0f,1.0f,1.0f,  1.6f,0.6f, 0,0,0},   FX_EXCLAIM, 4000,8000,  1600,3200, 0.15f, 170,true, 1.0f},
 {"confused", {9,11,9.5f,-3,   -0.7f,0.05f,0.25f,0,0,1, 0.55f,-0.4f,-0.5f, 6,-0.35f,0,6.5f,0.0f,0.8f,-0.5f, 1.0f,0,    0,0,0},   FX_DOTS,    1800,4200,  500,1500,  0.8f,  380,false,1.0f},
 {"sleepy",   {9.5f,11,9.5f,-2.5f,0, 0.5f,0,0.05f,0,1, 0.55f,  0,0.6f,       4,0.1f,0,7,     -0.6f,-0.4f,-0.4f,0.55f,-0.2f,0,0,0}, FX_ZZZ,     1400,3200,  2600,5200, 0.25f, 900,false,1.9f},
 {"sad",      {9,11,9.5f,-3,   0,   0.12f,-0.85f,0,0,1, 0.7f,  0,0.7f,       5,-0.85f,0,7.5f,-1.0f,-1.0f,-1.0f,0.5f,-0.4f,0,0,0}, FX_TEAR,    2600,5800,  2200,4600, 0.25f, 600,false,1.3f},
 {"annoyed",  {9,10,9.5f,-3,   0,   0.38f,0.85f,0,0,1,  0.45f, 0,0.1f,      6,-0.2f,0,7,    -0.3f,-0.7f,-0.7f,0.7f,0,   0,0,0},   FX_ANGER,   3000,6500,  1500,3600, 0.3f,  300,false,1.0f},
 {"wink",     {9.5f,11,9.5f,-3,0,   0,0,0.15f, 0,1,  0.6f,  0.2f,0,        6,1,0.4f,6.5f,   0.9f,0.5f,0.8f,  1.3f,0.3f, 0.3f,0,1.0f},FX_SPARKLE, 3000,6000,  1500,3000, 0.3f,  300,true, 1.0f},
 {"joy",      {9,10,9.5f,-3,   0,   0,0,0,     1,1,  0.58f, 0,0,        6,1,0.5f,6.5f,   1.0f,0.7f,0.7f,  2.0f,0.8f, 0.5f,0,0},  FX_SPARKLE, 4000,8000,  1500,3000, 0.1f,  240,true, 1.0f},
 {"asleep",   {9,10,9.5f,-2.5f,0,   0,0,0,     1,-1, 0.6f,  0,0,        4,0.15f,0,7,    -0.8f,-0.5f,-0.5f,0.45f,-0.3f,0.2f,0,0}, FX_ZZZ,     6000,9000,  6000,9000, 0.0f,  900,false,1.0f},
};

// ------------------------------------------------------------------ core
void EmotionState::begin(uint32_t now, Emo e) {
  target_ = e;
  from_ = to_ = cur_ = EMOTIONS[e].face;
  tStart_ = now; tDur_ = 1; pop_ = false;
  fxSince_ = now; lastUpdate_ = now;
  blink_ = 0; blinking_ = false; doubleBlink_ = false;
  gazeX_ = gazeY_ = fromX_ = fromY_ = toX_ = toY_ = jitX_ = jitY_ = 0;
  lastRegion_ = -1;
  clearGazeOverride();
  scheduleBlink(now);
  scheduleGaze(now);
  nextJit_ = now + 100;
}

void EmotionState::set(Emo e, uint32_t now, int transMs) {
  if (e >= EMO_COUNT) return;
  const EmotionDef &d = EMOTIONS[e];
  from_ = cur_;
  to_ = d.face;
  target_ = e;
  tStart_ = now;
  tDur_ = transMs >= 0 ? (uint32_t)transMs : d.transMs;
  if (tDur_ < 1) tDur_ = 1;
  pop_ = d.pop;
  fxSince_ = now + (uint32_t)(tDur_ * 0.55f);
  scheduleBlink(now);   // re-roll blink timing for the new mood
  scheduleGaze(now);
}

float EmotionState::fxStrength(uint32_t now) const {
  if (!timeReached(now, fxSince_)) return 0.f;
  return clamp01((float)(now - fxSince_) / 260.f);
}

void EmotionState::update(uint32_t now) {
  float dt = (now - lastUpdate_) * 0.001f;
  if (dt > 0.1f) dt = 0.1f;
  lastUpdate_ = now;

  // --- interpolate parameters with easing ---
  float p = progress(now, tStart_, tDur_);
  float e = pop_ ? easeOutBack(p) : easeInOutCubic(p);
  const float *a = arr(from_), *b = arr(to_);
  float *c = arr(cur_);
  for (int i = 0; i < FACE_N; i++) c[i] = lerpf(a[i], b[i], e);
  // arc direction is discrete: keep the shape of whichever side owns the arc (avoids a flip through "flat")
  cur_.arcDir = (to_.arc >= from_.arc) ? to_.arcDir : from_.arcDir;

  updateBlink(now);
  updateGaze(now, dt);
}

// ------------------------------------------------------------------ blink
void EmotionState::scheduleBlink(uint32_t now) {
  const EmotionDef &d = EMOTIONS[target_];
  nextBlink_ = now + rndMs(d.blinkMin, d.blinkMax);
}

void EmotionState::blinkNow(uint32_t now) {
  nextBlink_ = now; blinking_ = false;
}

void EmotionState::updateBlink(uint32_t now) {
  const EmotionDef &d = EMOTIONS[target_];
  if (!blinking_ && blinkEnabled_ && timeReached(now, nextBlink_)) {
    blinking_ = true;
    blinkStart_ = now;
    blinkDur_ = (uint32_t)(rndMs(120, 190) * d.blinkSlow);
    doubleBlink_ = chance(target_ == EMO_SLEEPY ? 25 : 9);
  }
  if (blinking_) {
    float t = progress(now, blinkStart_, blinkDur_);
    blink_ = t < 0.4f ? easeInQuad(t / 0.4f) : 1.f - easeOutCubic((t - 0.4f) / 0.6f);
    if (t >= 1.f) {
      blinking_ = false; blink_ = 0;
      if (doubleBlink_) { doubleBlink_ = false; nextBlink_ = now + 130; }
      else scheduleBlink(now);
    }
  } else blink_ = 0;
  // eyes that are already closed / hearts do not blink
  if (cur_.arc > 0.6f || cur_.heartEye > 0.6f) blink_ = 0;
}

// ------------------------------------------------------------------ gaze
void EmotionState::scheduleGaze(uint32_t now) {
  const EmotionDef &d = EMOTIONS[target_];
  nextGaze_ = now + rndMs(d.gazeMin, d.gazeMax);
}

void EmotionState::overrideGaze(float x, float y, uint32_t untilMs, uint32_t now) {
  gazeOvActive_ = true; gazeOvUntil_ = now + untilMs; ovX_ = x; ovY_ = y;
}

void EmotionState::updateGaze(uint32_t now, float dt) {
  const EmotionDef &d = EMOTIONS[target_];

  if (gazeOvActive_ && timeReached(now, gazeOvUntil_)) { gazeOvActive_ = false; scheduleGaze(now); }

  if (!gazeOvActive_ && timeReached(now, nextGaze_)) {
    // pick a new gaze region that differs from the last one (no repetitive patterns)
    int8_t r;
    int tries = 0;
    do { r = (int8_t)rndi(0, 9); tries++; } while (r == lastRegion_ && tries < 6);
    if (r == 4 && chance(40)) r = (int8_t)rndi(0, 9);          // centre is common but not dominant
    lastRegion_ = r;
    float gx = (float)((r % 3) - 1) * rndf(0.55f, 1.0f);
    float gy = (float)((r / 3) - 1) * rndf(0.45f, 0.8f);
    fromX_ = baseX_; fromY_ = baseY_;
    toX_ = gx * d.wander; toY_ = gy * d.wander;   // wander scales the destination, so motion stays continuous
    saccStart_ = now;
    saccDur_ = rndMs(110, 230);
    scheduleGaze(now);
  }

  if (gazeOvActive_) {
    // smooth chase of the override target
    float k = clamp01(dt * 12.f);
    baseX_ += (ovX_ - baseX_) * k;
    baseY_ += (ovY_ - baseY_) * k;
    fromX_ = toX_ = baseX_; fromY_ = toY_ = baseY_;
  } else {
    float t = easeOutCubic(progress(now, saccStart_, saccDur_));
    baseX_ = lerpf(fromX_, toX_, t);
    baseY_ = lerpf(fromY_, toY_, t);
  }

  // micro-movements: tiny, smoothed jitter retargeted every ~100-300 ms
  if (timeReached(now, nextJit_)) {
    jitTX_ = rndf(-0.07f, 0.07f); jitTY_ = rndf(-0.05f, 0.05f);
    nextJit_ = now + rndMs(100, 300);
  }
  float kj = clamp01(dt * 9.f);
  jitX_ += (jitTX_ - jitX_) * kj;
  jitY_ += (jitTY_ - jitY_) * kj;

  float ov = gazeOvActive_ ? 1.f : 0.f;
  gazeX_ = clampf(cur_.gazeX + baseX_ + jitX_ * (1.f - ov * 0.5f), -1.f, 1.f);
  gazeY_ = clampf(cur_.gazeY + baseY_ + jitY_ * (1.f - ov * 0.5f), -1.f, 1.f);
}
