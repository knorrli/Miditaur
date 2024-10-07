#include <cstdlib>
#include "Arduino.h"
#include "LedButton.h"
#include "Miditaur.h"

LedButton::LedButton(ButtonType _type, uint8_t _index, ButtonState _buttonState, LedState _ledState, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button *_button) {
  type = _type;
  buttonState = _buttonState;
  ledState = _ledState;
  index = _index;
  preset = 0;
  buttonPin = _buttonPin;
  ledPin = _ledPin;
  button = _button;
}

void LedButton::setup(int debounceDelay) {
  button->attach(buttonPin, INPUT_PULLUP);
  button->interval(debounceDelay);
  button->setPressedState(LOW);
  preset = loadPreset();
  

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.print("Led Button ");
  Serial.print(buttonPin);
  Serial.print(" READY, LED: ");
  Serial.print(ledPin);
  Serial.print(", button.getPin: ");
  Serial.println(button->getPin());
}

bool LedButton::pressed() {
  button->update();
  // Serial.println(button->getPin());
  bool pressed = button->pressed();
  // Serial.print("PRESSED CHECK: ");
  // Serial.println(pressed);
  return pressed;
}

void LedButton::setButtonState(ButtonState _state) {
  buttonState = _state;
}
void LedButton::setLedState(LedState _state) {
  ledState = _state;
}
void LedButton::setPreset(uint8_t _preset) {
  Serial.print("Fav");
  Serial.print(index);
  Serial.print(" => Writing Preset: ");
  Serial.println(preset);
  EEPROM.write(index, _preset);
  preset = _preset;
}

uint8_t LedButton::loadPreset() {
  uint8_t loadedPreset = EEPROM.read(index);
  Serial.print("FAV ");
  Serial.print(index);
  Serial.print(" => LOADED PRESET ");
  Serial.println(loadedPreset);
  if (loadedPreset) {
    return loadedPreset;
  } else {
    return 0;
  }
}