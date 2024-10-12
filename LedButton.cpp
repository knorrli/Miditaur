#include <cstdlib>
#include "Arduino.h"
#include "Miditaur.h"
#include "LedButton.h"

LedButton::LedButton(ButtonType _type, uint8_t _index, bool _supportContinuousPress, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button* _button) {
  type = _type;
  buttonPin = _buttonPin;
  ledPin = _ledPin;
  index = _index;
  supportContinuousPress = _supportContinuousPress;
  preset = 0;
  ledState = LED_OFF;
  button = _button;
}

void LedButton::setup(int debounceDelay) {
  button->attach(buttonPin, INPUT_PULLUP);
  button->interval(debounceDelay);
  button->setPressedState(LOW);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void LedButton::switchToBank(uint8_t bank) {
  loadPreset(bank);
}

bool LedButton::hasPreset() {
  return (preset != 255);
}

void LedButton::assignPreset(uint8_t bank, uint8_t _preset) {
  uint8_t address = memoryAddress(bank, index);
  Serial.print("Assigning Preset");
  Serial.print(_preset);
  Serial.print(" to FAV");
  Serial.print(index);
  Serial.print(" in Bank: ");
  Serial.print(bank);
  Serial.print(", Address: ");
  Serial.println(address);
  EEPROM.write(address, _preset);
  preset = _preset;
}

void LedButton::loadPreset(uint8_t bank) {
  uint8_t address = memoryAddress(bank, index);
  Serial.print("Loading Preset for FAV");
  Serial.print(index);
  Serial.print(" from Bank: ");
  Serial.print(bank);
  Serial.print(", Address: ");
  Serial.println(address);
  uint8_t loadedPreset = EEPROM.read(address);
  if (loadedPreset) {
    preset = loadedPreset;
  } else {
    preset = 255;
  }
}

void LedButton::clearPreset(uint8_t bank) {
  uint8_t address = memoryAddress(bank, index);
  Serial.print("Clearing Preset for FAV");
  Serial.print(index);
  Serial.print(" from Bank: ");
  Serial.print(bank);
  Serial.print(", Address: ");
  Serial.println(address);
  EEPROM.write(address, 255);
  preset = 255;
}

uint8_t LedButton::memoryAddress(uint8_t bank, uint8_t index) {
  return (bank * 10) + index;
}