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
#define NUM_MENUS 2
#define NUM_BANKS NUM_FAV_BUTTONS

#define MIN_BANK 1
#define MAX_BANK 5
#define INITIAL_BANK MIN_BANK
#define MIN_PRESET 0
#define MAX_PRESET 127
#define INITIAL_PRESET MIN_PRESET
#define BLINK_INTERVAL 250
#define INITIAL_CONTINUOUS_PRESS_DELAY 750
#define CONTINUOUS_PRESS_INTERVAL 100
#define TRANSITION_TEXT_DURATION 1000

USBHost myusb;
MIDIDevice midiDevice(myusb);

bool deviceConnected = false;
uint8_t midiChannel = 1;
unsigned long previousBlinkMillis = 0;
unsigned long lastContinuousPress = 0;
unsigned long lastTransitionTextStart = 0;
bool blinkState = HIGH;

String textNoAction = "---";
String transitionText = "";
ProgramState nextState = STATE_IDLE;
uint8_t displaySegments[NUM_DIGITS];

void displayBank();
void OnConnect();
void OnDisconnect();
void OnProgramChange(uint8_t channel, uint8_t preset);

SevSeg display;
ProgramState state = STATE_IDLE;

uint8_t activeBank = INITIAL_BANK;
uint8_t activePreset = INITIAL_PRESET;
uint8_t activeMenuIndex = 0;
uint8_t activeMenuItemIndex = 0;

MenuItem emptyMenuItem = { .text = "ERR", .active = false, .value = 0 };

MenuItem filterAttack = { .text = "ATT", .active = true, .value = 64 };
MenuItem filterDecay = { .text = "DEC", .active = true, .value = 64 };
MenuItem filterSustain = { .text = "SUS", .active = true, .value = 64 };
MenuItem filterRelease = { .text = "REL", .active = true, .value = 64 };
Submenu filterMenu = {
  .text = "FIL",
  .items = { &filterAttack, &filterDecay, &filterSustain, &filterRelease, &emptyMenuItem }
};

MenuItem ampAttack = { .text = "ATT", .active = true, .value = 64 };
MenuItem ampDecay = { .text = "DEC", .active = true, .value = 64 };
MenuItem ampSustain = { .text = "SUS", .active = true, .value = 64 };
MenuItem ampRelease = { .text = "REL", .active = true, .value = 64 };
Submenu ampMenu = {
  .text = "AnP",
  .items = { &ampAttack, &ampDecay, &ampSustain, &ampRelease, &emptyMenuItem }
};
Submenu* menus[NUM_MENUS] = { &filterMenu, &ampMenu };

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

LedButton Fav1 = LedButton(TYPE_FAV, 1, false, PIN_BUTTON_FAV_1, PIN_LED_FAV_1, &fav1Button);
LedButton Fav2 = LedButton(TYPE_FAV, 2, false, PIN_BUTTON_FAV_2, PIN_LED_FAV_2, &fav2Button);
LedButton Fav3 = LedButton(TYPE_FAV, 3, false, PIN_BUTTON_FAV_3, PIN_LED_FAV_3, &fav3Button);
LedButton Fav4 = LedButton(TYPE_FAV, 4, false, PIN_BUTTON_FAV_4, PIN_LED_FAV_4, &fav4Button);
LedButton Fav5 = LedButton(TYPE_FAV, 5, false, PIN_BUTTON_FAV_5, PIN_LED_FAV_5, &fav5Button);
LedButton Bank = LedButton(TYPE_BANK, 6, false, PIN_BUTTON_BANK, PIN_LED_BANK, &bankButton);
LedButton Prev = LedButton(TYPE_PREV, 7, true, PIN_BUTTON_PREV, PIN_LED_PREV, &prevButton);
LedButton Next = LedButton(TYPE_NEXT, 8, true, PIN_BUTTON_NEXT, PIN_LED_NEXT, &nextButton);
LedButton Menu = LedButton(TYPE_MENU, 9, false, PIN_BUTTON_MENU, PIN_LED_MENU, &menuButton);
LedButton Save = LedButton(TYPE_SAVE, 10, false, PIN_BUTTON_SAVE, PIN_LED_SAVE, &saveButton);

LedButton* activeButton;
LedButton* favButtons[NUM_FAV_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5 };
LedButton* ledButtons[NUM_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5, &Bank, &Prev, &Next, &Menu, &Save };
String names[NUM_BUTTONS] = { "Fav1", "Fav2", "Fav3", "Fav4", "Fav5", "Bank", "Prev", "Next", "Menu", "Save" };
String programStates[] = { "ERROR", "IDLE", "SELECT_BANK", "ASSIGN_PRESET", "CLEAR_PRESET", "MENU", "MENU_ITEM" };

void setup() {
  Serial.begin(9600);
  delay(1500);  // Safe setup

  // myusb.begin();
  // midiDevice.setHandleProgramChange(OnProgramChange);

  digitalWrite(LED_BUILTIN, HIGH);

  setupDisplay();

  switchToBank(activeBank);
  switchToPreset(activePreset);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setup(DEBOUNCE_DELAY);
    handleTransition(ledButton, STATE_IDLE);
  }
  display.setNumber(activePreset);

  Serial.println("READY");
}

void loop() {
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

    if (continuousPress(ledButton) || ledButton->button->pressed()) {
      nextState = handlePress(ledButton, state);
      activeButton = ledButton;

      if (!transitionText.equals("")) {
        display.getSegments(displaySegments);
        char transitionChars[4];
        transitionText.toCharArray(transitionChars, 4);
        display.setChars(transitionChars);
        lastTransitionTextStart = millis();
      }
      break;
    }
  }

  if (activeButton && activeButton->button->released()) {
    updateState(nextState);
    // Serial.print("Transition BUTTONS to ");
    // Serial.println(programStates[state]);
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      LedButton* ledButton = ledButtons[i];
      handleTransition(ledButton, state);
    }
    activeButton = NULL;
  }

  if (!transitionText.equals("") && (millis() - lastTransitionTextStart >= TRANSITION_TEXT_DURATION)) {
    transitionText = "";
    display.setSegments(displaySegments);
  }


  render();
}

void prepareBlinkState() {
  if (millis() - previousBlinkMillis >= BLINK_INTERVAL) {
    previousBlinkMillis = millis();

    if (blinkState == LOW) {
      blinkState = HIGH;
    } else {
      blinkState = LOW;
    }
  }
}

void render() {
  prepareBlinkState();

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


void handleTransition(LedButton* button, ProgramState state) {
  switch (state) {
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
    case STATE_MENU:
      onTransitionToMenu(button);
      break;
    case STATE_MENU_ITEM:
      onTransitionToMenuItem(button);
      break;
    default:
      Serial.print("TRANSITION NOT HANDLED: ");
      Serial.println(state);
      break;
  }
}

ProgramState handlePress(LedButton* button, ProgramState state) {
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
    case STATE_MENU:
      return handleMenuPress(button);
    case STATE_MENU_ITEM:
      return handleMenuItemPress(button);
    default:
      Serial.print("PRESS NOT HANDLED: ");
      Serial.println(programStates[state]);
      display.setChars("ERR");
      return STATE_ERROR;
  }
}

void onTransitionToIdle(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset == activePreset) {
        button->ledState = LED_BLINKING;
      } else {
        if (button->hasPreset()) {
          button->ledState = LED_ON;
        } else {
          button->ledState = LED_OFF;
        }
      }
      break;
    case TYPE_BANK:
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_MENU:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
      break;
  }
}

ProgramState handleIdlePress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->hasPreset()) {
        switchToPreset(button->preset);
      } else {
        transitionText = textNoAction;
      }
      return STATE_IDLE;
    case TYPE_BANK:
      displayBank(activeBank);
      return STATE_SELECT_BANK;
    case TYPE_PREV:
      switchToPreset(activePreset - 1);
      return STATE_IDLE;
    case TYPE_NEXT:
      switchToPreset(activePreset + 1);
      return STATE_IDLE;
    case TYPE_MENU:
      {
        if (button->button->isPressed() && Save.button->isPressed()) {
          display.setChars("CLR");
          Serial.println("SWITCH TO CLEAR");
          return STATE_CLEAR_PRESET;
        }
        activeMenuIndex = 0;
        Submenu* menu = menus[activeMenuIndex];
        display.setChars(menu->text);
        return STATE_MENU;
      }
    case TYPE_SAVE:
      transitionText = "SET";
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
      if (button->hasPreset()) {
        button->ledState = LED_ON;
      } else {
        button->ledState = LED_BLINKING;
      }
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_ON;
      break;
    case TYPE_BANK:
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
      button->assignPreset(activeBank, activePreset);
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_PREV:
      switchToBank(activeBank - 1);
      transitionText = "B" + String(activeBank);
      switchToPreset(activePreset);
      return state;
    case TYPE_NEXT:
      switchToBank(activeBank - 1);
      transitionText = "B" + String(activeBank);
      switchToPreset(activePreset);
      return state;
    case TYPE_SAVE:
      transitionText = textNoAction;
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
    case TYPE_MENU:
      return state;
    default:
      return STATE_ERROR;
  }
}

void onTransitionToClearPreset(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->hasPreset()) {
        button->ledState = LED_BLINKING;
      } else {
        button->ledState = LED_OFF;
      }
      break;
    case TYPE_BANK:
    case TYPE_PREV:
    case TYPE_NEXT:
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
    case TYPE_PREV:
      switchToBank(activeBank - 1);
      return state;
    case TYPE_NEXT:
      switchToBank(activeBank + 1);
      return state;
    case TYPE_SAVE:
      transitionText = textNoAction;
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
    case TYPE_MENU:
      return state;
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
    case TYPE_PREV:
      switchToBank(activeBank - 1);
      return state;
    case TYPE_NEXT:
      switchToBank(activeBank + 1);
      return state;
    case TYPE_BANK:
    case TYPE_MENU:
    case TYPE_SAVE:
      switchToPreset(activePreset);
      transitionText = textNoAction;
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  };
}

void onTransitionToMenu(LedButton* button) {
  Submenu* menu = menus[activeMenuIndex];
  switch (button->type) {
    case TYPE_FAV:
      {
        MenuItem* buttonMenuItem = menu->items[button->index - 1];
        if (buttonMenuItem->active) {
          button->ledState = LED_ON;
        } else {
          button->ledState = LED_OFF;
        }
        break;
      }
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_MENU:
      button->ledState = LED_ON;
      break;
    case TYPE_BANK:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
  }
}

ProgramState handleMenuPress(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      {
        Submenu* menu = menus[activeMenuIndex];
        MenuItem* buttonMenuItem = menu->items[button->index - 1];
        if (buttonMenuItem->active) {
          activeMenuItemIndex = button->index - 1;
          display.setNumber(buttonMenuItem->value);
          transitionText = buttonMenuItem->text;
          return STATE_MENU_ITEM;
        } else {
          return state;
        }
      }
    case TYPE_BANK:
      return state;
      break;
    case TYPE_PREV:
      switchToMenu(activeMenuIndex - 1);
      return STATE_MENU;

    case TYPE_NEXT:
      switchToMenu(activeMenuIndex + 1);
      return STATE_MENU;
    case TYPE_MENU:
      switchToMenu(activeMenuIndex + 1);
      return STATE_MENU;
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
    default:
      return STATE_ERROR;
  };
}

void onTransitionToMenuItem(LedButton* button) {
  Submenu* menu = menus[activeMenuIndex];

  switch (button->type) {
    case TYPE_FAV:
      {
        MenuItem* buttonMenuItem = menu->items[button->index - 1];
        if ((button->index - 1) == activeMenuItemIndex) {
          button->ledState = LED_BLINKING;
        } else {
          if (buttonMenuItem->active) {
            button->ledState = LED_ON;
          } else {
            button->ledState = LED_OFF;
          }
        }
        break;
      }
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_BLINKING;
      break;
    case TYPE_MENU:
      button->ledState = LED_ON;
      break;
    case TYPE_BANK:
    case TYPE_SAVE:
      button->ledState = LED_OFF;
  }
}

ProgramState handleMenuItemPress(LedButton* button) {
  Submenu* menu = menus[activeMenuIndex];
  switch (button->type) {
    case TYPE_FAV:
      {
        if ((button->index - 1) == activeMenuItemIndex) {
          display.setChars(menu->text);
          return STATE_MENU;
        } else {
          MenuItem* buttonMenuItem = menu->items[button->index - 1];
          Serial.print("Switching to menu item: ");
          Serial.println(buttonMenuItem->text);
          if (buttonMenuItem->active) {
            activeMenuItemIndex = button->index - 1;
            display.setNumber(buttonMenuItem->value);
            transitionText = buttonMenuItem->text;
          }
          return STATE_MENU_ITEM;
        }
      }
    case TYPE_PREV:
      {
        MenuItem* activeMenuItem = menu->items[activeMenuItemIndex];
        if (activeMenuItem->value > 0) {
          activeMenuItem->value = activeMenuItem->value - 1;
          display.setNumber(activeMenuItem->value);
        }
        return STATE_MENU_ITEM;
      }
    case TYPE_NEXT:
      {
        MenuItem* activeMenuItem = menu->items[activeMenuItemIndex];
        if (activeMenuItem->value < 127) {
          activeMenuItem->value = activeMenuItem->value + 1;
          display.setNumber(activeMenuItem->value);
        }
        return STATE_MENU_ITEM;
      }
    case TYPE_MENU:
      {
        display.setChars(menu->text);
        return STATE_MENU;
      }
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
    default:
      return state;
  };
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

void switchToPreset(int16_t newPreset) {
  if (newPreset > MAX_PRESET) {
    newPreset = MIN_PRESET;
  } else if (newPreset < MIN_PRESET) {
    newPreset = MAX_PRESET;
  }

  if (activePreset != newPreset) {
    activePreset = newPreset;
    midiDevice.sendProgramChange(midiChannel, activePreset);
  }
  Serial.print("Switching to Preset: ");
  Serial.println(activePreset);
  display.setNumber(activePreset);
}

void switchToBank(int8_t newBank) {
  if (newBank > MAX_BANK) {
    activeBank = MIN_BANK;
  } else if (newBank < MIN_BANK) {
    activeBank = MAX_BANK;
  } else {
    activeBank = newBank;
  }
  Serial.print("Switching to Bank: ");
  Serial.println(activeBank);
  for (uint8_t i = 0; i < NUM_FAV_BUTTONS; i++) {
    LedButton* favButton = favButtons[i];
    favButton->switchToBank(activeBank);
  }
  displayBank(activeBank);
}

void switchToMenu(int8_t newMenuIndex) {
  if (newMenuIndex >= NUM_MENUS) {
    activeMenuIndex = 0;
  } else if (newMenuIndex < 0) {
    activeMenuIndex = NUM_MENUS - 1;
  } else {
    activeMenuIndex = newMenuIndex;
  }
  Submenu* newMenu = menus[activeMenuIndex];
  Serial.print("Switching to Menu: ");
  Serial.println(activeMenuIndex);
  display.setChars(newMenu->text);
}

void displayBank(uint8_t bank) {
  char bankString[5];
  sprintf(bankString, "B%hhu", bank);
  display.setChars(bankString);
}

bool continuousPress(LedButton* button) {
  if (!button->supportContinuousPress || !button->button->isPressed()) {
    return false;
  }
  unsigned long currentMillis = millis();
  if ((button->button->currentDuration() > INITIAL_CONTINUOUS_PRESS_DELAY) && (currentMillis - lastContinuousPress >= CONTINUOUS_PRESS_INTERVAL)) {
    lastContinuousPress = millis();
    return true;
  } else {
    return false;
  }
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
