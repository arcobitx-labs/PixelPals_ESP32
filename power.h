#pragma once
// Power policy: calm mode (lower FPS + dim), display off cycle, optional light sleep / CPU scaling.
// Distinguishes: reducing updates  /  display off  /  CPU light sleep. Deep sleep is not used (nothing could wake it).
#include <Arduino.h>

void    powerBegin(uint32_t now);
void    powerPoke(uint32_t now);              // user interaction or notable event: leave calm mode
void    powerUpdate(uint32_t now);            // applies contrast / CPU frequency changes
uint8_t powerIdleFps();                       // FPS to use when nothing is animating
bool    powerCalm();
bool    powerWantsDisplayOff(uint32_t now);
void    powerDisplayOff(uint32_t now);
bool    powerIsDark();
bool    powerWakeDue(uint32_t now);
void    powerDisplayOn(uint32_t now);
void    powerDarkSlice();                     // one low-power slice while the display is off
