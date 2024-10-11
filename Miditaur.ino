#include <string.h>
#include "USBHost_t36.h"
#include <Bounce2.h>
#include "SevSeg.h"

#include "Miditaur.h"
#include "LedButton.h"

#define PIN_LED_FAV_1 24
#define PIN_LED_FAV_2 25
#define PIN_LED_FAV_3 26
#define PIN_LED_FAV_4 27
#define PIN_LED_FAV_5 28
#define PIN_LED_BANK 37
#define PIN_LED_PREV 38
#define PIN_LED_NEXT 41
#define PIN_LED_MENU 40
#define PIN_LED_SAVE 39

#define PIN_BUTTON_FAV_1 19
#define PIN_BUTTON_FAV_2 20
#define PIN_BUTTON_FAV_3 21
#define PIN_BUTTON_FAV_4 22
#define PIN_BUTTON_FAV_5 23
#define PIN_BUTTON_BANK 16
#define PIN_BUTTON_PREV 18
#define PIN_BUTTON_NEXT 14
#define PIN_BUTTON_MENU 15
#define PIN_BUTTON_SAVE 17

#define DEBOUNCE_DELAY 5

#define NUM_DIGITS 3
#define NUM_FAV_BUTTONS 5
#define NUM_BUTTONS NUM_FAV_BUTTONS + 5
#define NUM_MENUS 1

#define MIN_BANK 1
#define MAX_BANK 5
#define INITIAL_BANK MIN_BANK
#define MIN_PRESET 1
#define MAX_PRESET 128
#define INITIAL_PRESET MIN_PRESET
#define BLINK_INTERVAL 250

USBHost myusb;
MIDIDevice midiDevice(myusb);

bool deviceConnected = false;
uint8_t midiChannel = 1;
unsigned long previousBlinkMillis = 0;
bool blinkState = HIGH;

String textNoAction = "---";
String transitionText = "";
ProgramState nextState = STATE_IDLE;
uint8_t displaySegments[NUM_DIGITS];

void setupDisplay();
void prepareBlinkState();
void displayBank();
void render();
void switchToBank(uint8_t bank);
void switchToPreset(uint8_t preset);
void OnConnect();
void OnDisconnect();
void OnProgramChange(uint8_t channel, uint8_t preset);

SevSeg display;
ProgramState state = STATE_IDLE;

uint8_t activeBank = INITIAL_BANK;
uint8_t activePreset = INITIAL_PRESET;

MenuItem filterMenu[5] = {
  { .nextState = STATE_FILTER_ATTACK, .text = "ATT" },
  { .nextState = STATE_FILTER_DECAY, .text = "DEC" },
  { .nextState = STATE_FILTER_SUSTAIN, .text = "SUS" },
  { .nextState = STATE_FILTER_RELEASE, .text = "REL" },
  { .nextState = STATE_ERROR, .text = "" },
};

Bounce2::Button fav1Button = Bounce2::Button();
Bounce2::Button fav2Button = Bounce2::Button();
Bounce2::Button fav3Button = Bounce2::Button();
Bounce2::Button fav4Button = Bounce2::Button();
Bounce2::Button fav5Button = Bounce2::Button();
Bounce2::Button bankButton = Bounce2::Button();
Bounce2::Button prevButton = Bounce2::Button();
Bounce2::Button nextButton = Bounce2::Button();
Bounce2::Button menuButton = Bounce2::Button();
Bounce2::Button saveButton = Bounce2::Button();

LedButton Fav1 = LedButton(TYPE_FAV, 1, PIN_BUTTON_FAV_1, PIN_LED_FAV_1, &fav1Button);
LedButton Fav2 = LedButton(TYPE_FAV, 2, PIN_BUTTON_FAV_2, PIN_LED_FAV_2, &fav2Button);
LedButton Fav3 = LedButton(TYPE_FAV, 3, PIN_BUTTON_FAV_3, PIN_LED_FAV_3, &fav3Button);
LedButton Fav4 = LedButton(TYPE_FAV, 4, PIN_BUTTON_FAV_4, PIN_LED_FAV_4, &fav4Button);
LedButton Fav5 = LedButton(TYPE_FAV, 5, PIN_BUTTON_FAV_5, PIN_LED_FAV_5, &fav5Button);
LedButton Bank = LedButton(TYPE_BANK, 6, PIN_BUTTON_BANK, PIN_LED_BANK, &bankButton);
LedButton Prev = LedButton(TYPE_PREV, 7, PIN_BUTTON_PREV, PIN_LED_PREV, &prevButton);
LedButton Next = LedButton(TYPE_NEXT, 8, PIN_BUTTON_NEXT, PIN_LED_NEXT, &nextButton);
LedButton Menu = LedButton(TYPE_MENU, 9, PIN_BUTTON_MENU, PIN_LED_MENU, &menuButton);
LedButton Save = LedButton(TYPE_SAVE, 10, PIN_BUTTON_SAVE, PIN_LED_SAVE, &saveButton);

LedButton* activeButton;
LedButton* favButtons[NUM_FAV_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5 };
LedButton* ledButtons[NUM_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5, &Bank, &Prev, &Next, &Menu, &Save };
String names[NUM_BUTTONS] = { "Fav1", "Fav2", "Fav3", "Fav4", "Fav5", "Bank", "Prev", "Next", "Menu", "Save" };
String programStates[] = { "ERROR", "IDLE", "SELECT_BANK", "ASSIGN_PRESET", "CLEAR_PRESET", "MENU_FILTER", "FILTER_ATTACK", "FILTER_DECAY", "FILTER_SUSTAIN", "FILTER_RELEASE" };

void setup() {
  Serial.begin(9600);
  delay(1500);  // Safe setup

  myusb.begin();
  midiDevice.setHandleProgramChange(OnProgramChange);

  digitalWrite(LED_BUILTIN, HIGH);

  setupDisplay();

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setup(DEBOUNCE_DELAY);
    handleTransition(ledButton, STATE_IDLE);
  }
  display.setNumber(activePreset);

  switchToBank(activeBank);
  switchToPreset(activePreset);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    onTransitionToIdle(ledButton);
  }
  Serial.println("READY");
}

void loop() {
  prepareBlinkState();

  myusb.Task();
  midiDevice.read();

  if (deviceConnected && !midiDevice) {
    OnDisconnect();
  } else if (!deviceConnected && midiDevice) {
    OnConnect();
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->button->update();
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];

    if (ledButton->button->pressed()) {
      Serial.print("PRESS: ");
      Serial.println(ledButton->index);

      nextState = handlePress(ledButton, state);

      activeButton = ledButton;
      Serial.print("Active Button: ");
      Serial.println(activeButton->index);

      if (!transitionText.equals("")) {
        display.getSegments(displaySegments);
        char transitionChars[4];
        transitionText.toCharArray(transitionChars, 4);
        display.setChars(transitionChars);
      }
      break;
    }
  }

  if (activeButton && activeButton->button->released()) {
    Serial.print("Released Button ");
    Serial.println(activeButton->index);

    if (!transitionText.equals("")) {
      transitionText = "";
      display.setSegments(displaySegments);
    }
    updateState(nextState);
    Serial.print("Transition BUTTONS to ");
    Serial.println(programStates[state]);
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      LedButton* ledButton = ledButtons[i];
      handleTransition(ledButton, state);
    }
    activeButton = NULL;
  }


  render();
}

void prepareBlinkState() {
  unsigned long currentBlinkMillis = millis();
  if (currentBlinkMillis - previousBlinkMillis >= BLINK_INTERVAL) {
    previousBlinkMillis = currentBlinkMillis;

    if (blinkState == LOW) {
      blinkState = HIGH;
    } else {
      blinkState = LOW;
    }
  }
}

void render() {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    switch (ledButton->ledState) {
      case LED_ON:
        digitalWrite(ledButton->ledPin, HIGH);
        break;
      case LED_BLINKING:
        digitalWrite(ledButton->ledPin, blinkState);
        break;
      default:
        if (ledButton->button->isPressed()) {
          digitalWrite(ledButton->ledPin, HIGH);
        } else {
          digitalWrite(ledButton->ledPin, LOW);
        }
    }
  }

  display.refreshDisplay();
}

ProgramState handlePress(LedButton* button, ProgramState state) {
  Serial.print("Press: ");
  Serial.print(names[button->index - 1]);
  Serial.print(", in State: ");
  Serial.println(programStates[state]);
  switch (state) {
    case STATE_ERROR:
      Serial.println("ERROR");
      display.setChars("ERR");
      return STATE_ERROR;
    case STATE_IDLE:
      return handleIdlePress(button);
    case STATE_SELECT_BANK:
      return handleSelectBankPress(button);
    case STATE_ASSIGN_PRESET:
      return handleAssignPresetPress(button);
    case STATE_CLEAR_PRESET:
      return handleClearPresetPress(button);
    case STATE_MENU_FILTER:
      return handleMenuFilterPress(button);
    case STATE_FILTER_ATTACK:
      return handleFilterAttachPress(button);
    case STATE_FILTER_DECAY:
      return handleFilterDecayPress(button);
    case STATE_FILTER_SUSTAIN:
      return handleFilterSustainPress(button);
    case STATE_FILTER_RELEASE:
      return handleFilterReleasePress(button);
    case STATE_MENU_AMP:
      return handleMenuAmpPress(button);
    case STATE_FILTER_ATTACK:
      return handleAmpAttachPress(button);
    case STATE_FILTER_DECAY:
      return handleAmpDecayPress(button);
    case STATE_FILTER_SUSTAIN:
      return handleAmpSustainPress(button);
    case STATE_FILTER_RELEASE:
      return handleAmpReleasePress(button);
    default:
      Serial.print("PRESS NOT HANDLED: ");
      Serial.println(programStates[state]);
      display.setChars("ERR");
      return STATE_ERROR;
  }
}

void handleTransition(LedButton* button, ProgramState state) {
  switch (state) {
    case STATE_ERROR:
      display.setChars("ERR");
      break;
    case STATE_IDLE:
      onTransitionToIdle(button);
      break;
    case STATE_SELECT_BANK:
      onTransitionToSelectBank(button);
      break;
    case STATE_ASSIGN_PRESET:
      onTransitionToAssignPreset(button);
      break;
    case STATE_CLEAR_PRESET:
      onTransitionToClearPreset(button);
      break;
    case STATE_MENU_FILTER:
      onTransitionToMenuFilter(button);
      break;
    case STATE_FILTER_ATTACK:
      onTransitionToMenuItem(button, filterMenu, 1);
      break;
    case STATE_FILTER_DECAY:
      onTransitionToMenuItem(button, filterMenu, 2);
      break;
    case STATE_FILTER_SUSTAIN:
      onTransitionToMenuItem(button, filterMenu, 3);
      break;
    case STATE_FILTER_RELEASE:
      onTransitionToMenuItem(button, filterMenu, 4);
      break;
    default:
      Serial.print("TRANSITION NOT HANDLED: ");
      Serial.println(state);
      break;
  }
}

void onTransitionToIdle(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset == activePreset) {
        button->ledState = LED_BLINKING;
      } else {
        if (button->preset) {
          button->ledState = LED_ON;
        } else {
          button->ledState = LED_OFF;
        }
      }
      break;
    case TYPE_BANK:
      button->ledState = LED_OFF;
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_OFF;
      break;
    case TYPE_MENU:
      button->ledState = LED_OFF;
      break;
    case TYPE_SAVE:
      button->ledState = LED_OFF;
      break;
  }
}

ProgramState handleIdlePress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset) {
        switchToPreset(button->preset);
      } else {
        transitionText = textNoAction;
      }
      return STATE_IDLE;
    case TYPE_BANK:
      displayBank(activeBank);
      return STATE_SELECT_BANK;
    case TYPE_PREV:
      if (activePreset == MIN_PRESET) {
        switchToPreset(MAX_PRESET);
      } else {
        switchToPreset(activePreset - 1);
      }
      return STATE_IDLE;
    case TYPE_NEXT:
      switchToPreset(activePreset + 1);
      if (activePreset == MAX_PRESET) {
        switchToPreset(MIN_PRESET);
      } else {
        switchToPreset(activePreset + 1);
      }
      return STATE_IDLE;
    case TYPE_MENU:
      if (button->button->isPressed() && Save.button->isPressed()) {
        display.setChars("CLR");
        Serial.println("SWITCH TO CLEAR");
        return STATE_CLEAR_PRESET;
      }
      display.setChars("FIL");
      return STATE_MENU_FILTER;
    case TYPE_SAVE:
      transitionText = "ASS";
      if (button->button->isPressed() && Menu.button->isPressed()) {
        display.setChars("CLR");
        Serial.println("SWITCH TO CLEAR");
        return STATE_CLEAR_PRESET;
      }
      return STATE_ASSIGN_PRESET;
    default:
      return STATE_ERROR;
  }
}

void onTransitionToAssignPreset(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset) {
        button->ledState = LED_ON;
      } else {
        button->ledState = LED_BLINKING;
      }
      break;
    case TYPE_BANK:
      button->ledState = LED_OFF;
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_BLINKING;
      break;
    case TYPE_MENU:
      button->ledState = LED_OFF;
      break;
    case TYPE_SAVE:
      button->ledState = LED_BLINKING;
      break;
  }
}

ProgramState handleAssignPresetPress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      Serial.print("Assigning Preset to: ");
      Serial.print(names[button->index - 1]);
      Serial.print(", Preset: ");
      Serial.println(activePreset);
      button->assignPreset(activeBank, activePreset);
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
      return state;
      break;
    case TYPE_PREV:
      if (activeBank == MIN_BANK) {
        switchToBank(MAX_BANK);
      } else {
        switchToBank(activeBank - 1);
      }
      transitionText = "B" + String(activeBank);
      switchToPreset(activePreset);
      return state;
    case TYPE_NEXT:
      if (activeBank == MAX_BANK) {
        switchToBank(MIN_BANK);
      } else {
        switchToBank(activeBank + 1);
      }
      transitionText = "B" + String(activeBank);
      switchToPreset(activePreset);
      return state;
    case TYPE_MENU:
      return state;
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  }
}

void onTransitionToClearPreset(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset) {
        button->ledState = LED_BLINKING;
      } else {
        button->ledState = LED_OFF;
      }
      break;
    case TYPE_BANK:
      button->ledState = LED_OFF;
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_OFF;
      break;
    case TYPE_MENU:
      button->ledState = LED_OFF;
      break;
    case TYPE_SAVE:
      button->ledState = LED_BLINKING;
      break;
  }
}

ProgramState handleClearPresetPress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      Serial.print("Clearing Preset from: ");
      Serial.print(names[button->index - 1]);
      button->clearPreset(activeBank);
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
      return state;
    case TYPE_PREV:
      if (activeBank == MIN_BANK) {
        switchToBank(MAX_BANK);
      } else {
        switchToBank(activeBank - 1);
      }
      return state;
    case TYPE_NEXT:
      if (activeBank == MAX_BANK) {
        switchToBank(MIN_BANK);
      } else {
        switchToBank(activeBank + 1);
      }
      return state;
    case TYPE_MENU:
      return state;
      break;
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  }
}

void onTransitionToSelectBank(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->index == activeBank) {
        button->ledState = LED_BLINKING;
      } else {
        button->ledState = LED_ON;
      }
      break;
    case TYPE_BANK:
      button->ledState = LED_BLINKING;
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_MENU:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
      break;
  }
}

ProgramState handleSelectBankPress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      switchToBank(button->index);
      transitionText = "B" + String(activeBank);
      switchToPreset(activePreset);
      return STATE_IDLE;
      break;
    case TYPE_BANK:
      Serial.println("ABORT");
      switchToPreset(activePreset);
      return STATE_IDLE;
      break;
    case TYPE_PREV:
      if (activeBank == MIN_BANK) {
        switchToBank(MAX_BANK);
      } else {
        switchToBank(activeBank - 1);
      }
      return state;
    case TYPE_NEXT:
      if (activeBank == MAX_BANK) {
        switchToBank(MIN_BANK);
      } else {
        switchToBank(activeBank + 1);
      }
    case TYPE_MENU:
      transitionText = textNoAction;
      return state;
    case TYPE_SAVE:
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  };
}

void onTransitionToMenuFilter(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      {
        MenuItem menuItem = filterMenu[button->index - 1];
        if (menuItem.nextState != STATE_ERROR) {
          button->ledState = LED_ON;
        } else {
          button->ledState = LED_OFF;
        }
        break;
      }
    case TYPE_BANK:
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_MENU:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
      break;
  }
}

ProgramState handleMenuFilterPress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      {
        MenuItem menuItem = filterMenu[button->index - 1];
        if (menuItem.nextState != STATE_ERROR) {
          display.setChars(menuItem.text);
          return menuItem.nextState;
        } else {
          return state;
        }
      }
    case TYPE_BANK:
      return state;
      break;
    case TYPE_PREV:
      display.setChars("AnP");
      return STATE_MENU_AMP;
    case TYPE_NEXT:
      display.setChars("AnP");
      return STATE_MENU_AMP;
    case TYPE_MENU:
      display.setChars("AnP");
      return STATE_MENU_AMP;
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  };
}

void onTransitionToMenuItem(LedButton* button, MenuItem* menuItems, uint8_t activeButtonIndex) {
  switch (button->type) {
    case TYPE_FAV:
      {
        if (button->index == activeButtonIndex) {
          button->ledState = LED_BLINKING;
        } else {
          MenuItem menuItem = menuItems[button->index - 1];
          if (menuItem.nextState != STATE_ERROR) {
            button->ledState = LED_ON;
          } else {
            button->ledState = LED_OFF;
          }
        }
        break;
      }
    case TYPE_BANK:
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_MENU:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
      break;
  }
}

void setupDisplay() {
  byte numDigits = NUM_DIGITS;
  byte digitPins[] = { 10, 11, 12 };
  byte segmentPins[] = { 9, 6, 3, 2, 7, 8, 5, 4 };
  byte hardwareConfig = N_TRANSISTORS;  // See README.md for options
  bool resistorsOnSegments = true;      // 'false' means resistors are on digit pins

  bool updateWithDelays = false;  // Default 'false' is Recommended
  bool leadingZeros = true;       // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false;   // Use 'true' if your decimal point doesn't exist or isn't connected

  display.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros, disableDecPoint);
  display.setBrightness(100);
}

void updateState(ProgramState newState) {
  Serial.print("Transitioning to State to: ");
  Serial.println(programStates[newState]);
  state = newState;
}

// TODO: send MIDI
void switchToPreset(uint8_t preset) {
  Serial.print("Switching to Preset: ");
  Serial.println(preset);
  activePreset = preset;
  // midiDevice.sendProgramChange(midiChannel, preset);
  display.setNumber(activePreset - 1);
}

void switchToBank(uint8_t bank) {
  Serial.print("Switching to Bank: ");
  Serial.println(bank);
  activeBank = bank;
  for (uint8_t i = 0; i < NUM_FAV_BUTTONS; i++) {
    LedButton* favButton = favButtons[i];
    favButton->switchToBank(activeBank);
  }
  displayBank(bank);
}

void displayBank(uint8_t bank) {
  char bankString[5];
  sprintf(bankString, "B%hhu", bank);
  display.setChars(bankString);
}

void OnConnect() {
  deviceConnected = true;
  Serial.println("Connect");
}

void OnDisconnect() {
  deviceConnected = false;
  midiChannel = 0;
  activePreset = 0;
  Serial.println("Disconnect");
}

void OnProgramChange(uint8_t channel, uint8_t preset) {
  midiChannel = channel;
  // global.activePreset = preset; // TODO: handle non-IDLE states??
  Serial.print("Channel: ");
  Serial.print(channel);
  Serial.print(", Program: ");
  Serial.println(preset);
}
