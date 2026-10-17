#pragma once
// ============================================================
// Reusable animation primitives: math helpers, easing, timers, random.
// Header-only, no dynamic allocation, time is always passed in.
// ============================================================
#include <Arduino.h>
#include <math.h>

typedef float (*EaseFn)(float);

inline float clamp01(float t)                 { return t < 0.f ? 0.f : (t > 1.f ? 1.f : t); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int   iclamp(int v, int lo, int hi)    { return v < lo ? lo : (v > hi ? hi : v); }
inline int   imin(int a, int b)               { return a < b ? a : b; }
inline int   imax(int a, int b)               { return a > b ? a : b; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline int   roundi(float v)                  { return (int)lroundf(v); }

// 0..1 progress of a time window. Wrap-safe for normal use.
inline float progress(uint32_t now, uint32_t start, uint32_t durMs) {
  if ((int32_t)(now - start) <= 0) return 0.f;
  uint32_t dt = now - start;
  return dt >= durMs ? 1.f : (float)dt / (float)durMs;
}
inline bool timeReached(uint32_t now, uint32_t at) { return (int32_t)(now - at) >= 0; }

// ---- easing ----
inline float easeLinear(float t)     { return t; }
inline float easeInQuad(float t)     { return t * t; }
inline float easeOutQuad(float t)    { return 1.f - (1.f - t) * (1.f - t); }
inline float easeOutCubic(float t)   { float u = 1.f - t; return 1.f - u * u * u; }
inline float easeInOutCubic(float t) { return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) / 2.f; }
inline float smoothstep(float t)     { t = clamp01(t); return t * t * (3.f - 2.f * t); }
// overshoot then settle ("pop" / "snap into place")
inline float easeOutBack(float t) {
  const float c1 = 1.70158f, c3 = c1 + 1.f;
  float u = t - 1.f;
  return 1.f + c3 * u * u * u + c1 * u * u;
}
// anticipation then accelerate (used for bubble exit)
inline float easeInBack(float t) {
  const float c1 = 1.70158f, c3 = c1 + 1.f;
  return c3 * t * t * t - c1 * t * t;
}
inline float tri01(float t) { return t < 0.5f ? t * 2.f : (1.f - t) * 2.f; }   // 0 -> 1 -> 0
inline float sine01(uint32_t now, uint32_t periodMs, float phase = 0.f) {       // 0..1 oscillation
  return 0.5f + 0.5f * sinf(phase + (float)(now % periodMs) * (6.2831853f / (float)periodMs));
}

// ---- random helpers (random() is seeded in setup) ----
inline float    rndf(float lo, float hi)        { return lo + (hi - lo) * ((float)random(0, 10001) / 10000.f); }
inline int32_t  rndi(int32_t lo, int32_t hi)    { return (int32_t)random(lo, hi); }   // [lo, hi)
inline uint32_t rndMs(uint32_t lo, uint32_t hi) { return lo + (uint32_t)random(0, (long)(hi - lo) + 1); }
inline bool     chance(int pct)                 { return random(0, 100) < pct; }

// Cheap spatial hash for dissolve / reveal masks (0..255).
inline uint8_t hash8(int x, int y) {
  uint32_t h = (uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u;
  h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
  return (uint8_t)(h & 0xFF);
}

// One-shot / repeating timer helper.
struct Timer {
  uint32_t at = 0; bool armed = false;
  void set(uint32_t now, uint32_t ms) { at = now + ms; armed = true; }
  void clear()                        { armed = false; }
  bool due(uint32_t now) const        { return armed && timeReached(now, at); }
};
