#ifndef Miditaur_h
#define Miditaur_h

#include "Arduino.h"

enum ButtonType {
  TYPE_FAV,
  TYPE_PREV,
  TYPE_NEXT,
  TYPE_MENU,
  TYPE_SAVE
};

enum ButtonState {
  BUTTON_DISABLED,
  BUTTON_ENABLED,
  BUTTON_WRITABLE
};

enum LedState {
  LED_OFF,
  LED_ON,
  LED_PRESSED,
  LED_BLINKING
};

#endif