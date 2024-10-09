#include <string.h>
#include "USBHost_t36.h"
#include <Bounce2.h>
#include <EEPROM.h>
#include "SevSeg.h"

#include "States.h"
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

#define FAV_BUTTONS 6
#define NUM_BUTTONS FAV_BUTTONS + 4
#define NUM_MENUS 1
#define INITIAL_PRESET 1
#define MIN_BANK 1
#define MAX_BANK 5
#define MIN_PRESET 1
#define MAX_PRESET 128
#define BLINK_INTERVAL 250

USBHost myusb;
MIDIDevice midiDevice(myusb);

bool deviceConnected = false;
uint8_t midiChannel = 1;
uint8_t activePreset = 0;
uint8_t activeBank = 1;
unsigned long previousBlinkMillis = 0;
bool blinkState = HIGH;

void setupDisplay();
void prepareBlinkState();
void OnConnect();
void OnDisconnect();
void OnProgramChange(uint8_t channel, uint8_t preset);

SevSeg display;

MenuItem filterMenu[] = {
  { .nextState = STATE_FILTER_ATTACK, .text = "ATT" },
  { .nextState = STATE_FILTER_DECAY, .text = "DEC" },
  { .nextState = STATE_FILTER_SUSTAIN, .text = "SUS" },
  { .nextState = STATE_FILTER_RELEASE, .text = "REL" }
};
MenuItem ampMenu[5] = {
  { .nextState = STATE_FILTER_ATTACK, .text = "ATT" },
  { .nextState = STATE_FILTER_DECAY, .text = "DEC" },
  { .nextState = STATE_FILTER_SUSTAIN, .text = "SUS" },
  { .nextState = STATE_FILTER_RELEASE, .text = "REL" }
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

void favOnActivate(LedButton* button, Global global) {
  switch (global.state) {
    case STATE_ERROR:
    global.display->setChars("ERR");
      break;
    case STATE_IDLE:
      global.display->setNumber(button->preset);
      break;
    case STATE_ASSIGNING_FAV_PRESET:
      EEPROM.write(button->index, global.activePreset);
      button->preset = activePreset;
      global.display->setNumber(button->preset);
      break;
    case STATE_SELECTING_BANK:
      global.activeBank = button->index;
      break;
    case STATE_MENU_FILTER:
      break;
    default:
      break;
  }
}

void favTransitionTo(LedButton* button, Global global) {
  switch (global.state) {
    case STATE_ERROR:
      break;
    case STATE_IDLE:
      button->nextState = STATE_IDLE;
      break;
    case STATE_ASSIGNING_FAV_PRESET:
      button->nextState = STATE_IDLE;
      break;
    case STATE_SELECTING_BANK:
      button->nextState = STATE_IDLE;
      break;
    case STATE_MENU_FILTER:
      {
        MenuItem menuItem = filterMenu[button->index];
        if (menuItem.nextState) {
          button->nextState = menuItem.nextState;
        }
        break;
      }
    default:
      break;
  }
}

LedButton Fav1 = LedButton(1, PIN_BUTTON_FAV_1, PIN_LED_FAV_1, &fav1Button, &favTransitionTo, &favOnActivate);
LedButton Fav2 = LedButton(2, PIN_BUTTON_FAV_2, PIN_LED_FAV_2, &fav2Button, &favTransitionTo, &favOnActivate);
LedButton Fav3 = LedButton(3, PIN_BUTTON_FAV_3, PIN_LED_FAV_3, &fav3Button, &favTransitionTo, &favOnActivate);
LedButton Fav4 = LedButton(4, PIN_BUTTON_FAV_4, PIN_LED_FAV_4, &fav4Button, &favTransitionTo, &favOnActivate);
LedButton Fav5 = LedButton(5, PIN_BUTTON_FAV_5, PIN_LED_FAV_5, &fav5Button, &favTransitionTo, &favOnActivate);
LedButton Bank = LedButton(0, PIN_BUTTON_BANK, PIN_LED_BANK, &bankButton, NULL, NULL);
LedButton Prev = LedButton(0, PIN_BUTTON_PREV, PIN_LED_PREV, &prevButton, NULL, NULL);
LedButton Next = LedButton(0, PIN_BUTTON_NEXT, PIN_LED_NEXT, &nextButton, NULL, NULL);
LedButton Menu = LedButton(0, PIN_BUTTON_MENU, PIN_LED_MENU, &menuButton, NULL, NULL);
LedButton Save = LedButton(0, PIN_BUTTON_SAVE, PIN_LED_SAVE, &saveButton, NULL, NULL);

LedButton* ledButtons[NUM_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5, &Bank, &Prev, &Next, &Menu, &Save };

Global global = {
  .state = STATE_IDLE,
  .display = &display,
  .activePreset = MIN_PRESET,
  .activeBank = MIN_BANK
};

void setup() {
  Serial.begin(9600);
  delay(1500);  // Safe setup

  myusb.begin();
  midiDevice.setHandleProgramChange(OnProgramChange);

  digitalWrite(LED_BUILTIN, HIGH);

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setup(DEBOUNCE_DELAY);
  }

  setupDisplay();
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
    if (ledButton->pressed() && ledButton->canActivate()) {
      ledButton->activate(global);
      break;
    }
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->transitionTo(global);
    render(ledButton);
  }


  display.refreshDisplay();
}

void process(LedButton* ledButton) {
  // switch (programState) {
  //   case IDLE:
  //     handleIdle(ledButton);
  //     break;
  //   case BANK_SELECT:
  //     handleBankSelect(ledButton);
  //     break;
  //   case ASSIGNING:
  //     handleAssigning(ledButton);
  // }
}

// void handleIdle(LedButton* ledButton) {
//   switch (ledButton->type) {
//     case FAV:
//       break;
//     case BANK:
//       break;
//     case PREV:
//       break;
//     case NEXT:
//       break;
//     case MENU:
//       break;
//   }
// }

// void handleBankSelect(LedButton* ledButton) {
//   switch (ledButton->type) {
//     case FAV:
//       break;
//     case BANK:
//       break;
//     case PREV:
//       break;
//     case NEXT:
//       break;
//     case MENU:
//       break;
//   }
// }

// void handleAssigningFav(LedButton* ledButton) {
//   switch (ledButton->type) {
//     case FAV:
//       break;
//     case BANK:
//       break;
//     case PREV:
//       break;
//     case NEXT:
//       break;
//     case MENU:
//       break;
//   }
// }


//   Serial.print("PROCESS TYPE:");
//   Serial.println(ledButton->type);
//   switch (ledButton->type) {
//     case TYPE_FAV:
//       switch (ledButton->buttonState) {
//         case BUTTON_WRITABLE:
//           Serial.print("STORE PRESET: FAV");
//           ledButton->setPreset(activePreset);
//           transitionToPreset(ledButton->preset);
//           break;
//         case BUTTON_ENABLED:
//           transitionToPreset(ledButton->preset);
//           break;
//         default:
//           Serial.print("BUTTON DISABLED: FAV_");
//           Serial.println(ledButton->index);
//           break;
//       }
//       break;
//     case TYPE_BANK:
//       if (activePreset == MIN_PRESET) {
//         activePreset = MAX_PRESET;
//       } else {
//         activePreset--;
//       }
//     case TYPE_PREV:
//       if (activePreset == MIN_PRESET) {
//         activePreset = MAX_PRESET;
//       } else {
//         activePreset--;
//       }
//       transitionToPreset();
//       break;
//     case TYPE_NEXT:
//       if (activePreset == MAX_PRESET) {
//         activePreset = MIN_PRESET;
//       } else {
//         activePreset++;
//       }
//       transitionToPreset();
//       break;
//     case TYPE_MENU:
//       transitionToIdle();
//       break;
//     case TYPE_SAVE:
//       transitionToAssigning();
//       break;
//     default:
//       break;
//   }
// }

// void transitionToAssigning() {
//   Serial.println("Transition to ASSIGNING");
//   char bank[3] = { 98, activeBank };
//   display.setChars(bank);

//   for (uint8_t i = 0; i < FAV_BUTTONS; i++) {
//     LedButton* ledButton = ledButtons[i];
//     ledButton->setButtonState(BUTTON_WRITABLE);
//     if (ledButton->preset) {
//       ledButton->setLedState(LED_ON);
//     } else {
//       ledButton->setLedState(LED_BLINKING);
//     }
//   }
//   Prev.setButtonState(BUTTON_DISABLED);
//   Prev.setLedState(LED_OFF);

//   Next.setButtonState(BUTTON_DISABLED);
//   Next.setLedState(LED_OFF);

//   Menu.setButtonState(BUTTON_ENABLED);
//   Menu.setLedState(LED_ON);

//   Save.setButtonState(BUTTON_DISABLED);
//   Save.setLedState(LED_OFF);
// }

// void transitionToPreset(uint8_t preset) {
//   activePreset = preset;
//   Serial.print("Transition to Preset: ");
//   Serial.println(activePreset);

//   if (deviceConnected) {
//     midiDevice.sendProgramChange(activePreset, midiChannel);
//   }

//   transitionToIdle();
// }

// void transitionToIdle() {
//   Serial.println("Transition to IDLE");
//   for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
//     LedButton* ledButton = ledButtons[i];
//     ledButton->setButtonState(BUTTON_ENABLED);
//     ledButton->setLedState(LED_OFF);

//     if (activePreset) {
//       display.setNumber(activePreset - 1);

//       if ((ledButton->type == TYPE_FAV) && (ledButton->preset == activePreset)) {
//         ledButton->setLedState(LED_ON);
//       }
//     }
//   }
//   Prev.setLedState(LED_PRESSED);
//   Next.setLedState(LED_PRESSED);
// }

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

void render(LedButton* ledButton) {
  switch (ledButton->ledState) {
    case LED_ON:
      digitalWriteFast(ledButton->ledPin, HIGH);
      break;
    case LED_BLINKING:
      digitalWriteFast(ledButton->ledPin, blinkState);
      break;
    case LED_PRESSED:
      if (ledButton->button->isPressed()) {
        digitalWriteFast(ledButton->ledPin, HIGH);
      } else {
        digitalWriteFast(ledButton->ledPin, LOW);
      }
      break;
    default:
      digitalWriteFast(ledButton->ledPin, LOW);
  }
}

void setupDisplay() {
  byte numDigits = 3;
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
  activePreset = preset;
  Serial.print("Channel: ");
  Serial.print(channel);
  Serial.print(", Program: ");
  Serial.println(preset);
}
