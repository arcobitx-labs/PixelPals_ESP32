#include "power.h"
#include "config.h"
#include "display.h"
#if POWER_LIGHT_SLEEP
  #include <esp_sleep.h>
  #include <driver/gpio.h>
#endif

static uint32_t lastPoke = 0, darkSince = 0;
static bool calm = false, dark = false;

void powerBegin(uint32_t now) { lastPoke = now; calm = false; dark = false; }

void powerPoke(uint32_t now) { lastPoke = now; }

void powerUpdate(uint32_t now) {
  bool c = !dark && (uint32_t)(now - lastPoke) > CALM_AFTER_MS;
  if (c == calm) return;
  calm = c;
  displaySetContrast(calm ? OLED_CONTRAST_CALM : OLED_CONTRAST_ACTIVE);
#if POWER_DYNAMIC_CPU
  setCpuFrequencyMhz(calm ? 80 : 160);
#endif
}

uint8_t powerIdleFps() { return calm ? FPS_CALM : FPS_IDLE; }
bool    powerCalm()    { return calm; }
bool    powerIsDark()  { return dark; }

bool powerWantsDisplayOff(uint32_t now) {
  return DISPLAY_OFF_AFTER_MS > 0 && !dark && (uint32_t)(now - lastPoke) > DISPLAY_OFF_AFTER_MS;
}

void powerDisplayOff(uint32_t now) {
  displayPower(false);
  dark = true; darkSince = now;
#if POWER_DYNAMIC_CPU
  setCpuFrequencyMhz(80);
#endif
}

bool powerWakeDue(uint32_t now) { return dark && (uint32_t)(now - darkSince) >= DARK_PERIOD_MS; }

void powerDisplayOn(uint32_t now) {
  displayPower(true);
  dark = false; calm = false;
  displaySetContrast(OLED_CONTRAST_ACTIVE);
#if POWER_DYNAMIC_CPU
  setCpuFrequencyMhz(160);
#endif
  // schedule the next display-off after a short "peek" window
  uint32_t back = DISPLAY_OFF_AFTER_MS > PEEK_ON_MS ? DISPLAY_OFF_AFTER_MS - PEEK_ON_MS : 0;
  lastPoke = now - back;
}

void powerDarkSlice() {
#if POWER_LIGHT_SLEEP
  esp_sleep_enable_timer_wakeup(100000ULL);                     // wake every 100 ms to poll
  #if ENABLE_BUTTON
    gpio_wakeup_enable((gpio_num_t)PIN_BUTTON, BUTTON_ACTIVE_LOW ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  #endif
  esp_light_sleep_start();
#else
  delay(40);                                                    // display is off; this is the only place we wait
#endif
}
