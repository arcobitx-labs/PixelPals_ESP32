#pragma once
// ============================================================
// PixelPals - central configuration
// Everything hardware-dependent or tunable lives here.
// ============================================================
#include <stdint.h>

// ---------------- Display / I2C (confirmed) ----------------
#define PIN_SDA        8        // GPIO8 is also the SuperMini's onboard LED: never drive it.
#define PIN_SCL        9        // GPIO9 is the BOOT button / strapping pin. OK as SCL with OLED pull-ups.
#define OLED_ADDR      0x3C
#define SCREEN_W       128
#define SCREEN_H       64
#define I2C_CLOCK_HZ   400000UL // Try 800000 once everything works (faster frames, panel permitting).
#define OLED_CONTRAST_ACTIVE 0xCF
#define OLED_CONTRAST_CALM   0x30

// ---------------- Frame rates ----------------
#define FPS_ACTIVE     30       // events, effects, messages
#define FPS_IDLE       24       // lively idle (smooth blinks / gaze)
#define FPS_CALM       12       // after CALM_AFTER_MS without interaction

// ---------------- Demo mode ----------------
// 1 = events come quickly so you can see everything within a minute or two.
// 0 = relaxed "pocket companion" pacing (messages every few minutes).
#ifndef DEMO_MODE
#define DEMO_MODE      1
#endif

#if DEMO_MODE
  #define MESSAGE_FIRST_MS   12000UL
  #define MESSAGE_MIN_MS     35000UL
  #define MESSAGE_MAX_MS     60000UL
  #define EFFECT_FIRST_MS    25000UL
  #define EFFECT_MIN_MS      18000UL
  #define EFFECT_MAX_MS      35000UL
  #define RARE_COOLDOWN_MS   90000UL
  #define MICRO_MIN_MS       3500UL
  #define MICRO_MAX_MS       8000UL
#else
  #define MESSAGE_FIRST_MS   45000UL
  #define MESSAGE_MIN_MS     150000UL    // 2.5 min
  #define MESSAGE_MAX_MS     360000UL    // 6 min
  #define EFFECT_FIRST_MS    70000UL
  #define EFFECT_MIN_MS      90000UL
  #define EFFECT_MAX_MS      240000UL
  #define RARE_COOLDOWN_MS   900000UL    // 15 min
  #define MICRO_MIN_MS       4000UL
  #define MICRO_MAX_MS       10000UL
#endif

// ---------------- Power policy ----------------
// Level 1 (always on): after CALM_AFTER_MS of no interaction the frame rate and contrast drop,
//   and identical frames are never re-sent over I2C.
#ifndef CALM_AFTER_MS
#define CALM_AFTER_MS        60000UL
#endif
// Level 2 (opt-in): turn the display OFF after this much inactivity (0 = never).
//   The mascot plays its sleep sequence first. Without a button it wakes itself every
//   DARK_PERIOD_MS for a short "peek" (wake animation + message), then sleeps again.
#ifndef DISPLAY_OFF_AFTER_MS
#define DISPLAY_OFF_AFTER_MS 0UL        // e.g. 300000UL (5 min)
#endif
#ifndef DARK_PERIOD_MS
#define DARK_PERIOD_MS       600000UL   // 10 min dark between peeks
#endif
#ifndef PEEK_ON_MS
#define PEEK_ON_MS           20000UL
#endif
// Level 3 (opt-in): CPU light sleep while the display is off (timer wake only).
//   NOTE: USB serial drops during light sleep; leave 0 while developing.
#ifndef POWER_LIGHT_SLEEP
#define POWER_LIGHT_SLEEP    0
#endif
// Optional: run the CPU at 80 MHz while calm/dark (160 MHz otherwise). Leave 0 until the basics work.
#ifndef POWER_DYNAMIC_CPU
#define POWER_DYNAMIC_CPU    0
#endif
// Deep sleep is intentionally NOT used: without a button or sensor nothing could wake it on demand.

// ---------------- Optional input ----------------
#ifndef ENABLE_BUTTON
#define ENABLE_BUTTON  1
#endif
#define PIN_BUTTON  7          // ESP32-C3 GPIO7; wire a momentary button between this pin and GND.
#define BUTTON_ACTIVE_LOW  1      // wired to GND with INPUT_PULLUP
#define BTN_DEBOUNCE_MS    25
#define BTN_LONG_MS        700
#define BTN_DOUBLE_GAP_MS  280

enum Action : uint8_t {
  ACT_NONE, ACT_REACT, ACT_MESSAGE, ACT_MODE_NEXT, ACT_SPACE, ACT_GLITCH,
  ACT_TRANSFORM, ACT_RARE, ACT_GAME, ACT_SLEEP_TOGGLE
};
// Button behaviour table: change these three lines to re-map the button, no engine changes needed.
#define BTN_SHORT_ACTION   ACT_REACT
#define BTN_DOUBLE_ACTION  ACT_MESSAGE
#define BTN_LONG_ACTION    ACT_MODE_NEXT

#if ENABLE_BUTTON && !defined(PIN_BUTTON)
  #error "ENABLE_BUTTON is 1 but PIN_BUTTON is not defined. Confirm your wiring and define PIN_BUTTON."
#endif

// ---------------- Debug ----------------
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL   0          // serial console commands + [perf] output (press ? in the Serial Monitor)
#endif
#define FPS_REPORT_MS  3000
