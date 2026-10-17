#include "director.h"
#include "events.h"
#include "messages.h"
#include "input.h"
#include "power.h"

static const EventDef *const DEFS[EV_COUNT] = {
  nullptr, &evdefBoot, &evdefLove, &evdefHappy, &evdefSleep, &evdefTransform, &evdefGlitch,
  &evdefSpace, &evdefMessage, &evdefMorph, &evdefPowerUp, &evdefHeartPulse, &evdefPeek, &evdefGame
};

static EventId  cur = EV_NONE;
static uint8_t  curPrio = 0;
static Timer    nextMessage, nextEffect, nextMicro;
static uint32_t rareReadyAt = 0, microRestoreAt = 0, modeLabelUntil = 0;
static EventId  lastEffect = EV_NONE, lastRare = EV_NONE;
static int8_t   lastMicro = -1;
static uint8_t  mode = 0;                 // 0 pet, 1 cyber, 2 space, 3 arcade
static bool     goingDark = false;
static bool     reactToggle = false;
static int      emoCycle = 0;
static const char *MODE_NAMES[4] = {"PET", "CYBER", "SPACE", "ARCADE"};

// ------------------------------------------------------------------ helpers
static void restoreIdle(uint32_t now) {
  character.reset();
  emotion.clearGazeOverride();
  emotion.setBlinkEnabled(true);
  if (emotion.target() != EMO_NEUTRAL) emotion.set(EMO_NEUTRAL, now, 600);
}

static void endEvent(uint32_t now) {
  cur = EV_NONE; curPrio = 0;
  restoreIdle(now);
  nextMicro.set(now, rndMs(MICRO_MIN_MS, MICRO_MAX_MS));
}

bool directorRequest(EventId id, uint8_t prio, uint32_t now) {
  if (id == EV_NONE || id >= EV_COUNT) return false;
  if (goingDark && id != EV_SLEEP) return false;
  if (cur != EV_NONE) {
    if (!DEFS[cur]->interruptible) return false;      // e.g. boot finishes cleanly
    if (prio <= curPrio) return false;                // equal / lower priority never interrupts
    DEFS[cur]->cancel(now);
  }
  cur = id; curPrio = prio;
  microRestoreAt = 0;
  DEFS[id]->begin(now);
#if DEBUG_SERIAL
  Serial.printf("[event] %-10s prio=%u t=%lus\n", DEFS[id]->name, (unsigned)prio, (unsigned long)(now / 1000));
#endif
  if (prio >= PRI_USER) powerPoke(now);
  return true;
}

EventId directorCurrent() { return cur; }
bool    directorDark()    { return powerIsDark(); }

uint8_t directorFps() {
  if (cur != EV_NONE) return FPS_ACTIVE;
  return powerIdleFps();
}

void directorBegin(uint32_t now) {
  messagesBegin();
  sceneReset(now);
  emotion.begin(now, EMO_NEUTRAL);
  character.reset();
  powerBegin(now);
  inputBegin();
  directorRequest(EV_BOOT, PRI_BOOT, now);
  nextMessage.set(now, MESSAGE_FIRST_MS);
  nextEffect.set(now, EFFECT_FIRST_MS);
  nextMicro.set(now, 6000);
  rareReadyAt = now + RARE_COOLDOWN_MS / 2;
}

// ------------------------------------------------------------------ scheduling
static bool startMessage(uint8_t prio, uint32_t now) {
  MsgPick p = messagesPick();
  messageSetPending(p.text, p.emo, p.icon);
  if (directorRequest(EV_MESSAGE, prio, now)) return true;
  messageSetPending(nullptr, EMO_NEUTRAL, 0);
  return false;
}

static EventId pickEffect() {
  // weights: glitch, space, transform, react   (by mode)
  static const uint8_t W[4][4] = {{15, 20, 25, 40}, {45, 10, 40, 5}, {10, 65, 15, 10}, {20, 20, 20, 40}};
  EventId ids[4] = {EV_GLITCH, EV_SPACE, EV_TRANSFORM, EV_NONE};
  for (int tries = 0; tries < 6; tries++) {
    int tot = 0; for (int i = 0; i < 4; i++) tot += W[mode][i];
    int r = (int)rndi(0, tot), k = 0;
    for (; k < 4; k++) { if (r < W[mode][k]) break; r -= W[mode][k]; }
    EventId e = ids[k];
    if (e == EV_NONE) { reactToggle = !reactToggle; e = reactToggle ? EV_LOVE : EV_HAPPY; }
    if (e != lastEffect) return e;
  }
  return EV_HAPPY;
}

static EventId pickRare() {
  static const EventId pool[4] = {EV_MORPH, EV_POWERUP, EV_HEARTPULSE, EV_PEEK};
  EventId e;
  do { e = pool[rndi(0, 4)]; } while (e == lastRare);
  return e;
}

static void microIdle(uint32_t now) {
  int8_t a;
  do { a = (int8_t)rndi(0, 8); } while (a == lastMicro);
  if (a == 5 && !powerCalm() && chance(60)) a = 0;               // yawns are more likely when calm
  lastMicro = a;
  switch (a) {
    case 0: emotion.set(EMO_HAPPY, now); microRestoreAt = now + rndMs(1400, 2300); break;
    case 1: emotion.set(EMO_CURIOUS, now); emotion.overrideGaze(chance(50) ? 0.8f : -0.8f, -0.3f, 900, now); microRestoreAt = now + 1700; break;
    case 2: character.wiggleSprout(now); break;
    case 3: character.twitchEar(now, chance(50) ? -1 : 1); break;
    case 4: character.kickBounce(now, 2.0f, 340); break;
    case 5: emotion.set(EMO_SLEEPY, now, 700); microRestoreAt = now + 1900; break;
    case 6: emotion.set(EMO_SHY, now); microRestoreAt = now + 1500; break;
    case 7: emotion.set(EMO_CONFUSED, now); character.kickShake(now, 0.8f, 420); microRestoreAt = now + 1300; break;
  }
}

static void schedule(uint32_t now) {
  // display-off policy (opt-in): sleep sequence first, then power the panel down
  if (powerWantsDisplayOff(now) && !goingDark) {
    sleepSetForever(true);
    if (directorRequest(EV_SLEEP, PRI_USER, now)) { goingDark = true; return; }
  }
  float calmMul = powerCalm() ? 2.f : 1.f;

  if (nextMessage.due(now)) {
    if (startMessage(PRI_MESSAGE, now)) nextMessage.set(now, rndMs(MESSAGE_MIN_MS, MESSAGE_MAX_MS));
    else nextMessage.set(now, 3000);
    return;
  }
  if (nextEffect.due(now)) {
    EventId e;
    bool rare = timeReached(now, rareReadyAt) && chance(30);
    e = rare ? pickRare() : pickEffect();
    if (directorRequest(e, PRI_EFFECT, now)) {
      if (rare) { lastRare = e; rareReadyAt = now + RARE_COOLDOWN_MS; } else lastEffect = e;
      nextEffect.set(now, (uint32_t)(rndMs(EFFECT_MIN_MS, EFFECT_MAX_MS) * calmMul));
    } else nextEffect.set(now, 3000);
    return;
  }
  if (microRestoreAt && timeReached(now, microRestoreAt)) {
    microRestoreAt = 0; emotion.clearGazeOverride();
    if (emotion.target() != EMO_NEUTRAL) emotion.set(EMO_NEUTRAL, now, 600);
  }
  if (nextMicro.due(now)) {
    if (!microRestoreAt) microIdle(now);
    nextMicro.set(now, (uint32_t)(rndMs(MICRO_MIN_MS, MICRO_MAX_MS) * calmMul));
  }
}

// ------------------------------------------------------------------ user actions
void directorAction(Action a, uint32_t now) {
  powerPoke(now);
  switch (a) {
    case ACT_REACT:
      reactToggle = !reactToggle;
      directorRequest(reactToggle ? EV_LOVE : EV_HAPPY, PRI_USER, now);
      break;
    case ACT_MESSAGE:   startMessage(PRI_USER, now); break;
    case ACT_MODE_NEXT: mode = (mode + 1) & 3; modeLabelUntil = now + 1400; break;
    case ACT_SPACE:     directorRequest(EV_SPACE, PRI_USER, now); break;
    case ACT_GLITCH:    directorRequest(EV_GLITCH, PRI_USER, now); break;
    case ACT_TRANSFORM: directorRequest(EV_TRANSFORM, PRI_USER, now); break;
    case ACT_RARE:      directorRequest(pickRare(), PRI_USER, now); break;
    case ACT_GAME:
#if ENABLE_BUTTON
      directorRequest(EV_GAME, PRI_GAME, now);
#endif
      break;
    case ACT_SLEEP_TOGGLE:
      if (cur == EV_SLEEP) sleepWake();
      else { sleepSetForever(true); directorRequest(EV_SLEEP, PRI_USER, now); }
      break;
    default: break;
  }
}

void directorCycleEmotion(uint32_t now) {
  if (cur != EV_NONE) return;
  emoCycle = (emoCycle + 1) % EMO_COUNT;
  emotion.set((Emo)emoCycle, now);
  microRestoreAt = now + 4000;
#if DEBUG_SERIAL
  Serial.printf("[emotion] %s\n", EMOTIONS[emoCycle].name);
#endif
}

static void handleInput(uint32_t now) {
  InputEvt in = inputPoll(now);
#if ENABLE_BUTTON
  if (cur == EV_GAME) {
    if (inputPressEdge()) gameSendPress(now);
    if (in == IN_LONG) gameSendLong();
    return;
  }
  inputPressEdge();    // discard
#endif
  if (in == IN_NONE) return;
  if (powerIsDark()) { return; }
  Action a = in == IN_SHORT ? BTN_SHORT_ACTION : (in == IN_DOUBLE ? BTN_DOUBLE_ACTION : BTN_LONG_ACTION);
  if (mode == 3 && in == IN_SHORT) a = ACT_GAME;                 // arcade mode: short press starts the game
  directorAction(a, now);
}

// ------------------------------------------------------------------ dark (display off) handling
void directorDarkTick(uint32_t now) {
  InputEvt in = inputPoll(now);
  bool wake = powerWakeDue(now) || in != IN_NONE;
  if (wake) {
    powerDisplayOn(now);
    goingDark = false;
    sleepWake();                                          // plays the wake-up part of the sleep event
    nextMessage.set(now, 3500);                           // a little message with every peek
    emotion.update(now);
  } else powerDarkSlice();
}

// ------------------------------------------------------------------ frame
void directorFrame(uint32_t now) {
  emotion.update(now);
  character.update(now);
  particles.update(now);
  powerUpdate(now);
  handleInput(now);

  if (cur != EV_NONE) {
    if (!DEFS[cur]->update(now)) endEvent(now);
  } else {
    schedule(now);
  }

  if (cur != EV_NONE) DEFS[cur]->draw(now);
  else sceneDrawBase();

  if (cur == EV_NONE && modeLabelUntil && !timeReached(now, modeLabelUntil)) {
    oled.fillRect(0, 53, 128, 11, SSD1306_BLACK);
    gfxTextCentered(MODE_NAMES[mode], 64, 55, SSD1306_WHITE, true);
  }
  displayFlush();

  // display-off: once the mascot is fast asleep, power the panel down
  if (goingDark && cur == EV_SLEEP && sleepIsDeep() && !powerIsDark()) powerDisplayOff(now);
}
