// Optional one-button mini-game ("Pixel Hop"): jump gaps, collect 8 coins to win.
// Only compiled when ENABLE_BUTTON is 1 (needs real hardware input).
#include "events.h"

#if ENABLE_BUTTON
namespace gm {
enum St { START, PLAY, WIN, OVER };
struct Plat { float x, w; };
struct Coin { float x, y; bool on; };
static Plat pl[8]; static int nPl;
static Coin coins[8];
static St st; static uint32_t stT, last, jumpBuf, lastGround, lastCoin;
static float py, vy, speed; static bool onGround, press, quit; static int score;
static const float GROUND = 50.f, PX = 30.f, GS = 0.5f, FEET = 27.5f * 0.5f, GRAV = 780.f, JUMPV = -208.f;
static const int GOAL = 8;

static float groundY() { return GROUND - FEET; }

static void addPlatform(float gapMin, float gapMax) {
  float x = 0; if (nPl > 0) x = pl[nPl - 1].x + pl[nPl - 1].w + rndf(gapMin, gapMax);
  float w = rndf(30, 52);
  pl[nPl].x = x; pl[nPl].w = w; nPl++;
  if (chance(80)) for (int i = 0; i < 8; i++) if (!coins[i].on) { coins[i] = {x + w * rndf(0.3f, 0.7f), GROUND - rndf(14, 26), true}; break; }
  if (nPl > 1 && chance(45)) for (int i = 0; i < 8; i++) if (!coins[i].on) { coins[i] = {x - 9.f, GROUND - 31.f, true}; break; }
}

static void resetRun(uint32_t now) {
  nPl = 0; for (int i = 0; i < 8; i++) coins[i].on = false;
  pl[0] = {-10, 100}; nPl = 1;
  while (pl[nPl - 1].x + pl[nPl - 1].w < 170 && nPl < 8) addPlatform(11, 17);
  py = groundY(); vy = 0; onGround = true; speed = 50; score = 0; lastGround = now; jumpBuf = 0; lastCoin = 0;
}

static void toState(St s, uint32_t now) {
  st = s; stT = now; particles.clear();
  character.crown = (s == WIN);
  if (s == START) { emotion.set(EMO_HAPPY, now); }
  if (s == WIN)   { emotion.set(EMO_JOY, now); character.kickBounce(now, 4.f, 400); }
  if (s == OVER)  { emotion.set(EMO_SAD, now); character.kickShake(now, 1.2f, 500); }
  if (s == PLAY)  { resetRun(now); emotion.set(EMO_NEUTRAL, now, 200); }
}

void begin(uint32_t now) { last = now; press = quit = false; character.reset(); toState(START, now); }

static bool supported() { for (int i = 0; i < nPl; i++) if (pl[i].x <= PX + 3 && pl[i].x + pl[i].w >= PX - 3) return true; return false; }
static void jump(uint32_t now) { vy = JUMPV; onGround = false; jumpBuf = 0; character.kickShake(now, 0, 1); }

bool update(uint32_t now) {
  float dt = (now - last) * 0.001f; if (dt > 0.05f) dt = 0.05f; last = now;
  if (quit) return false;
  bool pr = press; press = false;
  if (st == START) { if (pr) toState(PLAY, now); else if (now - stT > 12000) return false; return true; }
  if (st == WIN || st == OVER) {
    if (pr && now - stT > 600) toState(START, now);
    else if (now - stT > 9000) return false;
    if (st == WIN && chance(12)) particles.heart(64 + rndf(-34, 34), 30 + rndf(-18, 18));
    return true;
  }
  // PLAY
  speed = 50.f + score * 2.5f;
  for (int i = 0; i < nPl; i++) pl[i].x -= speed * dt;
  for (int i = 0; i < 8; i++) if (coins[i].on) { coins[i].x -= speed * dt; if (coins[i].x < -8) coins[i].on = false; }
  while (nPl > 0 && pl[0].x + pl[0].w < -10) { for (int i = 1; i < nPl; i++) pl[i - 1] = pl[i]; nPl--; }
  while (pl[nPl - 1].x + pl[nPl - 1].w < 170 && nPl < 8) addPlatform(11, 17 + (score > 5 ? 5 : score));
  bool sup = supported();
  if (onGround && !sup) { onGround = false; lastGround = now; }
  if (pr) { if (onGround || now - lastGround < 90) jump(now); else jumpBuf = now; }
  if (!onGround) {
    float prev = py; vy += GRAV * dt; py += vy * dt;
    if (vy > 0 && prev <= groundY() + 0.5f && py >= groundY() && sup) {
      py = groundY(); vy = 0; onGround = true;
      if (jumpBuf && now - jumpBuf < 120) jump(now);
    }
    if (py > 80) { toState(OVER, now); return true; }
  }
  for (int i = 0; i < 8; i++) if (coins[i].on && fabsf(coins[i].x - PX) < 7 && fabsf(coins[i].y - (py - 3)) < 10) {
    coins[i].on = false; score++; lastCoin = now;
    particles.burst(coins[i].x, coins[i].y, 7, 40.f, 400, PK_SPARK);
    if (score >= GOAL) { toState(WIN, now); return true; }
  }
  return true;
}

void draw(uint32_t now) {
  oled.clearDisplay();
  if (st == START) {
    oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
    const char s[] = {'P','R','E','S','S',' ','\x03',0};
    gfxTextBold(s, 64 - 21, 4);
    character.x = 64; character.y = 38; character.scale = 0.72f; character.draw(now);
  } else if (st == PLAY) {
    for (int i = 0; i < nPl; i++) {
      int x0 = roundi(pl[i].x), w = roundi(pl[i].w);
      oled.fillRect(x0, (int)GROUND, w, 2, SSD1306_WHITE);
      for (int xx = 0; xx < w; xx += 4) for (int yy = 0; yy < 10; yy += 4) { oled.drawPixel(x0 + xx + (yy >> 1), (int)GROUND + 4 + yy, SSD1306_WHITE); }
      oled.drawFastVLine(x0, (int)GROUND, 14, SSD1306_WHITE); oled.drawFastVLine(x0 + w - 1, (int)GROUND, 14, SSD1306_WHITE);
    }
    for (int i = 0; i < 8; i++) if (coins[i].on) {
      bool wide = ((now / 140) & 3) != 0;
      if (wide) oled.drawBitmap(roundi(coins[i].x) - 4, roundi(coins[i].y) - 4, COIN, COIN_W, COIN_H, SSD1306_WHITE);
      else oled.fillRect(roundi(coins[i].x) - 1, roundi(coins[i].y) - 4, 3, 9, SSD1306_WHITE);
    }
    character.x = PX; character.y = py; character.scale = GS; character.showFx = false;
    FaceParams f = EMOTIONS[onGround ? ((now - lastCoin < 400 && lastCoin) ? EMO_HAPPY : EMO_NEUTRAL) : EMO_EXCITED].face;
    character.drawWith(f, 0.f, 0.7f, 0.f, now);
    oled.drawBitmap(1, 1, COIN, COIN_W, COIN_H, SSD1306_WHITE);
    char buf[8]; snprintf(buf, sizeof(buf), "%d/%d", score, GOAL);
    oled.setTextSize(1); oled.setCursor(12, 2); oled.print(buf);
  } else if (st == WIN) {
    character.x = 64; character.y = 36; character.scale = 0.78f; character.showFx = false; character.draw(now);
  } else {
    character.x = 64; character.y = 28; character.scale = 0.55f; character.showFx = false; character.draw(now);
    float ph = now * 0.004f;
    for (int i = 0; i < 16; i++) { float a = ph + i * 0.3927f; if (i & 1) continue; oled.drawPixel(64 + roundi(cosf(a) * 27), 28 + roundi(sinf(a) * 18), SSD1306_WHITE); }
    gfxTextCentered("GAME OVER", 64, 52, SSD1306_WHITE, true);
  }
  particles.draw();
}
void cancel(uint32_t now) { character.crown = false; sceneReset(now); }
}
const EventDef evdefGame = {"game", true, gm::begin, gm::update, gm::draw, gm::cancel};
void gameSendPress(uint32_t) { gm::press = true; }
void gameSendLong()          { gm::quit = true; }
#else
static void gNone(uint32_t) {}
static bool gUpd(uint32_t) { return false; }
const EventDef evdefGame = {"game", true, gNone, gUpd, gNone, gNone};
void gameSendPress(uint32_t) {}
void gameSendLong() {}
#endif
