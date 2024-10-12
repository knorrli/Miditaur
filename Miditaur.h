#ifndef States_h
#define States_h

#include "SevSeg.h"

enum ProgramState {
  STATE_ERROR,
  
  STATE_IDLE,
  STATE_SELECT_BANK,
  STATE_ASSIGN_PRESET,
  STATE_CLEAR_PRESET,


  STATE_MENU,
  STATE_MENU_ITEM
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
  LED_BLINKING
};


struct MenuItem {
  char text[4];
  boolean active;
  uint8_t value;
};

struct Submenu {
  char text[4];
  MenuItem *items[5];
};

#endif