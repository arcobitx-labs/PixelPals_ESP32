// Message reveal: react -> slide into compact layout -> speech bubble pops in -> typewriter
// -> hold -> bubble shrinks -> mascot returns to idle.
#include "events.h"
#include "messages.h"
#include <string.h>

namespace msg {
const char *pendingText = nullptr; Emo pendingEmo = EMO_HAPPY; uint8_t pendingIcon = 0;
char lines[MSG_MAX_LINES][MSG_LINE_CHARS + 2]; int nLines, totalChars;
Emo emo; uint8_t icon;
uint32_t t0, typeStart, typeDur, exitStart, endAt; bool d1, d2, d3, heartSpawned;

const int BX = 41, BY = 6, BW = 85, BH = 52, TAILX = 40, TAILY = 32;

void begin(uint32_t now) {
  t0 = now; d1 = d2 = d3 = heartSpawned = false;
  if (!pendingText) { MsgPick p = messagesPick(); pendingText = p.text; pendingEmo = p.emo; pendingIcon = p.icon; }
  emo = pendingEmo; icon = pendingIcon;
  nLines = messagesWrap(pendingText, lines, MSG_MAX_LINES, MSG_LINE_CHARS);
  pendingText = nullptr;
  totalChars = 0; for (int i = 0; i < nLines; i++) totalChars += (int)strlen(lines[i]);
  typeStart = 1450;
  typeDur = (uint32_t)clampf(totalChars * 36.f, 500.f, 1500.f);
  uint32_t hold = (uint32_t)clampf(1700.f + 55.f * totalChars, 2600.f, 6200.f);
  exitStart = typeStart + typeDur + hold;
  endAt = exitStart + 850;
  emotion.set(emo, now);
  character.kickBounce(now, 3.f, 360);
  particles.ring(character.centerX(), character.centerY() - 4, 24.f, 5, 30.f, 600, PK_SPARK);
}

bool update(uint32_t now) {
  uint32_t t = now - t0;
  // mascot pose: slide left & shrink, later return
  if (t < exitStart) {
    float p = easeInOutCubic(progress(t, 650, 700));
    character.x = lerpf(64, 19, p); character.y = lerpf(34, 36, p); character.scale = lerpf(1.f, 0.58f, p);
  } else {
    float p = easeInOutCubic(progress(t, exitStart + 100, 650));
    character.x = lerpf(19, 64, p); character.y = lerpf(36, 34, p); character.scale = lerpf(0.58f, 1.f, p);
  }
  // eyes glance at the bubble while it is on screen
  if (t > 1000 && t < exitStart + 100) {
    if (t < typeStart + typeDur + 900) emotion.overrideGaze(0.55f, 0.1f, 200, now);
    else emotion.clearGazeOverride();
  }
  if (!d1 && t > typeStart + typeDur) {
    d1 = true;
    if (icon == 1) particles.heart(30, 22);
    character.wiggleSprout(now);
  }
  if (!d2 && t > exitStart + 100) { d2 = true; emotion.clearGazeOverride(); emotion.set(EMO_NEUTRAL, now, 650); }
  return t < endAt;
}

static void drawBubble(float g, uint32_t now, uint32_t t) {
  if (g <= 0.05f) return;
  int x0 = TAILX + roundi((BX - TAILX) * g), x1 = TAILX + roundi((BX + BW - TAILX) * g);
  int y0 = TAILY + roundi((BY - TAILY) * g), y1 = TAILY + roundi((BY + BH - TAILY) * g);
  int w = x1 - x0, h = y1 - y0;
  if (w < 10 || h < 10) return;
  oled.fillRoundRect(x0, y0, w, h, 4, SSD1306_BLACK);
  oled.drawRoundRect(x0, y0, w, h, 4, SSD1306_WHITE);
  if (g > 0.45f) {                                           // little tail pointing at the mascot
    oled.drawLine(x0 - 5, TAILY, x0, TAILY - 3, SSD1306_WHITE);
    oled.drawLine(x0 - 5, TAILY, x0, TAILY + 3, SSD1306_WHITE);
    oled.drawFastVLine(x0, TAILY - 2, 5, SSD1306_BLACK);
  }
  if (g < 0.92f) return;                                     // text only once the bubble is (nearly) full size
  // typewriter text, lines centered
  float typeP = progress(t, typeStart, typeDur);
  int shown = (int)(typeP * totalChars + 0.5f);
  if (t >= exitStart) shown = totalChars;
  int ty = y0 + (h - (nLines * 9 - 1)) / 2;
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < nLines && shown > 0; i++) {
    int len = (int)strlen(lines[i]);
    int n = shown < len ? shown : len;
    int lx = x0 + (w - len * 6) / 2;
    oled.setCursor(lx, ty + i * 9);
    for (int c = 0; c < n; c++) oled.write((uint8_t)lines[i][c]);
    shown -= len;
  }
  if (typeP >= 1.f && t < exitStart) {                       // corner icon
    bool big = ((now / 380) & 1);
    if (icon == 1) oled.drawBitmap(x0 + w - 9, y0 + 2, big ? HEART7 : HEART5, big ? HEART7_W : HEART5_W, big ? HEART7_H : HEART5_H, SSD1306_WHITE);
    else if (icon == 2) oled.drawBitmap(x0 + w - 8, y0 + 2, big ? SPARK5 : SPARK3, big ? SPARK5_W : SPARK3_W, big ? SPARK5_H : SPARK3_H, SSD1306_WHITE);
  }
}

void draw(uint32_t now) {
  uint32_t t = now - t0;
  sceneDrawBase();
  float g = 0.f;
  if (t >= 1000 && t < exitStart) g = easeOutBack(progress(t, 1000, 450));
  else if (t >= exitStart) g = 1.f - easeInBack(progress(t, exitStart, 380));
  drawBubble(g < 0 ? 0 : g, now, t);
}
void cancel(uint32_t now) { sceneReset(now); }
}
const EventDef evdefMessage = {"message", true, msg::begin, msg::update, msg::draw, msg::cancel};

void messageSetPending(const char *text, Emo emo, uint8_t icon) {
  msg::pendingText = text; msg::pendingEmo = emo; msg::pendingIcon = icon;
}
