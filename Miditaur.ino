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
#define STARTUP_ANIMATION_DURATION 35

#define NUM_DIGITS 3
#define NUM_FAV_BUTTONS 5
#define NUM_BUTTONS NUM_FAV_BUTTONS + 5
#define NUM_MENUS 7
#define NUM_MENU_ITEMS NUM_MENUS* NUM_FAV_BUTTONS
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
#define STARTUP_ANIMATION_STEPS 19

#define SYSEX_SEND_CC_SIZE 7
#define SYSEX_SAVE_PANEL_SIZE 17


#define NAME_FILTER "FIL"
#define NAME_AMP "AMP"
#define NAME_ERROR "ERR"
#define NAME_ATTACK "ATT"
#define NAME_DECAY "DEC"
#define NAME_SUSTAIN "SUS"
#define NAME_RELEASE "REL"
#define NAME_AMP_TRIGGER_MODE "LEG"

#define NAME_KEYBOARD "KBD"
#define NAME_KEY_FILTER_VELOCITY "FIL"
#define NAME_KEY_AMP_VELOCITY "AMP"
#define NAME_KEY_TRACKING "TRA"
#define NAME_KEY_PRIORITY "PRI"

#define NAME_OSCILLATOR "OSC"
#define NAME_OSC_BEAT_FREQUENCY "FRQ"
#define NAME_OSC_NOTE_SYNC "SYN"
#define NAME_OSC_HARD_SYNC "HRD"
#define NAME_OSC_2_MOD "MOD"

#define NAME_LFO "LFO"
#define NAME_LFO_WAVEFORM "TYP"
#define NAME_LFO_KEY_TRIGGER "TRG"

#define NAME_PITCH_BEND "BND"
#define NAME_PITCH_BEND_UP "UP"
#define NAME_PITCH_BEND_DOWN "DN"

#define NAME_GLIDE "GLI"
#define NAME_GLIDE_SWITCH "SET"
#define NAME_GLIDE_RATE "TIM"
#define NAME_GLIDE_TYPE "TPE"
#define NAME_GLIDE_LEGATO "LEG"

USBHost myusb;
MIDIDevice midiDevice(myusb);

bool deviceConnected = false;
uint8_t midiChannel = 1;
bool displayingControlChange = false;
bool midiReceived = false;
bool blinkState = HIGH;
unsigned long previousBlinkMillis = 0;
unsigned long lastContinuousPress = 0;
unsigned long lastTransitionTextStart = 0;
unsigned long lastControlChangeReceivedMillis = 0;
//7-segment map:
//   AAA
//  F   B
//  F   B
//   GGG
//  E   C
//  E   C
//   DDD   H
// HGFEDCBA
uint8_t startupAnimationDigit = 2;
uint8_t startupAnimationStep = STARTUP_ANIMATION_STEPS;
uint8_t animation[18] = {
  0b01000000,  // PLACEHOLDER
  0b01000000,  // G
  0b01000010,  // BG
  0b00000010,  // B
  0b00000011,  // AB
  0b00000001,  // A
  0b00100001,  // FA
  0b00100000,  // F
  0b01100000,  // GF
  0b01000000,  // G
  0b01000100,  // CG
  0b00000100,  // C
  0b00001100,  // DC
  0b00001000,  // D
  0b00011000,  // ED
  0b00010000,  // E
  0b01010000,  // GE
  0b01000000,  // G
};
String textAbortAction = "NOT";
String textConfirmAction = "YES";
String transitionText = "";
ProgramState nextState = STATE_IDLE;
uint8_t displaySegments[NUM_DIGITS];

SevSeg display;
ProgramState state = STATE_IDLE;

uint8_t activeBank = INITIAL_BANK;
uint8_t activePreset = INITIAL_PRESET;
uint8_t activeMenuIndex = 0;
uint8_t activeMenuItemIndex = 0;

uint8_t sysExTriggerSendCC[SYSEX_SEND_CC_SIZE] = { 0xF0, 0x04, 0x08, 0x1B, 0x00, 0x00, 0xF7 };
uint8_t sysExTriggerSavePanel[SYSEX_SAVE_PANEL_SIZE] = { 0xF0, 0x04, 0x08, 0x06, 0x07, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7 };

MenuItem emptyMenuItem = { .text = NAME_ERROR, .midiCC = 0, .active = false, .value = 0, .displayMin = 0, .displayMax = 0 };

MenuItem filterAttack = { .text = NAME_ATTACK, .midiCC = 22, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem filterDecay = { .text = NAME_DECAY, .midiCC = 24, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem filterSustain = { .text = NAME_SUSTAIN, .midiCC = 25, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem filterRelease = { .text = NAME_RELEASE, .midiCC = 24, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
Submenu filterMenu = {
  .text = NAME_FILTER,
  .items = { &filterAttack, &filterDecay, &filterSustain, &filterRelease, &emptyMenuItem }
};

MenuItem ampAttack = { .text = NAME_ATTACK, .midiCC = 28, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem ampDecay = { .text = NAME_DECAY, .midiCC = 29, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem ampSustain = { .text = NAME_SUSTAIN, .midiCC = 30, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem ampRelease = { .text = NAME_RELEASE, .midiCC = 29, .active = true, .value = 64, .displayMin = 0, .displayMax = 127 };
MenuItem ampTriggerMode{ .text = NAME_AMP_TRIGGER_MODE, .midiCC = 73, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
Submenu ampMenu = {
  .text = NAME_AMP,
  .items = { &ampAttack, &ampDecay, &ampSustain, &ampRelease, &ampTriggerMode }
};

MenuItem keyFilterVelocity = { .text = NAME_KEY_FILTER_VELOCITY, .midiCC = 89, .active = true, .value = 64, .displayMin = 0, .displayMax = 100 };
MenuItem keyAmpVelocity = { .text = NAME_KEY_AMP_VELOCITY, .midiCC = 90, .active = true, .value = 64, .displayMin = 0, .displayMax = 100 };
MenuItem keyTracking = { .text = NAME_KEY_TRACKING, .midiCC = 20, .active = true, .value = 32, .displayMin = 0, .displayMax = 200 };
MenuItem keyPriority = { .text = NAME_KEY_PRIORITY, .midiCC = 91, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
Submenu keyMenu = {
  .text = NAME_KEYBOARD,
  .items = { &keyFilterVelocity, &keyAmpVelocity, &keyTracking, &keyPriority, &emptyMenuItem }
};

MenuItem oscBeatFrequency = { .text = NAME_OSC_BEAT_FREQUENCY, .midiCC = 18, .active = true, .value = 64, .displayMin = -50, .displayMax = 50 };
MenuItem oscNoteSync = { .text = NAME_OSC_NOTE_SYNC, .midiCC = 81, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem oscHardSync = { .text = NAME_OSC_HARD_SYNC, .midiCC = 80, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem osc2Mod = { .text = NAME_OSC_2_MOD, .midiCC = 112, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
Submenu oscMenu = {
  .text = NAME_OSCILLATOR,
  .items = { &oscBeatFrequency, &oscNoteSync, &oscHardSync, &osc2Mod, &emptyMenuItem }
};

MenuItem lfoShape = { .text = NAME_LFO_WAVEFORM, .midiCC = 85, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem lfoKeyTrigger = { .text = NAME_LFO_KEY_TRIGGER, .midiCC = 82, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
Submenu lfoMenu = {
  .text = NAME_LFO,
  .items = { &lfoShape, &lfoKeyTrigger, &osc2Mod, &emptyMenuItem, &emptyMenuItem }
};

MenuItem pitchBendUp = { .text = NAME_PITCH_BEND_UP, .midiCC = 107, .active = true, .value = 32, .displayMin = 0, .displayMax = 127 };
MenuItem pitchBendDown = { .text = NAME_PITCH_BEND_DOWN, .midiCC = 108, .active = true, .value = 32, .displayMin = 0, .displayMax = 127 };
Submenu pitchendMenu = {
  .text = NAME_PITCH_BEND,
  .items = { &pitchBendUp, &pitchBendDown, &emptyMenuItem, &emptyMenuItem, &emptyMenuItem }
};

MenuItem glideSwitch = { .text = NAME_GLIDE_SWITCH, .midiCC = 65, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem glideRate = { .text = NAME_GLIDE_RATE, .midiCC = 5, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem glideType = { .text = NAME_GLIDE_TYPE, .midiCC = 92, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
MenuItem glideLegato = { .text = NAME_GLIDE_LEGATO, .midiCC = 83, .active = true, .value = 0, .displayMin = 0, .displayMax = 127 };
Submenu glideMenu = {
  .text = NAME_GLIDE,
  .items = { &glideSwitch, &glideRate, &glideType, &glideLegato, &emptyMenuItem }
};

MenuItem* menuItems[NUM_MENU_ITEMS] = {
  &filterAttack, &filterDecay, &filterSustain, &filterRelease, &emptyMenuItem,
  &ampAttack, &ampDecay, &ampSustain, &ampRelease, &emptyMenuItem,
  &keyFilterVelocity, &keyAmpVelocity, &keyTracking, &keyPriority, &emptyMenuItem,
  &oscBeatFrequency, &oscNoteSync, &oscHardSync, &osc2Mod, &emptyMenuItem,
  &lfoShape, &lfoKeyTrigger, &osc2Mod, &emptyMenuItem, &emptyMenuItem,
  &pitchBendUp, &pitchBendDown, &emptyMenuItem, &emptyMenuItem, &emptyMenuItem,
  &glideSwitch, &glideType, &emptyMenuItem, &emptyMenuItem, &emptyMenuItem
};
Submenu* menus[NUM_MENUS] = { &filterMenu, &ampMenu, &keyMenu, &oscMenu, &lfoMenu, &pitchendMenu, &glideMenu };

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

  delay(1000);  // Safe setup

  myusb.begin();
  midiDevice.setHandleProgramChange(onProgramChange);
  midiDevice.setHandleControlChange(onControlChange);
  midiDevice.setHandleSystemExclusive(onSystemExclusiveChunk);
  midiDevice.setHandleSystemExclusive(onSystemExclusive);

  setupDisplay();

  digitalWrite(LED_BUILTIN, HIGH);



  switchToBank(activeBank);
  switchToPreset(activePreset);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setup(DEBOUNCE_DELAY);
    handleTransition(ledButton, STATE_IDLE);
  }

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

  if (startupAnimationStep > 0) {
    displayStartupAnimation();
    return;
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->button->update();
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];

    if (continuousPress(ledButton) || ledButton->button->pressed()) {
      transitionText = "";
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

  // restore segments saved before displaying CC value
  if (displayingControlChange && (millis() - lastControlChangeReceivedMillis >= TRANSITION_TEXT_DURATION)) {
    Serial.print("RESETTING DISPLAY");
    displayingControlChange = false;


    if (state == STATE_MENU_ITEM) {
      Submenu* menu = menus[activeMenuIndex];
      MenuItem* activeMenuItem = menu->items[activeMenuItemIndex];
      display.setNumber(activeMenuItem->value);
    } else {
      display.setSegments(displaySegments);
    }
  }


  if (midiReceived || (activeButton && activeButton->button->released())) {
    updateState(nextState);
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      LedButton* ledButton = ledButtons[i];
      handleTransition(ledButton, state);
    }
    activeButton = NULL;
    midiReceived = false;
  }

  if (!transitionText.equals("") && (millis() - lastTransitionTextStart >= TRANSITION_TEXT_DURATION)) {
    transitionText = "";
    display.setSegments(displaySegments);
  }

  render();
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
    case STATE_STORE_PRESET:
      onTransitionToStorePreset(button);
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
      Serial.print("STATE NOT HANDLED");
      Serial.println(state);
      display.setChars("ERR");
  }
}

ProgramState handlePress(LedButton* button, ProgramState state) {
  switch (state) {
    case STATE_IDLE:
      return handleIdlePress(button);
    case STATE_SELECT_BANK:
      return handleSelectBankPress(button);
    case STATE_ASSIGN_PRESET:
      return handleAssignPresetPress(button);
    case STATE_STORE_PRESET:
      return handleStorePresetPress(button);
    case STATE_CLEAR_PRESET:
      return handleClearPresetPress(button);
    case STATE_MENU:
      return handleMenuPress(button);
    case STATE_MENU_ITEM:
      return handleMenuItemPress(button);
    default:
      Serial.print("STATE NOT HANDLED");
      Serial.println(state);
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
        transitionText = textAbortAction;
      }
      return STATE_IDLE;
    case TYPE_PREV:
      switchToPreset(activePreset - 1);
      return STATE_IDLE;
    case TYPE_NEXT:
      switchToPreset(activePreset + 1);
      return STATE_IDLE;
    case TYPE_BANK:
      {
        if (button->button->isPressed() && Save.button->isPressed()) {
          display.setChars("STO");
          Serial.println("SWITCH TO STORE");
          return STATE_STORE_PRESET;
        }
        displayBank(activeBank);
        return STATE_SELECT_BANK;
      }

    case TYPE_MENU:
      {
        if (button->button->isPressed() && Save.button->isPressed()) {
          display.setChars("CLR");
          Serial.println("SWITCH TO CLEAR");
          return STATE_CLEAR_PRESET;
        }
        Serial.println("HANDLE MENU PRESS IN IDLE");
        activeMenuIndex = 0;
        sendSysEx(sysExTriggerSendCC, SYSEX_SEND_CC_SIZE);
        Submenu* menu = menus[activeMenuIndex];
        display.setChars(menu->text);
        return STATE_MENU;
      }
    case TYPE_SAVE:
      if (button->button->isPressed() && Menu.button->isPressed()) {
        display.setChars("CLR");
        Serial.println("SWITCH TO CLEAR");
        return STATE_CLEAR_PRESET;
      }
      if (button->button->isPressed() && Bank.button->isPressed()) {
        display.setChars("STO");
        Serial.println("SWITCH TO STORE");
        return STATE_STORE_PRESET;
      }
      transitionText = "SET";
      return STATE_ASSIGN_PRESET;
  }
  return STATE_ERROR;
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
      transitionText = textAbortAction;
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
    case TYPE_MENU:
      return state;
  }
  return STATE_ERROR;
}

void onTransitionToStorePreset(LedButton* button) {
  switch (button->type) {
    case TYPE_FAV:
      if (button->preset == activePreset) {
        button->ledState = LED_ON;
      } else {
        button->ledState = LED_OFF;
      }
      break;
    case TYPE_BANK:
    case TYPE_PREV:
    case TYPE_NEXT:
      button->ledState = LED_OFF;
      break;
    case TYPE_MENU:
    case TYPE_SAVE:
      button->ledState = LED_BLINKING;
      break;
  }
}

ProgramState handleStorePresetPress(LedButton* button) {
  switch (button->type) {
    case TYPE_MENU:
      transitionText = textConfirmAction;
      sysExTriggerSavePanel[5] = activePreset;
      sendSysEx(sysExTriggerSavePanel, SYSEX_SAVE_PANEL_SIZE);
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_SAVE:
      transitionText = textAbortAction;
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_FAV:
    case TYPE_PREV:
    case TYPE_NEXT:
    case TYPE_BANK:
      return state;
  }
  return STATE_ERROR;
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
      transitionText = textAbortAction;
      switchToPreset(activePreset);
      return STATE_IDLE;
    case TYPE_BANK:
    case TYPE_MENU:
      return state;
  }
  return STATE_ERROR;
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
      transitionText = textAbortAction;
      return STATE_IDLE;
  }
  return STATE_ERROR;
}

void onTransitionToMenu(LedButton* button) {
  Submenu* menu = menus[activeMenuIndex];
  switch (button->type) {
    case TYPE_FAV:
      {
        MenuItem* buttonMenuItem = menu->items[button->index - 1];
        if (buttonMenuItem->active) {
          button->ledState = LED_BLINKING;
        } else {
          button->ledState = LED_OFF;
        }
        break;
      }
    case TYPE_MENU:
      button->ledState = LED_ON;
      break;
    case TYPE_PREV:
    case TYPE_NEXT:
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
          displayParamValue(buttonMenuItem);
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
    case TYPE_MENU:
      switchToMenu(activeMenuIndex + 1);
      return STATE_MENU;
    case TYPE_SAVE:
      switchToPreset(activePreset);
      return STATE_IDLE;
  }
  return STATE_ERROR;
}

void onTransitionToMenuItem(LedButton* button) {
  Submenu* menu = menus[activeMenuIndex];

  switch (button->type) {
    case TYPE_FAV:
      {
        if ((button->index - 1) == activeMenuItemIndex) {
          button->ledState = LED_BLINKING;
        } else {
          MenuItem* buttonMenuItem = menu->items[button->index - 1];
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
          if (buttonMenuItem->active) {
            activeMenuItemIndex = button->index - 1;
            displayParamValue(buttonMenuItem);
            transitionText = buttonMenuItem->text;
          }
          return STATE_MENU_ITEM;
        }
      }
    case TYPE_BANK:
      return state;
    case TYPE_PREV:
      {
        MenuItem* activeMenuItem = menu->items[activeMenuItemIndex];
        updateControlParam(activeMenuItem, VALUE_DECREASE);
        return STATE_MENU_ITEM;
      }
    case TYPE_NEXT:
      {
        MenuItem* activeMenuItem = menu->items[activeMenuItemIndex];
        updateControlParam(activeMenuItem, VALUE_INCREASE);
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
  }
  return STATE_ERROR;
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
  // Serial.print("Transitioning to State: ");
  // Serial.println(programStates[newState]);
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
    // Serial.print("MIDI PC SEND!");
    // Serial.print(" Channel: ");
    // Serial.print(midiChannel);
    // Serial.print(", Preset: ");
    // Serial.println(activePreset);

    midiDevice.sendProgramChange(activePreset, midiChannel);
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

void updateControlParam(MenuItem* menuItem, ProgramValueChange valueChange) {
  if ((strcmp(menuItem->text, NAME_KEY_PRIORITY) == 0) || (strcmp(menuItem->text, NAME_AMP_TRIGGER_MODE) == 0) || strcmp(menuItem->text, NAME_GLIDE_TYPE) == 0) {
    if (valueChange == VALUE_DECREASE) {
      if (menuItem->value > 85) {
        menuItem->value = 85;
      } else if (menuItem->value > 42) {
        menuItem->value = 0;
      }
    } else {
      if (menuItem->value < 43) {
        menuItem->value = 43;
      } else if (menuItem->value < 86) {
        menuItem->value = 127;
      }
    }
  } else if (strcmp(menuItem->text, NAME_LFO_WAVEFORM) == 0) {
    if (valueChange == VALUE_DECREASE) {
      if (menuItem->value > 106) {
        menuItem->value = 106;
      } else if (menuItem->value > 85) {
        menuItem->value = 85;
      } else if (menuItem->value > 63) {
        menuItem->value = 63;
      } else if (menuItem->value > 42) {
        menuItem->value = 42;
      } else if (menuItem->value > 21) {
        menuItem->value = 21;
      } else {
        menuItem->value = 0;
      }
    } else {
      if (menuItem->value < 22) {
        menuItem->value = 22;
      } else if (menuItem->value < 43) {
        menuItem->value = 43;
      } else if (menuItem->value < 64) {
        menuItem->value = 64;
      } else if (menuItem->value < 86) {
        menuItem->value = 86;
      } else if (menuItem->value < 107) {
        menuItem->value = 107;
      } else {
        menuItem->value = 127;
      }
    }
  } else if ((strcmp(menuItem->text, NAME_PITCH_BEND_UP) == 0) || (strcmp(menuItem->text, NAME_PITCH_BEND_DOWN) == 0)) {
    if (valueChange == VALUE_DECREASE) {
      if (menuItem->value > 111) {
        menuItem->value = 111;
      } else if (menuItem->value > 95) {
        menuItem->value = 95;
      } else if (menuItem->value > 79) {
        menuItem->value = 79;
      } else if (menuItem->value > 63) {
        menuItem->value = 63;
      } else if (menuItem->value > 47) {
        menuItem->value = 47;
      } else if (menuItem->value > 31) {
        menuItem->value = 31;
      } else if (menuItem->value > 15) {
        menuItem->value = 0;
      }
    } else {
      if (menuItem->value < 16) {
        menuItem->value = 16;
      } else if (menuItem->value < 32) {
        menuItem->value = 32;
      } else if (menuItem->value < 48) {
        menuItem->value = 48;
      } else if (menuItem->value < 64) {
        menuItem->value = 64;
      } else if (menuItem->value < 80) {
        menuItem->value = 80;
      } else if (menuItem->value < 96) {
        menuItem->value = 96;
      } else if (menuItem->value < 112) {
        menuItem->value = 127;
      }
    }
  } else if (
    (strcmp(menuItem->text, NAME_OSC_NOTE_SYNC) == 0) || (strcmp(menuItem->text, NAME_OSC_HARD_SYNC) == 0) || (strcmp(menuItem->text, NAME_OSC_2_MOD) == 0) || (strcmp(menuItem->text, NAME_LFO_KEY_TRIGGER) == 0) || (strcmp(menuItem->text, NAME_GLIDE_SWITCH) == 0) || (strcmp(menuItem->text, NAME_GLIDE_LEGATO) == 0)) {
    if (valueChange == VALUE_DECREASE) {
      if (menuItem->value > 63) {
        menuItem->value = 0;
      }
    } else {
      if (menuItem->value < 64) {
        menuItem->value = 127;
      }
    }
  } else {
    int displayValue = mapToDisplayValue(menuItem);
    int newDisplayValue = displayValue;
    if (valueChange == VALUE_DECREASE) {
      while (newDisplayValue == displayValue && menuItem->value > 0) {
        menuItem->value = menuItem->value - 1;
        newDisplayValue = mapToDisplayValue(menuItem);
      }
    }
    if (valueChange == VALUE_INCREASE) {
      while (newDisplayValue == displayValue && menuItem->value < 127) {
        menuItem->value = menuItem->value + 1;
        newDisplayValue = mapToDisplayValue(menuItem);
      }
    }
  }

  displayParamValue(menuItem);
  // midiDevice.sendProgramChange(menuItem->midiCC, midiChannel);
}

void displayParamValue(MenuItem* menuItem) {
  if (strcmp(menuItem->text, NAME_KEY_PRIORITY) == 0) {
    if (menuItem->value > 85) {
      display.setChars("LAS");
    } else if (menuItem->value > 42) {
      display.setChars("HI");
    } else {
      display.setChars("LO");
    }
  } else if (strcmp(menuItem->text, NAME_AMP_TRIGGER_MODE) == 0) {
    if (menuItem->value > 85) {
      display.setChars("RST");
    } else if (menuItem->value > 42) {
      display.setChars("OFF");
    } else {
      display.setChars("ON");
    }
  } else if (strcmp(menuItem->text, NAME_GLIDE_TYPE) == 0) {
    if (menuItem->value > 85) {
      display.setChars("EXP");
    } else if (menuItem->value > 42) {
      display.setChars("TIM");
    } else {
      display.setChars("RTE");
    }
  } else if (strcmp(menuItem->text, NAME_LFO_WAVEFORM) == 0) {
    if (menuItem->value < 22) {
      display.setChars("TRI");
    } else if (menuItem->value < 43) {
      display.setChars("SQU");
    } else if (menuItem->value < 64) {
      display.setChars("FAL");
    } else if (menuItem->value < 86) {
      display.setChars("RIS");
    } else if (menuItem->value < 107) {
      display.setChars("RND");
    } else {
      display.setChars("ENV");
    }
  } else if ((strcmp(menuItem->text, NAME_PITCH_BEND_UP) == 0) || (strcmp(menuItem->text, NAME_PITCH_BEND_DOWN) == 0)) {
    if (menuItem->value < 16) {
      display.setChars("OFF");
    } else if (menuItem->value < 32) {
      display.setChars("  2");
    } else if (menuItem->value < 48) {
      display.setChars("  3");
    } else if (menuItem->value < 64) {
      display.setChars("  4");
    } else if (menuItem->value < 80) {
      display.setChars("  5");
    } else if (menuItem->value < 96) {
      display.setChars("  7");
    } else if (menuItem->value < 112) {
      display.setChars(" 12");
    } else {
      display.setChars(" 24");
    }
  } else if ((strcmp(menuItem->text, NAME_OSC_NOTE_SYNC) == 0) || (strcmp(menuItem->text, NAME_OSC_HARD_SYNC) == 0) || (strcmp(menuItem->text, NAME_OSC_2_MOD) == 0) || (strcmp(menuItem->text, NAME_LFO_KEY_TRIGGER) == 0) || (strcmp(menuItem->text, NAME_GLIDE_SWITCH) == 0) || (strcmp(menuItem->text, NAME_GLIDE_LEGATO) == 0)) {
    if (menuItem->value > 63) {
      display.setChars("ON");
    } else {
      display.setChars("OFF");
    }
  } else {
    display.setNumber(mapToDisplayValue(menuItem));
  }
}

int mapToDisplayValue(MenuItem* menuItem) {
  int newRange = (menuItem->displayMax - menuItem->displayMin);
  return ((menuItem->value * newRange) / 127) + menuItem->displayMin;
}

bool continuousPress(LedButton* button) {
  if (!button->supportContinuousPress || !button->button->isPressed()) {
    return false;
  }
  unsigned long currentMillis = millis();
  if ((button->button->currentDuration() > INITIAL_CONTINUOUS_PRESS_DELAY) && (currentMillis - lastContinuousPress >= CONTINUOUS_PRESS_INTERVAL)) {
    lastContinuousPress = millis();
    Serial.print("CONTINUOUS PRESS: ");
    Serial.println(names[button->index - 1]);
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
  activePreset = 255;
  Serial.println("Disconnect");
}

void onProgramChange(uint8_t channel, uint8_t preset) {
  midiReceived = true;
  midiChannel = channel;
  activePreset = preset;  // This way, we don't send back the MIDI PC Message
  switchToPreset(activePreset);
  Serial.print("RECEIVE PC! Channel: ");
  Serial.print(channel);
  Serial.print(", Program: ");
  Serial.println(preset);
}

void onControlChange(byte channel, byte control, byte value) {
  midiReceived = true;
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.println(value, DEC);

  MenuItem* modifiedMenuItem = NULL;
  for (uint8_t i = 0; i < NUM_MENU_ITEMS; i++) {
    MenuItem* menuItem = menuItems[i];
    if (menuItem->active && menuItem->midiCC == control) {
      modifiedMenuItem = menuItem;
    }
  }

  if (modifiedMenuItem != NULL) {
    Serial.print("FOUND MENU ITEM TO MODIFY: ");
    Serial.print(modifiedMenuItem->text);
    Serial.print(", CC: ");
    Serial.println(modifiedMenuItem->midiCC);

    modifiedMenuItem->value = value;
    // Store existing display value to revert after control change ends
    if (!displayingControlChange) {
      display.getSegments(displaySegments);
      displayingControlChange = true;
    }
    display.setNumber(modifiedMenuItem->value);
    lastControlChangeReceivedMillis = millis();
  } else {
    Serial.print("Control Change not handled: ");
    Serial.print(control);
  }
}

// This 3-input System Exclusive function is more complex, but allows you to
// process very large messages which do not fully fit within the midi1's
// internal buffer.  Large messages are given to you in chunks, with the
// 3rd parameter to tell you which is the last chunk.  This function is
// a Teensy extension, not available in the Arduino MIDI library.
//
void onSystemExclusiveChunk(const byte* data, uint16_t length, bool last) {
  midiReceived = true;
  Serial.print("SysEx Message (Chunked) (length=");
  Serial.print(length);
  Serial.print("): ");
  printBytes(data, length);
  if (last) {
    Serial.println(" (end)");
  } else {
    Serial.println(" (to be continued)");
  }
}

// This simpler 2-input System Exclusive function can only receive messages
// up to the size of the internal buffer.  Larger messages are truncated, with
// no way to receive the data which did not fit in the buffer.  If both types
// of SysEx functions are set, the 3-input version will be called by midi1.
//
void onSystemExclusive(byte* data, unsigned int length) {
  midiReceived = true;
  Serial.print("SysEx Message: ");
  printBytes(data, length);
  Serial.println();
}

void displayStartupAnimation() {
  if (millis() - lastTransitionTextStart > STARTUP_ANIMATION_DURATION) {
    uint8_t animationStep = animation[startupAnimationStep - 1];
    // printBin(animationStep);
    startupAnimationStep--;

    display.blank();
    lastTransitionTextStart = millis();

    display.setSegmentsDigit(startupAnimationDigit, animationStep);

    if (startupAnimationStep == 0) {
      if (startupAnimationDigit > 0) {
        startupAnimationDigit--;
        startupAnimationStep = STARTUP_ANIMATION_STEPS - 1;
        display.setSegmentsDigit(startupAnimationDigit, animationStep);
        Serial.println("RESET");
      } else {
        display.setNumber(activePreset);
        display.getSegments(displaySegments);
        transitionText = "RDY";
        char transitionChars[4];
        transitionText.toCharArray(transitionChars, 4);
        display.setChars(transitionChars);
        lastTransitionTextStart = millis();
      }
    }
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->button->update();
  }
  display.refreshDisplay();
}

void sendSysEx(const byte* data, unsigned int size) {
  Serial.print("SENDING SYSEX, (size=");
  Serial.print(size);
  Serial.println("): ");
  printBytes(data, size);
  Serial.println();


  Serial.println("DEBUG: CC");
  printBytes(sysExTriggerSendCC, 7);
  Serial.println("DEBUG: CC");
  printBytes(sysExTriggerSavePanel, 17);
  // midiDevice.sendSysEx(sizeof(data), data);
}

void printBytes(const byte* data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
  Serial.println();
}