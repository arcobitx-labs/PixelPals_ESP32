# PixelPals: ESP32-C3 pocket companion

A cat-eared helmet bot with a leaf sprout, built for a 0.96" SSD1306 128x64 OLED. Fully offline, no Wi-Fi, no API.

## Hardware (confirmed)
ESP32-C3 SuperMini, SSD1306 at I2C 0x3C, **SDA = GPIO 8, SCL = GPIO 9**. Optional button input: GPIO 7 to GND.

## Arduino IDE setup
1. Keep all files together in a folder named `PixelPals`.
2. Library Manager: install **Adafruit SSD1306** and **Adafruit GFX Library** (accept BusIO).
3. Boards Manager: **esp32 by Espressif**. Board: **ESP32C3 Dev Module**.
4. Tools > **USB CDC On Boot: Enabled** (Serial Monitor on the C3).
5. Upload. If the upload fails, hold BOOT while plugging in USB. If it uploads but nothing runs, try Flash Mode DIO.
6. Serial Monitor at 115200: you should see `[display] OK ...` and `[perf] ...` lines.

Blank screen? The Serial Monitor prints an I2C scan. If it finds 0x3D, change `OLED_ADDR` in `config.h`.

## What it does
- **Boot:** a pixel flickers on, grows, bursts into particles that assemble the mascot, eyes open.
- **11+ emotions:** neutral, happy, love, excited, curious, shy, surprised, confused, sleepy, sad, annoyed (plus wink, joy, asleep). One parameter set per emotion, eased between; randomized blinks (incl. double blinks), gaze shifts that avoid repeating, micro-jitter, breathing, sprout sway, ear twitches.
- **Messages (42, five categories):** the mascot reacts, slides left and shrinks, a speech bubble pops in, typewriter text, hold, back to idle. Recent messages are not repeated. Edit `messages.cpp` to add your own (max 4 lines x 13 chars; `<3` becomes a heart).
- **Effects:** pixel dissolve + reassemble into a new expression, cyberpunk glitch, space (stars, ringed planet, shooting star, warp), love and happy reactions, sleep/wake.
- **Rare surprises (cooldown):** morph (helmet/cat/blob + crown), energy-ring power-up, heart pulse, peek-a-boo.
- **Mini-game "Pixel Hop"** (needs a button): jump gaps, collect 8 coins.

## Try everything quickly
`DEMO_MODE 1` (default in `config.h`) makes events frequent. Set it to **0** for relaxed pacing (messages every 2.5 to 6 min).
In the Serial Monitor press `?` for commands: `m` message, `l` love, `h` happy, `t` transform, `g` glitch, `p` space,
`1` morph, `2` power-up, `3` heart pulse, `4` peek, `r` random rare, `s` sleep, `w` wake, `e` cycle emotions.

## Priorities
Boot (cannot be interrupted) > game > user action > message > scheduled effect > idle micro-behaviours.
Equal or lower priority never interrupts; a message is never overwritten by an idle event; every event ends by easing back to idle.

## Power (all knobs in config.h)
1. **Always on:** identical frames are not re-sent over I2C; after `CALM_AFTER_MS` (60 s) idle FPS drops 24 -> 12 and contrast dims.
2. **Opt-in display off:** set `DISPLAY_OFF_AFTER_MS` (e.g. 300000). The mascot falls asleep, the panel turns off, and every `DARK_PERIOD_MS` it wakes for a short peek with a message. A button press also wakes it.
3. **Opt-in CPU light sleep** (`POWER_LIGHT_SLEEP 1`) while the display is off, timer wake. USB serial drops during light sleep, so keep it 0 while developing. `POWER_DYNAMIC_CPU 1` runs 80 MHz when calm.
Deep sleep is deliberately not used: with no button or sensor nothing could wake it on demand.
No battery-life figure is claimed. It depends on your cell, regulator and measured current.

## Button
Button support is enabled by default on **GPIO 7**. Wire a momentary switch between GPIO 7 and GND; the firmware uses
`INPUT_PULLUP`, so do not wire the button to 3.3 V. Defaults: short = reaction, double = message, long = next mode
(Pet / Cyber / Space / Arcade; in Arcade a short press starts the game). Re-map via `BTN_SHORT_ACTION /
BTN_DOUBLE_ACTION / BTN_LONG_ACTION`. Avoid GPIO 8 (LED/SDA) and 9 (SCL/BOOT).

A short press is reported about 280 ms after release so the firmware can distinguish a double press. Wait until the boot
animation finishes before testing: boot has priority and cannot be interrupted. The mini-game reads the debounced press
edge immediately.

## Code map
`config.h` settings | `display` I2C + skip-identical-frame flush | `gfx` helpers | `anim_core.h` easing/timers |
`emotion` emotion table + blink/gaze | `character` the mascot | `particles` | `effects` stars/planet/ring/glitch |
`events_*.cpp` one file per group of animations | `director` priorities + scheduler | `messages` | `input` | `power` | `game` | `console`.
To add an animation: write an `EventDef` (begin/update/draw/cancel), register it in `director.cpp`.

## Honest status
Written and checked on a desktop simulator (real firmware code against stand-in libraries): all animations rendered and reviewed
frame by frame, all 42 messages verified to fit, the scheduler, priorities, power cycle and game exercised, and the code compiles
warning-free as C++11 and C++17. **Not yet compiled with the ESP32 toolchain and not run on your board.** Frame rate on real
hardware depends on I2C speed (about 25 ms per full frame at 400 kHz), so check the `[perf]` lines; if the panel tolerates it,
try `I2C_CLOCK_HZ 800000UL`.
