#include <Arduino.h>
#include <FastLED.h>
#include "HID-Project.h"
#include "PBMC64Wireless.h"

#define TYPING_DELAY_MS 80
#define POST_EMU_ACTION_GRACE_PERIOD 250
#define JOYSTICK_CALIBRATION_MEASUREMENTS 1000
#define JOYSTICK_SAMPLES 60
#define JOYSTICK_FIRE_BUTTON_PIN 1
#define JOYSTICK_EXTRA_BUTTON_PIN 0
#define AUX_BUTTON_PIN 2
#define JOYSTICK_X_PIN 4
#define JOYSTICK_Y_PIN 3
#define JOYSTICK_MOVEMENT_ENGAGE_THRESHOLD 120
#define JOYSTICK_MOVEMENT_RELEASE_THRESHOLD 100

enum EmulatorAction {
    MENU,
    ON_SCREEN_KEYBOARD
};

PBMC64Wireless::JoystickInput currentJoystickInput;
bool joystickExtraKeyNeedsRelease = false;
bool auxButtonPressed = false;
bool toggleOnScreenKeyboardRequested = false;
bool volumeUpRequested = false;
bool volumeDownRequested = false;
int joystickXCenter = 0;
int joystickYCenter = 0;

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

int readJoystick(uint32_t pin) {
  int t = 0;
  int n = JOYSTICK_SAMPLES;
  while (n--) {
    t += (int) analogRead(pin);
  }
  return t / JOYSTICK_SAMPLES;
}

int readJoystickX() {
  return readJoystick(JOYSTICK_X_PIN);
}

int readJoystickY() {
  return readJoystick(JOYSTICK_Y_PIN);
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

void flashLed() {
  digitalWrite(13, HIGH);
  delay(50);
  digitalWrite(13, LOW);
}

void setup() {
  turnOffLeds();
  setupUSBKeyboard();
  initInput();
  flashLed();
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

size_t removeKey(KeyboardKeycode keycode) {
  if (currentJoystickInput.autofire) {
    if (KEYPAD_SUBTRACT != keycode) {
      return BootKeyboard.remove(keycode);
    } else {
      return 0;
    }
  }
  return BootKeyboard.remove(keycode);
}

size_t addKey(KeyboardKeycode keycode) {
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
      BootKeyboard.add(KEYPAD_SUBTRACT);
      joystickExtraKeyNeedsRelease = true;
    } else {
      BootKeyboard.remove(KEYPAD_SUBTRACT);
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
      BootKeyboard.release(KEYPAD_SUBTRACT);
      joystickExtraKeyNeedsRelease = false;
    }
  }
}

bool joystickLeft(int x) {
  if (currentJoystickInput.left) {
    return x < joystickXCenter - JOYSTICK_MOVEMENT_RELEASE_THRESHOLD;
  }
  return x < joystickXCenter - JOYSTICK_MOVEMENT_ENGAGE_THRESHOLD;
}

bool joystickRight(int x) {
  if (currentJoystickInput.right) {
    return x > joystickXCenter + JOYSTICK_MOVEMENT_RELEASE_THRESHOLD;
  }
  return x > joystickXCenter + JOYSTICK_MOVEMENT_ENGAGE_THRESHOLD;
}

bool joystickDown(int y) {
  if (currentJoystickInput.down) {
    return y > joystickYCenter + JOYSTICK_MOVEMENT_RELEASE_THRESHOLD;
  }
  return y > joystickYCenter + JOYSTICK_MOVEMENT_ENGAGE_THRESHOLD;
}

bool joystickUp(int y) {
  if (currentJoystickInput.up) {
    return y < joystickYCenter - JOYSTICK_MOVEMENT_RELEASE_THRESHOLD;
  }
  return y < joystickYCenter - JOYSTICK_MOVEMENT_ENGAGE_THRESHOLD;
}

void pollJoystick() {
  int x = readJoystickX();
  int y = readJoystickY();
  currentJoystickInput.autofire = pressed(JOYSTICK_EXTRA_BUTTON_PIN);
  currentJoystickInput.fire = pressed(JOYSTICK_FIRE_BUTTON_PIN);
  currentJoystickInput.left = joystickLeft(x);
  currentJoystickInput.right = joystickRight(x);
  currentJoystickInput.down = joystickDown(y);
  currentJoystickInput.up = joystickUp(y);
}

void resetAuxButtonFlags() {
  toggleOnScreenKeyboardRequested = false;
  volumeUpRequested = false;
  volumeDownRequested = false;
}

bool hasAuxSystemRequest() {
  return toggleOnScreenKeyboardRequested || volumeUpRequested || volumeDownRequested;
}

void triggerEmulatorAction(EmulatorAction action) {
  switch (action) {
    case MENU:
      type(KEY_F12);
      break;
    default:
      type(KEYPAD_ADD);
  }
  BootKeyboard.releaseAll();
  clearJoystick();
  delay(POST_EMU_ACTION_GRACE_PERIOD);
}

void pollAuxButton() {
  bool previousState = auxButtonPressed;
  auxButtonPressed = pressed(AUX_BUTTON_PIN);
  if (auxButtonPressed && !previousState) {
    toggleOnScreenKeyboardRequested = currentJoystickInput.left;
    volumeUpRequested = currentJoystickInput.up;
    volumeDownRequested = currentJoystickInput.down;
  }
  if (!auxButtonPressed && previousState) {
    if (hasAuxSystemRequest()) {
      if (toggleOnScreenKeyboardRequested) {
        triggerEmulatorAction(ON_SCREEN_KEYBOARD);
      } else if (volumeUpRequested) {
        type(KEY_F11);
      } else if (volumeDownRequested) {
        type(KEY_F10);
      }
    } else {
      triggerEmulatorAction(MENU);
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
