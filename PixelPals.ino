// ============================================================
// PixelPals - ESP32-C3 pocket companion (SSD1306 128x64, I2C)
// Board  : ESP32C3 Dev Module (ESP32-C3 SuperMini)     SDA = GPIO8, SCL = GPIO9
// Libs   : Adafruit SSD1306 + Adafruit GFX Library (BusIO installs with them)
// Setting: Tools > USB CDC On Boot > Enabled   (needed for Serial Monitor on the C3)
// Tune everything in config.h. Press ? in the Serial Monitor for test commands.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "director.h"
#include "console.h"

static uint32_t nextFrameMs = 0;
static uint32_t lastRetryMs = 0;

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500) { }        // short wait for USB CDC, never blocks forever
  randomSeed((uint32_t)ESP.getEfuseMac() ^ micros());   // varies per boot
  Serial.println(F("\n[PixelPals] starting"));
  if (displayInit()) directorBegin(millis());
}

void loop() {
  uint32_t now = millis();

  if (!displayReady()) {                              // display missing: retry every 3 s, non-blocking
    if (now - lastRetryMs >= 3000) {
      lastRetryMs = now;
      if (displayInit()) directorBegin(now);
    }
    return;
  }

  consolePoll(now);

  if (directorDark()) {                               // display off: poll slowly, wake on schedule/button
    directorDarkTick(now);
    return;
  }

  // fixed-rate frame pacing, no delay()
  if ((int32_t)(now - nextFrameMs) < 0) return;
  uint32_t interval = 1000UL / directorFps();
  nextFrameMs += interval;
  if ((int32_t)(now - nextFrameMs) > 0) nextFrameMs = now + interval;   // fell behind: resync

  directorFrame(now);
  displayReport(now);
}
