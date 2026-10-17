#pragma once
// ============================================================
// Event framework. Every animation "scene" (boot, reaction, message, effect, ...) is an EventDef:
// begin -> update (returns false when finished) -> draw, with cancel() for clean interruption.
// The director owns priorities and scheduling; events never call each other directly.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "gfx.h"
#include "anim_core.h"
#include "emotion.h"
#include "character.h"
#include "particles.h"
#include "effects.h"
#include "assets.h"

struct EventDef {
  const char *name;
  bool        interruptible;
  void (*begin)(uint32_t now);
  bool (*update)(uint32_t now);     // false = finished
  void (*draw)(uint32_t now);
  void (*cancel)(uint32_t now);     // called when interrupted; must leave the scene clean
};

extern const EventDef evdefBoot, evdefLove, evdefHappy, evdefSleep, evdefTransform, evdefGlitch,
                      evdefSpace, evdefMessage, evdefMorph, evdefPowerUp, evdefHeartPulse, evdefPeek, evdefGame;

// ---- shared scene helpers (events_common.cpp) ----
void sceneReset(uint32_t now);          // character/emotion/particles back to a clean idle state
void sceneDrawBase();                   // clear + character + particles (the standard frame)
void sceneCharBox(int &x0, int &y0, int &x1, int &y1);

// ---- sleep control (events_basic.cpp) ----
void sleepSetForever(bool forever);     // stay asleep until sleepWake()
void sleepWake();
bool sleepIsDeep();                     // eyes closed, safe to turn the display off

// ---- message request (events_message.cpp): the director picks, the event shows ----
void messageSetPending(const char *text, Emo emo, uint8_t icon);

// ---- game (game.cpp, only when ENABLE_BUTTON) ----
void gameSendPress(uint32_t now);       // short press = jump / start
void gameSendLong();                    // long press = quit
