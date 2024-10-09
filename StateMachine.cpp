#include "Miditaur.h"
#include "LedButton.h"

MenuItem filterMenu[5] = {
  { .nextState = STATE_FILTER_ATTACK, .text = "ATT" },
  { .nextState = STATE_FILTER_DECAY, .text = "DEC" },
  { .nextState = STATE_FILTER_SUSTAIN, .text = "SUS" },
  { .nextState = STATE_FILTER_RELEASE, .text = "REL" },
  { .nextState = STATE_ERROR, .text = "" }
};
MenuItem ampMenu[5] = {
  { .nextState = STATE_FILTER_ATTACK, .text = "ATT" },
  { .nextState = STATE_FILTER_DECAY, .text = "DEC" },
  { .nextState = STATE_FILTER_SUSTAIN, .text = "SUS" },
  { .nextState = STATE_FILTER_RELEASE, .text = "REL" },
  { .nextState = STATE_ERROR, .text = "" }
};

void favOnActivate(LedButton* button, Global global) {
  switch (global.state) {
    case STATE_ERROR:
      global.display->setChars("ERR");  // TODO
      break;
    case STATE_IDLE:
    case STATE_BANK_CHANGED:
      global.display->setNumber(button->preset);
      global.state = STATE_IDLE;
      break;
    case STATE_ASSIGNING_FAV_PRESET:
      button->assignPreset(global.activePreset);
      global.display->setNumber(button->preset);
      global.state = STATE_IDLE;
      break;
    case STATE_SELECTING_BANK:
      {
        global.activeBank = button->index;
        char bank[3] = { 98, global.activeBank };
        global.display->setChars(bank);
        global.state = STATE_BANK_CHANGED;
        break;
      }
    case STATE_MENU_FILTER:
      {
        MenuItem menuItem = filterMenu[button->index];
        if (menuItem.nextState != STATE_ERROR) {
          button->ledState = LED_BLINKING;
          global.state = menuItem.nextState;
        }
        break;
      }
      break;
    case STATE_FILTER_ATTACK:
    case STATE_FILTER_DECAY:
    case STATE_FILTER_SUSTAIN:
    case STATE_FILTER_RELEASE:
      button->ledState = LED_ON;
      global.state = STATE_MENU_FILTER;
      break;
    case STATE_MENU_AMP:
      {
        MenuItem menuItem = ampMenu[button->index];
        if (menuItem.nextState != STATE_ERROR) {
          button->ledState = LED_BLINKING;
          global.state = menuItem.nextState;
        }
        break;
      }
      break;
    case STATE_AMP_ATTACK:
    case STATE_AMP_DECAY:
    case STATE_AMP_SUSTAIN:
    case STATE_AMP_RELEASE:
      button->ledState = LED_ON;
      global.state = STATE_MENU_AMP;
      break;
    default:
      break;
  }
}

void favTransitionTo(LedButton* button, Global global) {
  switch (global.state) {
    case STATE_ERROR:
      break;
    case STATE_IDLE:
      if (button->preset == global.activePreset) {
        button->ledState = LED_ON;
      } else {
        button->ledState = LED_OFF;
      }
      break;
    case STATE_ASSIGNING_FAV_PRESET:
      if (button->preset) {
        button->ledState = LED_ON;
      } else {
        button->ledState = LED_BLINKING;
      }
      break;
    case STATE_SELECTING_BANK:
      button->ledState = LED_BLINKING;
      break;
    case STATE_MENU_FILTER:
      {
        MenuItem menuItem = filterMenu[button->index];
        if (menuItem.nextState) {
          button->ledState = LED_ON;
        } else {
          button->ledState = LED_OFF;
        }
        break;
      }
    case STATE_FILTER_ATTACK:
    case STATE_FILTER_SUSTAIN:
    case STATE_FILTER_DECAY:
    case STATE_FILTER_RELEASE:
      break;
    case STATE_AMP_ATTACK:
    case STATE_AMP_SUSTAIN:
    case STATE_AMP_DECAY:
    case STATE_AMP_RELEASE:
      break;
    default:
      break;
  }
}


void bankOnActivate(LedButton* button, Global global) {
}
void bankTransitionTo(LedButton* button, Global global) {
}