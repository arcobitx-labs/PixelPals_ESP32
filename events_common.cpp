#include "events.h"

void sceneReset(uint32_t now) {
  character.reset();
  emotion.clearGazeOverride();
  emotion.setBlinkEnabled(true);
  emotion.set(EMO_NEUTRAL, now, 500);
  particles.clear();
}

void sceneDrawBase() {
  oled.clearDisplay();
  character.draw(millis());
  particles.draw();
}

void sceneCharBox(int &x0, int &y0, int &x1, int &y1) { character.bounds(x0, y0, x1, y1); }
