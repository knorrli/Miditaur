#ifndef States_h
#define States_h

#include "SevSeg.h"

enum ProgramState {
  STATE_ERROR,
  STATE_IDLE,
  STATE_ASSIGNING_FAV_PRESET,
  STATE_SELECTING_BANK,

  STATE_MENU_FILTER,
  STATE_FILTER_ATTACK,
  STATE_FILTER_DECAY,
  STATE_FILTER_SUSTAIN,
  STATE_FILTER_RELEASE,

  STATE_MENU_AMP,
  STATE_AMP_ATTACK,
  STATE_AMP_DECAY,
  STATE_AMP_SUSTAIN,
  STATE_AMP_RELEASE
};

enum ButtonType {
  TYPE_FAV,
  TYPE_BANK,
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

struct MenuItem {
  ProgramState nextState;
  char text[4];
};


struct Global {
  ProgramState state;
  SevSeg* display;
  uint8_t activePreset;
  uint8_t activeBank;
};

#endif