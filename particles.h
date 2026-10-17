#pragma once
// Fixed-size particle pool (no heap). Dots, sparkles, rising hearts, and "seekers"
// that fly from a start point onto a target pixel (used for boot / reassembly).
#include <Arduino.h>

enum PKind : uint8_t { PK_NONE, PK_DOT, PK_SPARK, PK_HEART, PK_SEEK };

struct Particle {
  float   x, y, vx, vy;      // for PK_SEEK: x,y = start point
  int16_t tx, ty;            // PK_SEEK target
  int16_t age;               // ms; negative = waiting (delay)
  uint16_t life;             // ms
  uint8_t kind;
  uint8_t aux;               // flags / phase
};

class Particles {
 public:
  static const int N = 170;
  void clear();
  void update(uint32_t now);
  void draw();
  int  count() const;
  bool spawn(uint8_t kind, float x, float y, float vx, float vy, uint16_t life, uint8_t aux = 0);
  void burst(float x, float y, int n, float speed, uint16_t life, uint8_t kind = PK_DOT);
  // Particles start on a circle of radius r and fly outward (keeps the face clear).
  void ring(float x, float y, float r, int n, float speed, uint16_t life, uint8_t kind = PK_DOT);
  void seek(float sx, float sy, int tx, int ty, int delayMs, uint16_t durMs);
  void heart(float x, float y);
  // Turn every live dot into a seeker aimed at the given targets (reassembly).
  int  retargetAll(const int16_t *tx, const int16_t *ty, int nTargets, int maxDelayMs, uint16_t durMs);
  Particle p[N];
 private:
  uint32_t last_ = 0;
};

extern Particles particles;
