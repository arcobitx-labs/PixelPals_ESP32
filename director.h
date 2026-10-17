#pragma once
// ============================================================
// Director: owns the event state machine, priorities, cooldowns and the random scheduler.
// Priority rules: a higher priority may interrupt a lower one; equal or lower is rejected;
// BOOT cannot be interrupted. Idle micro-behaviours never interrupt anything.
// ============================================================
#include <Arduino.h>
#include "config.h"

enum EventId : uint8_t {
  EV_NONE, EV_BOOT, EV_LOVE, EV_HAPPY, EV_SLEEP, EV_TRANSFORM, EV_GLITCH, EV_SPACE, EV_MESSAGE,
  EV_MORPH, EV_POWERUP, EV_HEARTPULSE, EV_PEEK, EV_GAME, EV_COUNT
};
enum Priority : uint8_t { PRI_IDLE = 10, PRI_EFFECT = 40, PRI_MESSAGE = 60, PRI_USER = 80, PRI_GAME = 85, PRI_BOOT = 100 };

void      directorBegin(uint32_t now);
void      directorFrame(uint32_t now);          // update, draw, flush (call at the pacing rate)
bool      directorRequest(EventId id, uint8_t prio, uint32_t now);
EventId   directorCurrent();
uint8_t   directorFps();                        // desired frame rate for the next frame
void      directorAction(Action a, uint32_t now);   // user-level actions (button / console)
void      directorCycleEmotion(uint32_t now);   // debug helper
bool      directorDark();
void      directorDarkTick(uint32_t now);       // call instead of directorFrame while the display is off
