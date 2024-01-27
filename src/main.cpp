#include <Arduino.h>
#include <FastLED.h>
#include "HID-Project.h"
#include "PBMC64Wireless.h"

#define TYPING_DELAY_MS 70
#define DOUBLE_CLICK_TIMEOUT_MS 250
#define POST_AUX_ACTION_GRACE_PERIOD 400
#define JOYSTICK_CALIBRATION_MEASUREMENTS 6000
#define JOYSTICK_FIRE_BUTTON_PIN 1
#define JOYSTICK_EXTRA_BUTTON_PIN 0
#define AUX_BUTTON_PIN 2
#define JOYSTICK_X_PIN 4
#define JOYSTICK_Y_PIN 3
#define JOYSTICK_MOVEMENT_THRESHOLD 100

PBMC64Wireless::JoystickInput currentJoystickInput;
KeyboardKeycode joystickExtraKey = KEY_SPACE;
bool joystickExtraKeyNeedsRelease = false;
bool auxButtonPressed = false;
bool auxButtonInDoubleClick = false;
bool auxActionDirty = false;
bool statusReportRequested = false;
bool volumeUpRequested = false;
bool volumeDownRequested = false;
bool resetAuxKeyRequested = false;
bool programmingHappened = false;
int joystickXCenter = 0;
int joystickYCenter = 0;
unsigned long lastAuxButtonReleasedTimestamp = 0;

void setupUSBKeyboard() {
  BootKeyboard.begin();
  BootKeyboard.releaseAll();
}

void clearJoystick() {
  currentJoystickInput.fire = false;
  currentJoystickInput.autofire = false;
  currentJoystickInput.left = false;
  currentJoystickInput.right = false;
  currentJoystickInput.up = false;
  currentJoystickInput.down = false;
}

bool pressed(uint32_t pin) {
  return digitalRead(pin) == LOW;
}

int readJoystickX() {
  return (int) analogRead(JOYSTICK_X_PIN);
}

int readJoystickY() {
  return (int) analogRead(JOYSTICK_Y_PIN);
}

void calibrateJoystick() {
  delay(400);
  int xTotal, yTotal = 0;
  int n = JOYSTICK_CALIBRATION_MEASUREMENTS;
  while (n--) {
    xTotal += readJoystickX();
    yTotal += readJoystickY();
  }
  joystickXCenter = xTotal / JOYSTICK_CALIBRATION_MEASUREMENTS;
  joystickYCenter = yTotal / JOYSTICK_CALIBRATION_MEASUREMENTS;
}

void initInput() {
  pinMode(JOYSTICK_EXTRA_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOYSTICK_FIRE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(AUX_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  clearJoystick();
  calibrateJoystick();
}

void turnOffLeds() {
  CRGB leds[1];
  CFastLED::addLeds<APA102, INTERNAL_DS_DATA, INTERNAL_DS_CLK, BGR>(leds, 1);
  leds[0] = CRGB::Black;
  FastLED.show();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void setup() {
  turnOffLeds();
  setupUSBKeyboard();
  initInput();
}

void type(KeyboardKeycode k) {
  BootKeyboard.press(k);
  delay(TYPING_DELAY_MS);
  BootKeyboard.release(k);
  delay(TYPING_DELAY_MS);
}

void type(uint8_t k) {
  BootKeyboard.press(k);
  delay(TYPING_DELAY_MS);
  BootKeyboard.release(k);
  delay(TYPING_DELAY_MS);
}

void typeRow(char row[]) {
  BootKeyboard.releaseAll();
  int i = 0;
  while (row[i]) {
    type(row[i++]);
  }
  for (int c = 0; c < i; c++) {
    type(KEY_LEFT_ARROW);
  }
  type(KEY_DOWN_ARROW);
}

size_t removeKey(KeyboardKeycode keycode) {
  if (currentJoystickInput.autofire) {
    if (joystickExtraKey != keycode) {
      return BootKeyboard.remove(keycode);
    } else {
      return 0;
    }
  }
  return BootKeyboard.remove(keycode);
}

size_t addKey(KeyboardKeycode keycode) {
  if (auxButtonPressed) {
    joystickExtraKey = keycode;
    BootKeyboard.remove(keycode);
    programmingHappened = true;
  }
  return BootKeyboard.add(keycode);
}

bool joystickActive() {
  return currentJoystickInput.left
         || currentJoystickInput.right
         || currentJoystickInput.up
         || currentJoystickInput.down
         || currentJoystickInput.fire
         || currentJoystickInput.autofire;
}

void updateJoystickState() {
  if (joystickActive()) {
    if (currentJoystickInput.autofire) {
      BootKeyboard.add(joystickExtraKey);
      joystickExtraKeyNeedsRelease = true;
    } else {
      BootKeyboard.remove(joystickExtraKey);
    }
    currentJoystickInput.left ? addKey(KEYPAD_7) : removeKey(KEYPAD_7);
    currentJoystickInput.right ? addKey(KEYPAD_1) : removeKey(KEYPAD_1);
    currentJoystickInput.up ? addKey(KEYPAD_9) : removeKey(KEYPAD_9);
    currentJoystickInput.down ? addKey(KEYPAD_3) : removeKey(KEYPAD_3);
    currentJoystickInput.fire ? addKey(KEYPAD_0) : removeKey(KEYPAD_0);
  } else {
    BootKeyboard.release(KEYPAD_7);
    BootKeyboard.release(KEYPAD_1);
    BootKeyboard.release(KEYPAD_9);
    BootKeyboard.release(KEYPAD_3);
    BootKeyboard.release(KEYPAD_0);
    if (joystickExtraKeyNeedsRelease) {
      BootKeyboard.release(joystickExtraKey);
      joystickExtraKeyNeedsRelease = false;
    }
  }
}

void pollJoystick() {
  clearJoystick();
  int x = (int) analogRead(JOYSTICK_X_PIN);
  int y = (int) analogRead(JOYSTICK_Y_PIN);
  currentJoystickInput.autofire = pressed(JOYSTICK_EXTRA_BUTTON_PIN);
  currentJoystickInput.fire = pressed(JOYSTICK_FIRE_BUTTON_PIN);
  if (x < joystickXCenter - JOYSTICK_MOVEMENT_THRESHOLD) {
    currentJoystickInput.left = true;
  } else if (x > joystickXCenter + JOYSTICK_MOVEMENT_THRESHOLD) {
    currentJoystickInput.right = true;
  }
  if (y > joystickYCenter + JOYSTICK_MOVEMENT_THRESHOLD) {
    currentJoystickInput.down = true;
  } else if (y < joystickYCenter - JOYSTICK_MOVEMENT_THRESHOLD) {
    currentJoystickInput.up = true;
  }
}

void sendStatusReport() {
  char row[80];
  snprintf(row, 80, "up %ld", millis());
  typeRow(row);
  snprintf(row, 80, "x,y center %d, %d", joystickXCenter, joystickYCenter);
  typeRow(row);
  type(KEY_ENTER);
}

void resetAuxButtonFlags() {
  auxButtonInDoubleClick = false;
  programmingHappened = false;
  auxActionDirty = false;
  statusReportRequested = false;
  volumeUpRequested = false;
  volumeDownRequested = false;
  resetAuxKeyRequested = false;
}

bool hasAuxSystemRequest() {
  return statusReportRequested || volumeUpRequested || volumeDownRequested || resetAuxKeyRequested;
}

bool doubleClickGracePeriodHasPassed() {
  return millis() > lastAuxButtonReleasedTimestamp + DOUBLE_CLICK_TIMEOUT_MS;
}

void pollAuxButton() {
  bool previousState = auxButtonPressed;
  auxButtonPressed = pressed(AUX_BUTTON_PIN);
  if (!statusReportRequested && auxButtonPressed && !previousState) {
    statusReportRequested = currentJoystickInput.left;
    volumeUpRequested = currentJoystickInput.up;
    volumeDownRequested = currentJoystickInput.down;
    resetAuxKeyRequested = currentJoystickInput.right;
  }
  if (!auxButtonPressed && previousState) {
    lastAuxButtonReleasedTimestamp = millis();
    auxActionDirty = true;
  } else if (auxButtonPressed && !doubleClickGracePeriodHasPassed()) {
    auxButtonInDoubleClick = true;
  }
  if (auxActionDirty && hasAuxSystemRequest()) {
    if (statusReportRequested) {
      sendStatusReport();
    } else if (volumeUpRequested) {
      type(KEY_F11);
    } else if (volumeDownRequested) {
      type(KEY_F10);
    } else if (resetAuxKeyRequested) {
      joystickExtraKey = KEY_SPACE;
    }
    resetAuxButtonFlags();
  } else if (auxActionDirty && doubleClickGracePeriodHasPassed()) {
    if (!programmingHappened) {
      if (auxButtonInDoubleClick) {
        type(KEYPAD_ADD);
      } else {
        type(KEY_F12);
      }
      BootKeyboard.releaseAll();
      clearJoystick();
      delay(POST_AUX_ACTION_GRACE_PERIOD);
    }
    resetAuxButtonFlags();
  }
}

void loop() {
  pollAuxButton();
  pollJoystick();
  updateJoystickState();
  if (joystickActive()) {
    BootKeyboard.send();
  }
}
