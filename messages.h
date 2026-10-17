#pragma once
// ============================================================
// Local, configurable message collection. Edit messages.cpp to add / change messages.
// Rules of thumb: max 4 lines, max 13 characters per line, '\n' = manual line break,
// "<3" is drawn as a heart glyph. Messages are small gifts: positive, no diagnoses.
// ============================================================
#include <Arduino.h>
#include "emotion.h"

#define MSG_MAX_LINES 4
#define MSG_LINE_CHARS 13

struct MsgPick {
  const char *text;
  uint8_t     category;
  Emo         emo;       // how the mascot reacts
  uint8_t     icon;      // 0 none, 1 heart, 2 sparkle
};

void    messagesBegin();
MsgPick messagesPick();                       // weighted category choice, avoids recent repeats
// Splits text into lines (manual '\n' first, then word-wrap). Returns number of lines.
int     messagesWrap(const char *text, char lines[][MSG_LINE_CHARS + 2], int maxLines, int maxChars);
int     messagesTotalCount();
const char *messagesAt(int idx);              // for validation / debugging
