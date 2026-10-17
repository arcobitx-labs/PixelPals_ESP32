#include "console.h"
#include "config.h"
#include "director.h"
#include "events.h"

#if DEBUG_SERIAL
static void help() {
  Serial.println(F("[console] l=love h=happy m=message t=transform g=glitch p=space r=random-rare"));
  Serial.println(F("          1=morph 2=power-up 3=heart-pulse 4=peek  s=sleep w=wake e=next emotion  x=game"));
}
void consolePoll(uint32_t now) {
  while (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case '?': help(); break;
      case 'l': directorRequest(EV_LOVE, PRI_USER, now); break;
      case 'h': directorRequest(EV_HAPPY, PRI_USER, now); break;
      case 'm': directorAction(ACT_MESSAGE, now); break;
      case 't': directorRequest(EV_TRANSFORM, PRI_USER, now); break;
      case 'g': directorRequest(EV_GLITCH, PRI_USER, now); break;
      case 'p': directorRequest(EV_SPACE, PRI_USER, now); break;
      case 'r': directorAction(ACT_RARE, now); break;
      case '1': directorRequest(EV_MORPH, PRI_USER, now); break;
      case '2': directorRequest(EV_POWERUP, PRI_USER, now); break;
      case '3': directorRequest(EV_HEARTPULSE, PRI_USER, now); break;
      case '4': directorRequest(EV_PEEK, PRI_USER, now); break;
      case 's': sleepSetForever(true); directorRequest(EV_SLEEP, PRI_USER, now); break;
      case 'w': sleepWake(); break;
      case 'e': directorCycleEmotion(now); break;
      case 'x': directorAction(ACT_GAME, now); break;
      default: break;
    }
  }
}
#else
void consolePoll(uint32_t) {}
#endif
