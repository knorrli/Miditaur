#ifndef LedButton_h
#define LedButton_h

#include "Arduino.h"
#include <Bounce2.h>
#include <EEPROM.h>

#include "Miditaur.h"

class LedButton {
private:
  uint8_t memoryAddress(uint8_t bank, uint8_t index);

public:
  LedButton(ButtonType _type, uint8_t _index, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button* _button);
  uint8_t type;
  uint8_t index;
  uint8_t bank;
  uint8_t preset;
  uint8_t buttonPin;
  uint8_t ledPin;
  LedState ledState;
  Bounce2::Button* button;

  void setup(int debounceDelay);

  void switchToBank(uint8_t bank);
  void assignPreset(uint8_t bank, uint8_t _preset);
  void loadPreset(uint8_t bank);
  void clearPreset(uint8_t bank);
};

#endif