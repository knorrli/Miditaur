

#include <Bounce2.h>
#include <EEPROM.h>
#include "SevSeg.h"

#include "Miditaur.h"
#include "LedButton.h"


#define PIN_LED_FAV_1 24
#define PIN_LED_FAV_2 25
#define PIN_LED_FAV_3 26
#define PIN_LED_FAV_4 27
#define PIN_LED_FAV_5 28
#define PIN_LED_FAV_6 37
#define PIN_LED_PREV 38
#define PIN_LED_NEXT 41
#define PIN_LED_MENU 40
#define PIN_LED_SAVE 39

#define PIN_BUTTON_FAV_1 19
#define PIN_BUTTON_FAV_2 20
#define PIN_BUTTON_FAV_3 21
#define PIN_BUTTON_FAV_4 22
#define PIN_BUTTON_FAV_5 23
#define PIN_BUTTON_FAV_6 16
#define PIN_BUTTON_PREV 18
#define PIN_BUTTON_NEXT 14
#define PIN_BUTTON_MENU 15
#define PIN_BUTTON_SAVE 17

#define DEBOUNCE_DELAY 5

#define FAV_BUTTONS 6
#define NUM_BUTTONS FAV_BUTTONS + 4
// #define NUM_MENUS 1
#define MIN_PRESET 1
#define MAX_PRESET 128
#define BLINK_INTERVAL 250

void process(LedButton ledButton);
void transitionToAssigning();
void transitionToIdle();
void transitionToPrev();
void transitionToNext();
void transitionToFav(LedButton* ledButton);
void prepareBlinkState();

SevSeg display;

Bounce2::Button fav1Button = Bounce2::Button();
Bounce2::Button fav2Button = Bounce2::Button();
Bounce2::Button fav3Button = Bounce2::Button();
Bounce2::Button fav4Button = Bounce2::Button();
Bounce2::Button fav5Button = Bounce2::Button();
Bounce2::Button fav6Button = Bounce2::Button();
Bounce2::Button prevButton = Bounce2::Button();
Bounce2::Button nextButton = Bounce2::Button();
Bounce2::Button menuButton = Bounce2::Button();
Bounce2::Button saveButton = Bounce2::Button();

LedButton Fav1 = LedButton(TYPE_FAV, 1, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_1, PIN_LED_FAV_1, &fav1Button);
LedButton Fav2 = LedButton(TYPE_FAV, 2, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_2, PIN_LED_FAV_2, &fav2Button);
LedButton Fav3 = LedButton(TYPE_FAV, 3, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_3, PIN_LED_FAV_3, &fav3Button);
LedButton Fav4 = LedButton(TYPE_FAV, 4, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_4, PIN_LED_FAV_4, &fav4Button);
LedButton Fav5 = LedButton(TYPE_FAV, 5, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_5, PIN_LED_FAV_5, &fav5Button);
LedButton Fav6 = LedButton(TYPE_FAV, 6, BUTTON_ENABLED, LED_OFF, PIN_BUTTON_FAV_6, PIN_LED_FAV_6, &fav6Button);
LedButton Prev = LedButton(TYPE_PREV, 0, BUTTON_ENABLED, LED_PRESSED, PIN_BUTTON_PREV, PIN_LED_PREV, &prevButton);
LedButton Next = LedButton(TYPE_NEXT, 0, BUTTON_ENABLED, LED_PRESSED, PIN_BUTTON_NEXT, PIN_LED_NEXT, &nextButton);
LedButton Menu = LedButton(TYPE_MENU, 0, BUTTON_ENABLED, LED_ON, PIN_BUTTON_MENU, PIN_LED_MENU, &menuButton);
LedButton Save = LedButton(TYPE_SAVE, 0, BUTTON_ENABLED, LED_ON, PIN_BUTTON_SAVE, PIN_LED_SAVE, &saveButton);
LedButton* ledButtons[NUM_BUTTONS] = { &Fav1, &Fav2, &Fav3, &Fav4, &Fav5, &Fav6, &Prev, &Next, &Menu, &Save };

unsigned long previousBlinkMillis = 0;
bool blinkState = HIGH;
uint8_t activePreset = 0;

void setup() {
  Serial.begin(9600);
  delay(1500);  // Safe setup

  digitalWrite(LED_BUILTIN, HIGH);

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setup(DEBOUNCE_DELAY);
  }

  Serial.println("READY");

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

void loop() {
  prepareBlinkState();

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    if (ledButton->pressed()) {
      process(ledButton);
    }
    render(ledButton);
  }


  display.refreshDisplay();  // Must run repeatedly
}

void process(LedButton* ledButton) {
  Serial.print("PROCESS TYPE:");
  Serial.println(ledButton->type);
  switch (ledButton->type) {
    case TYPE_FAV:
      switch (ledButton->buttonState) {
        case BUTTON_WRITABLE:
          Serial.print("STORE PRESET: FAV");
          ledButton->setPreset(activePreset);
          transitionToFav(ledButton);
          break;
        case BUTTON_ENABLED:
          transitionToFav(ledButton);
          Serial.print("FAV");
          Serial.print(ledButton->index);
          Serial.print(" => SWITCH TO PRESET: ");
          Serial.println(ledButton->preset);
          break;
        default:
          Serial.print("BUTTON DISABLED: FAV_");
          Serial.println(ledButton->index);
          break;
      }
      break;
    case TYPE_PREV:
      transitionToPrev();
      break;
    case TYPE_NEXT:
      transitionToNext();
      break;
    case TYPE_MENU:
      transitionToIdle();
      break;
    case TYPE_SAVE:
      transitionToAssigning();
      break;
    default:
      break;
  }
}

void transitionToAssigning() {
  Serial.println("Transition to ASSIGNING");
  display.setChars("ASS");

  for (uint8_t i = 0; i < FAV_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setButtonState(BUTTON_WRITABLE);
    if (ledButton->preset) {
      ledButton->setLedState(LED_ON);
    } else {
      ledButton->setLedState(LED_BLINKING);
    }
  }
  Prev.setButtonState(BUTTON_DISABLED);
  Prev.setLedState(LED_OFF);
  Next.setButtonState(BUTTON_DISABLED);
  Next.setLedState(LED_OFF);
  Menu.setButtonState(BUTTON_ENABLED);
  Menu.setLedState(LED_ON);
  Save.setButtonState(BUTTON_DISABLED);
  Save.setLedState(LED_OFF);
}

void transitionToPrev() {
  if (activePreset == MIN_PRESET) {
    activePreset = MAX_PRESET;
  } else {
    activePreset--;
  }
  Serial.print("Transition to PREV: ");
  Serial.println(activePreset);
  transitionToIdle();
}

void transitionToNext() {
  if (activePreset == MAX_PRESET) {
    activePreset = MIN_PRESET;
  } else {
    activePreset++;
  }
  Serial.print("Transition to NEXT: ");
  Serial.println(activePreset);
  transitionToIdle();
}

void transitionToFav(LedButton* ledButton) {
  activePreset = ledButton->preset;
  transitionToIdle();
}

void transitionToIdle() {
  Serial.println("Transition to IDLE");
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    LedButton* ledButton = ledButtons[i];
    ledButton->setButtonState(BUTTON_ENABLED);
    ledButton->setLedState(LED_OFF);

    if (activePreset) {
      display.setNumber(activePreset - 1);

      if ((ledButton->type == TYPE_FAV) && (ledButton->preset == activePreset)) {
        ledButton->setLedState(LED_ON);
      }
    }
  }
  Prev.setLedState(LED_PRESSED);
  Next.setLedState(LED_PRESSED);
}

void prepareBlinkState() {
  unsigned long currentBlinkMillis = millis();
  if (currentBlinkMillis - previousBlinkMillis >= BLINK_INTERVAL) {
    // save the last time you blinked the LED
    previousBlinkMillis = currentBlinkMillis;

    // if the LED is off turn it on and vice-versa:
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
