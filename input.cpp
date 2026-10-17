#include "input.h"
#include "config.h"

#if ENABLE_BUTTON
static bool     stable = false, raw = false, pressEdge = false, longFired = false;
static uint32_t rawSince = 0, pressedAt = 0, lastRelease = 0;
static uint8_t  clicks = 0;

static bool readPressed() {
  int v = digitalRead(PIN_BUTTON);
  return BUTTON_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

void inputBegin() {
  pinMode(PIN_BUTTON, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
  stable = raw = readPressed(); rawSince = millis();
}

bool inputPressEdge() { bool e = pressEdge; pressEdge = false; return e; }

InputEvt inputPoll(uint32_t now) {
  bool r = readPressed();
  if (r != raw) { raw = r; rawSince = now; }
  if (raw != stable && (now - rawSince) >= BTN_DEBOUNCE_MS) {          // debounced change
    stable = raw;
    if (stable) { pressedAt = now; longFired = false; pressEdge = true; }
    else if (!longFired) {
      clicks++; lastRelease = now;
      if (clicks >= 2) { clicks = 0; return IN_DOUBLE; }
    }
  }
  if (stable && !longFired && (now - pressedAt) >= BTN_LONG_MS) { longFired = true; clicks = 0; return IN_LONG; }
  if (!stable && clicks == 1 && (now - lastRelease) >= BTN_DOUBLE_GAP_MS) { clicks = 0; return IN_SHORT; }
  return IN_NONE;
}
#else
void     inputBegin() {}
InputEvt inputPoll(uint32_t) { return IN_NONE; }
bool     inputPressEdge() { return false; }
#endif
