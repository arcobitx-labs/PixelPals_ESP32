#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 oled;      // single 1 KB framebuffer, drawn into by everything else

bool displayInit();                // safe to call again to retry
bool displayReady();
bool displayFlush();               // push framebuffer over I2C; skips the transfer if nothing changed. Returns true if sent.
void displaySetContrast(uint8_t c);
void displayPower(bool on);        // SSD1306 display on/off (RAM retained)
void displayReport(uint32_t now);  // periodic FPS / flush-time / heap print on Serial
