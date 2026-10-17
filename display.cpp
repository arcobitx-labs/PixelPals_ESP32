#include "display.h"
#include "config.h"
#include <Wire.h>

Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1, I2C_CLOCK_HZ, I2C_CLOCK_HZ);

static bool     s_wireStarted = false;
static bool     s_ready       = false;
static uint32_t s_lastHash    = 0;
static bool     s_haveHash    = false;
static uint32_t s_frames = 0, s_sent = 0, s_flushUsAcc = 0, s_lastReport = 0;
static uint8_t  s_contrast = 0xFF;

static void i2cScan() {
#if DEBUG_SERIAL
  Serial.println(F("[i2c] scanning..."));
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("[i2c] device at 0x%02X\n", a); found++; }
  }
  if (!found) Serial.println(F("[i2c] nothing found: check SDA/SCL wiring, power and GND"));
#endif
}

bool displayInit() {
  if (!s_wireStarted) {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(I2C_CLOCK_HZ);
    s_wireStarted = true;
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {   // periphBegin=false: Wire already started
#if DEBUG_SERIAL
    Serial.printf("[display] SSD1306 init failed at 0x%02X\n", OLED_ADDR);
#endif
    i2cScan();
    s_ready = false;
    return false;
  }
  oled.clearDisplay();
  oled.display();
  s_ready = true;
  s_haveHash = false;
  s_lastReport = millis();
  displaySetContrast(OLED_CONTRAST_ACTIVE);
#if DEBUG_SERIAL
  Serial.printf("[display] OK: %dx%d @ 0x%02X, SDA=%d SCL=%d, %lu kHz\n",
                SCREEN_W, SCREEN_H, OLED_ADDR, PIN_SDA, PIN_SCL, (unsigned long)(I2C_CLOCK_HZ / 1000));
#endif
  return true;
}

bool displayReady() { return s_ready; }

bool displayFlush() {
  s_frames++;
  // FNV-1a over the 1 KB framebuffer: if the picture did not change, skip the ~20 ms I2C transfer.
  const uint8_t *b = oled.getBuffer();
  uint32_t h = 2166136261u;
  for (int i = 0; i < (SCREEN_W * SCREEN_H / 8); i++) { h ^= b[i]; h *= 16777619u; }
  if (s_haveHash && h == s_lastHash) return false;
  s_lastHash = h; s_haveHash = true;
  uint32_t t0 = micros();
  oled.display();
  s_flushUsAcc += micros() - t0;
  s_sent++;
  return true;
}

void displaySetContrast(uint8_t c) {
  if (c == s_contrast) return;
  s_contrast = c;
  oled.ssd1306_command(SSD1306_SETCONTRAST);
  oled.ssd1306_command(c);
}

void displayPower(bool on) {
  oled.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  s_haveHash = false;
}

void displayReport(uint32_t now) {
#if DEBUG_SERIAL
  uint32_t dt = now - s_lastReport;
  if (dt < FPS_REPORT_MS) return;
  float fps = s_frames * 1000.0f / (float)dt;
  float sent = s_sent * 1000.0f / (float)dt;
  unsigned long avgUs = s_sent ? (s_flushUsAcc / s_sent) : 0;
  Serial.printf("[perf] %.1f frames/s | %.1f I2C sends/s | flush avg %lu us | free heap %lu B\n",
                fps, sent, avgUs, (unsigned long)ESP.getFreeHeap());
  s_frames = 0; s_sent = 0; s_flushUsAcc = 0; s_lastReport = now;
#endif
}
