#ifndef LedButton_h
#define LedButton_h

#include "Arduino.h"
#include <Bounce2.h>
#include <EEPROM.h>

#include "Miditaur.h"

class LedButton {
private:
  void (*transitionCB)(LedButton*, Global);
  void (*activateCB)(LedButton*, Global);
  uint8_t memoryAddress();
  void loadPreset();
public:
  LedButton(uint8_t _index, uint8_t _buttonPin, uint8_t _ledPin, Bounce2::Button* _button, void (*_transitionCB)(LedButton* self, Global global), void (*_activateCB)(LedButton* self, Global global));
  uint8_t index;
  uint8_t preset;
  uint8_t buttonPin;
  uint8_t ledPin;
  uint8_t bank;
  ButtonState buttonState;
  LedState ledState;
  Bounce2::Button* button;

  void setup(int debounceDelay);
  bool pressed();
  void transitionTo(Global global);
  void activate(Global global);
  void assignPreset(uint8_t preset);
  void switchBank(uint8_t bank);
};

#endif