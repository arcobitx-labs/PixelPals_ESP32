#pragma once
// Non-blocking debounced button. Compiles to no-ops while ENABLE_BUTTON is 0.
#include <Arduino.h>
enum InputEvt : uint8_t { IN_NONE, IN_SHORT, IN_DOUBLE, IN_LONG };
void     inputBegin();
InputEvt inputPoll(uint32_t now);       // classified press (short press is reported after the double-press window)
bool     inputPressEdge();              // raw debounced press edge (for the mini-game: zero latency), cleared on read
