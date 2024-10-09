#include <cstdlib>
#include "Arduino.h"
#include "LedButton.h"
#include "States.h"

LedButton::LedButton(uint8_t _index, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button* _button, void (*_onTransition)(LedButton* button, Global global), void (*_onActivate)(LedButton* self, Global global)) {
  index = _index;
  preset = 0;
  buttonPin = _buttonPin;
  ledPin = _ledPin;
  button = _button;
  transitionCB = _onTransition;
  activateCB = _onActivate;
}

void LedButton::setup(int debounceDelay) {
  button->attach(buttonPin, INPUT_PULLUP);
  button->interval(debounceDelay);
  button->setPressedState(LOW);

  preset = loadPreset();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

bool LedButton::pressed() {
  button->update();
  bool pressed = button->pressed();
  return pressed;
}

void LedButton::transitionTo(Global global) {
  transitionCB(this, global);
}

void LedButton::activate(Global global) {
  activateCB(this, global);
}

bool LedButton::canActivate() {
  return nextState;
}

uint8_t LedButton::loadPreset() {
  uint8_t loadedPreset = EEPROM.read(index);
  if (loadedPreset) {
    return loadedPreset;
  } else {
    return 0;
  }
}