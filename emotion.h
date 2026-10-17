#pragma once
// ============================================================
// Emotion engine: every expression is a set of target parameters.
// One FaceParams "current" value eases toward the target (no per-emotion loops).
// Blinking and gaze are independent, randomized sub-engines.
// ============================================================
#include <Arduino.h>

enum Emo : uint8_t {
  EMO_NEUTRAL, EMO_HAPPY, EMO_LOVE, EMO_EXCITED, EMO_CURIOUS, EMO_SHY, EMO_SURPRISED,
  EMO_CONFUSED, EMO_SLEEPY, EMO_SAD, EMO_ANNOYED,
  EMO_WINK, EMO_JOY, EMO_ASLEEP,            // extras used by reactions / sleep / boot
  EMO_COUNT
};

enum Fx : uint8_t {
  FX_NONE, FX_HEART, FX_HEARTS, FX_SPARKLE, FX_QUESTION, FX_EXCLAIM, FX_ZZZ, FX_DOTS, FX_TEAR, FX_ANGER
};

// All members are floats so the struct can be interpolated generically.
struct FaceParams {
  float eyeW, eyeH, eyeDX, eyeY;          // eye size / spacing / vertical offset (px at scale 1)
  float skew;                             // -1..1: right eye bigger (+) or smaller (-)
  float lidTop, lidTilt, lidBot;          // upper lid cover 0..1, tilt -1 (sad)..+1 (angry), lower lid lift 0..1
  float arc, arcDir;                      // closed-eye arc amount; +1 = ^ (joy), -1 = u (sleep)
  float pupil;                            // pupil size relative to eye
  float gazeX, gazeY;                     // default gaze -1..1
  float mouthW, mouthCurve, mouthOpen, mouthY;
  float sprout, earL, earR;               // -1 droop .. +1 perk
  float bob, paw;                         // breathing amplitude, paws raised
  float blush, heartEye, wink;
};

struct EmotionDef {
  const char *name;
  FaceParams  face;
  Fx          fx;
  uint16_t    blinkMin, blinkMax;   // ms between blinks
  uint16_t    gazeMin, gazeMax;     // ms between gaze shifts
  float       wander;               // how far the eyes roam (0..1)
  uint16_t    transMs;              // default transition time
  bool        pop;                  // overshoot easing for surprise-like changes
  float       blinkSlow;            // blink duration multiplier
};

extern const EmotionDef EMOTIONS[EMO_COUNT];

class EmotionState {
 public:
  void begin(uint32_t now, Emo e);
  void set(Emo e, uint32_t now, int transMs = -1);   // -1 = use the emotion's default
  void update(uint32_t now);

  const FaceParams &face() const { return cur_; }
  Emo   target() const           { return target_; }
  Fx    fx() const               { return EMOTIONS[target_].fx; }
  float fxStrength(uint32_t now) const;               // 0..1, fades in after a change
  float blink() const            { return blink_; }   // 0 open .. 1 closed
  float gazeX() const            { return gazeX_; }   // final, -1..1
  float gazeY() const            { return gazeY_; }

  void blinkNow(uint32_t now);
  void overrideGaze(float x, float y, uint32_t untilMs, uint32_t now);   // look at something for a while
  void clearGazeOverride()       { gazeOvUntil_ = 0; gazeOvActive_ = false; }
  void setBlinkEnabled(bool b)   { blinkEnabled_ = b; }

 private:
  void updateBlink(uint32_t now);
  void updateGaze(uint32_t now, float dt);
  void scheduleBlink(uint32_t now);
  void scheduleGaze(uint32_t now);

  FaceParams from_, to_, cur_;
  Emo      target_ = EMO_NEUTRAL;
  uint32_t tStart_ = 0, tDur_ = 1;
  bool     pop_ = false;
  uint32_t fxSince_ = 0;
  uint32_t lastUpdate_ = 0;

  // blink
  bool     blinkEnabled_ = true;
  float    blink_ = 0;
  uint32_t nextBlink_ = 0, blinkStart_ = 0, blinkDur_ = 0;
  bool     blinking_ = false, doubleBlink_ = false;

  // gaze
  float    gazeX_ = 0, gazeY_ = 0;       // final
  float    fromX_ = 0, fromY_ = 0, toX_ = 0, toY_ = 0, baseX_ = 0, baseY_ = 0;
  uint32_t saccStart_ = 0, saccDur_ = 160, nextGaze_ = 0;
  int8_t   lastRegion_ = -1;
  float    jitX_ = 0, jitY_ = 0, jitTX_ = 0, jitTY_ = 0;
  uint32_t nextJit_ = 0;
  bool     gazeOvActive_ = false;
  uint32_t gazeOvUntil_ = 0;
  float    ovX_ = 0, ovY_ = 0;
};

extern EmotionState emotion;
