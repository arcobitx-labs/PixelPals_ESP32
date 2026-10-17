#pragma once
// Reusable visual effects: starfield, planet, shooting star, energy ring, big heart, glitch.
#include <Arduino.h>

void starsInit();
void starsUpdate(float dt, float speedMul);
void starsDraw(float density, float warp, uint32_t now);   // density 0..1, warp 0..1 (streaks)
void drawPlanet(int cx, int cy, int r);
void shooterLaunch(uint32_t now);
void shooterDraw(uint32_t now);
void drawEnergyRing(int cx, int cy, uint32_t now, float radiusScale);
void drawBigHeart(int cx, int cy, float pulse01);          // 0 small .. 1 large
void glitchApply(uint32_t now, float intensity);           // intensity 0..1, post-process on the framebuffer
