#pragma once
// ============================================================
// The PixelPals mascot: a helmet-ring head, cat ears, leaf sprout, chin plate, paws and foot.
// Drawn procedurally from FaceParams, so every emotion shares one code path.
// All coordinates are authored at scale 1 (about 54 px tall) and scaled by `scale`.
// ============================================================
#include <Arduino.h>
#include "emotion.h"
#include "anim_core.h"

enum CharStyle : uint8_t { STYLE_HELMET = 0, STYLE_CAT = 1, STYLE_BLOB = 2 };

class Character {
 public:
  // placement (animated by events)
  float   x = 64, y = 34, scale = 1.f;
  uint8_t style = STYLE_HELMET;
  float   breathRate = 1.f;   // 1 = normal, <1 slower (sleep)
  float   floatAmt = 0.f;     // 0..1 zero-g drifting (space)
  bool    crown = false;
  bool    showFx = true;      // emotion fx (hearts, ?, !, Zz ...)
  bool    showBody = true;    // paws + foot

  void reset();
  void update(uint32_t now);
  void kickBounce(uint32_t now, float amp, uint16_t periodMs = 420);
  void kickShake(uint32_t now, float amp, uint16_t durMs = 420);
  void wiggleSprout(uint32_t now);
  void twitchEar(uint32_t now, int side);
  void pulseScale(uint32_t now, float amp, uint16_t durMs);

  // Draw using the global emotion state (normal path).
  void draw(uint32_t now);
  // Draw with explicit face values (game, previews).
  void drawWith(const FaceParams &f, float blink, float gx, float gy, uint32_t now);
  void drawFx(Fx fx, float strength, uint32_t now);
  void bounds(int &x0, int &y0, int &x1, int &y1) const;
  int  centerX() const { return cx_; }
  int  centerY() const { return cy_; }
  float drawScale() const { return S_; }

 private:
  void drawEar(int side, float ear, bool outlineThin);
  void drawEye(int side, const FaceParams &f, float blink, float gx, float gy, uint32_t now);
  void drawMouth(const FaceParams &f);
  void drawSprout(float sprout, uint32_t now);
  int  PX(float v) const { return cx_ + roundi(v * S_); }
  int  PY(float v) const { return cy_ + roundi(v * S_); }

  int   cx_ = 64, cy_ = 34;
  float S_ = 1.f;
  float breathPhase_ = 0.f;
  uint32_t lastUpdate_ = 0;
  // transient layers
  uint32_t bounceStart_ = 0; float bounceAmp_ = 0; uint16_t bouncePeriod_ = 1;
  uint32_t shakeStart_ = 0;  float shakeAmp_ = 0;  uint16_t shakeDur_ = 1;
  uint32_t wiggleStart_ = 0; bool wiggle_ = false;
  uint32_t earStart_ = 0;    int earSide_ = 0;
  uint32_t pulseStart_ = 0;  float pulseAmp_ = 0;  uint16_t pulseDur_ = 1;
};

extern Character character;
