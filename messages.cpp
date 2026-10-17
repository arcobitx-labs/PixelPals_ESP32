#include "messages.h"
#include "anim_core.h"
#include <string.h>

// ---------------------------------------------------------------- message tables (edit freely)
static const char *const MSG_ENCOURAGE[] = {
  "You got\nthis!", "Keep\ngoing!", "One step\nat a time.", "Proud of\nyou!",
  "Small\nsteps...\nbig dreams", "You can do\nhard things.", "Progress, not\nperfection.",
  "Go at your\nown pace.", "Believe in\nyourself!", "Today counts.\nSo do you.",
};
static const char *const MSG_KIND[] = {
  "You\nmatter.", "Stay\nkind.", "You make a\ndifference.", "The world\nneeds your\nlight.",
  "Be gentle\nwith\nyourself.", "Kindness\nlooks good\non you.", "Your smile\nis a gift.", "Good vibes\nonly",
};
static const char *const MSG_AFFECTION[] = {
  "Sending love\n<3", "You are\nappreciated.", "Someone is\nglad you're\nhere.", "You are\nawesome!",
  "You are\nloved.", "A tiny hug,\ndelivered.", "Sending you\ngood things.",
};
static const char *const MSG_MOOD[] = {
  "Hope I made\nyou smile.", "Take a little\nbreak.", "Better days\nahead.", "Don't forget\nto breathe.",
  "Drink some\nwater!", "Stretch and\nrelax.", "Rest is\nproductive\ntoo.", "This too\nshall pass.", "Look up for\na moment.",
};
static const char *const MSG_PLAYFUL[] = {
  "Tiny bot.\nBig heart.", "You found\nme!", "Beep boop.\nYou're\nawesome!", "Secret smile\nunlocked.",
  "Booting up\ngood vibes...", "Loading\nhappiness...\n100%", "Pixel power\nactivated!", "Beep!\nCompliment\ndelivered.",
};

struct Category {
  const char *name; const char *const *msgs; uint8_t count; uint8_t weight;
  Emo emoA, emoB; uint8_t icon;
};
#define N(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))
static const Category CATS[] = {
  {"encourage", MSG_ENCOURAGE, N(MSG_ENCOURAGE), 26, EMO_EXCITED, EMO_HAPPY,  2},
  {"kindness",  MSG_KIND,      N(MSG_KIND),      20, EMO_HAPPY,   EMO_LOVE,   1},
  {"affection", MSG_AFFECTION, N(MSG_AFFECTION), 20, EMO_LOVE,    EMO_LOVE,   1},
  {"mood",      MSG_MOOD,      N(MSG_MOOD),      20, EMO_HAPPY,   EMO_SHY,    0},
  {"playful",   MSG_PLAYFUL,   N(MSG_PLAYFUL),   14, EMO_WINK,    EMO_JOY,    2},
};
static const int NCATS = sizeof(CATS) / sizeof(CATS[0]);

// ---------------------------------------------------------------- selection with repeat avoidance
static const int HIST = 14;
static int16_t history[HIST];
static int histPos = 0;

void messagesBegin() { for (int i = 0; i < HIST; i++) history[i] = -1; histPos = 0; }

static int baseOf(int cat) { int b = 0; for (int i = 0; i < cat; i++) b += CATS[i].count; return b; }

int messagesTotalCount() { return baseOf(NCATS); }

const char *messagesAt(int idx) {
  for (int c = 0; c < NCATS; c++) { if (idx < CATS[c].count) return CATS[c].msgs[idx]; idx -= CATS[c].count; }
  return "";
}

static bool recent(int id) { for (int i = 0; i < HIST; i++) if (history[i] == id) return true; return false; }

MsgPick messagesPick() {
  int total = 0; for (int i = 0; i < NCATS; i++) total += CATS[i].weight;
  int cat = 0, idx = 0;
  for (int tries = 0; tries < 16; tries++) {
    int r = (int)rndi(0, total);
    for (cat = 0; cat < NCATS; cat++) { if (r < CATS[cat].weight) break; r -= CATS[cat].weight; }
    idx = (int)rndi(0, CATS[cat].count);
    if (!recent(baseOf(cat) + idx)) break;
  }
  history[histPos] = (int16_t)(baseOf(cat) + idx);
  histPos = (histPos + 1) % HIST;
  MsgPick p;
  p.text = CATS[cat].msgs[idx];
  p.category = (uint8_t)cat;
  p.emo = chance(60) ? CATS[cat].emoA : CATS[cat].emoB;
  p.icon = CATS[cat].icon;
  return p;
}

// ---------------------------------------------------------------- wrapping
int messagesWrap(const char *text, char lines[][MSG_LINE_CHARS + 2], int maxLines, int maxChars) {
  int nl = 0;
  const char *p = text;
  while (*p && nl < maxLines) {
    // one manual paragraph
    char para[48]; int pl = 0;
    while (*p && *p != '\n' && pl < 46) {
      if (p[0] == '<' && p[1] == '3') { para[pl++] = '\x03'; p += 2; }   // "<3" -> heart glyph (CP437 #3)
      else para[pl++] = *p++;
    }
    para[pl] = 0;
    if (*p == '\n') p++;
    // word-wrap the paragraph
    int i = 0;
    while (i < pl && nl < maxLines) {
      while (i < pl && para[i] == ' ') i++;
      int lineLen = 0; char *dst = lines[nl];
      while (i < pl) {
        int j = i; while (j < pl && para[j] != ' ') j++;
        int wl = j - i;
        if (lineLen == 0) {
          if (wl > maxChars) wl = maxChars, j = i + wl;           // hard split an overlong word
          memcpy(dst, para + i, wl); lineLen = wl; i = j;
        } else if (lineLen + 1 + wl <= maxChars) {
          dst[lineLen++] = ' '; memcpy(dst + lineLen, para + i, wl); lineLen += wl; i = j;
        } else break;
        while (i < pl && para[i] == ' ') i++;
      }
      dst[lineLen] = 0; nl++;
    }
  }
  return nl;
}
