#include <cstdlib>
#include "Arduino.h"
#include "Miditaur.h"
#include "LedButton.h"

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

  loadPreset();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

bool LedButton::pressed() {
  button->update();
  return button->pressed();
}

void LedButton::transitionTo(Global global) {
  transitionCB(this, global);
}

void LedButton::activate(Global global) {
  activateCB(this, global);
}

void LedButton::switchBank(uint8_t _bank) {
  bank = _bank;
  loadPreset();
}

void LedButton::assignPreset(uint8_t _preset) {
  EEPROM.write(memoryAddress(), _preset);
  preset = _preset;
}

// PRIVATE

uint8_t LedButton::memoryAddress() {
  return (bank * 10) + index;
}

void LedButton::loadPreset() {
  uint8_t loadedPreset = EEPROM.read(memoryAddress());
  if (loadedPreset) {
    preset = loadedPreset;
  } else {
    preset = 0;
  }
}