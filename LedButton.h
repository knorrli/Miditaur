#ifndef LedButton_h
#define LedButton_h

#include "Arduino.h"
#include "Miditaur.h"
#include <Bounce2.h>
#include <EEPROM.h>

class LedButton {
public:
  LedButton(ButtonType _type, uint8_t _index, ButtonState _buttonState, LedState _ledState, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button* _button);
  ButtonType type;
  ButtonState buttonState;
  LedState ledState;
  uint8_t index;
  uint8_t preset;
  uint8_t buttonPin;
  uint8_t ledPin;
  Bounce2::Button* button;

  void setup(int debounceDelay);
  bool pressed();
  void setButtonState(ButtonState _state);
  void setLedState(LedState _state);
  void setPreset(uint8_t _preset);

private:
  uint8_t loadPreset();
};

#endif