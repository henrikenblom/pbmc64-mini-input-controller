#include <Arduino.h>
#include <FastLED.h>
#include "HID-Project.h"
#include "PBMC64Wireless.h"

#define NUM_LEDS 1
#define TYPING_DELAY 70
#define LOCAL_JOYSTICK_FIRE_PIN 0
#define LOCAL_JOYSTICK_PROGRAMMABLE_BUTTON_PIN 1
#define AUX_BUTTON_PIN 11
#define LOCAL_JOYSTICK_X_PIN PIN_A3
#define LOCAL_JOYSTICK_Y_PIN PIN_A4
#define LOCAL_JOYSTICK_RIGHT_TRIGGER  275
#define LOCAL_JOYSTICK_LEFT_TRIGGER 470
#define LOCAL_JOYSTICK_UP_TRIGGER 450
#define LOCAL_JOYSTICK_DOWN_TRIGGER 230

CRGB leds[NUM_LEDS];
PBMC64Wireless::JoystickInput currentLocalJoystickInput;
KeyboardKeycode localJoystickAuxKey = KEY_SPACE;
bool localJoystickAuxKeyNeedsRelease = false;
bool auxButtonPressed = false;
bool programmingHappened = false;

void setupUSBKeyboard() {
  BootKeyboard.begin();
  BootKeyboard.releaseAll();
}

void clearLocalJoystick() {
  currentLocalJoystickInput.fire = false;
  currentLocalJoystickInput.autofire = false;
  currentLocalJoystickInput.left = false;
  currentLocalJoystickInput.right = false;
  currentLocalJoystickInput.up = false;
  currentLocalJoystickInput.down = false;
}

void initLocalInput() {
  pinMode(LOCAL_JOYSTICK_PROGRAMMABLE_BUTTON_PIN, INPUT);
  pinMode(LOCAL_JOYSTICK_FIRE_PIN, INPUT);
  pinMode(LOCAL_JOYSTICK_X_PIN, INPUT);
  pinMode(LOCAL_JOYSTICK_Y_PIN, INPUT);
  pinMode(AUX_BUTTON_PIN, INPUT);
  clearLocalJoystick();
}

void setup() {
  CFastLED::addLeds<APA102, INTERNAL_DS_DATA, INTERNAL_DS_CLK, BGR>(leds, NUM_LEDS);
  leds[0] = CRGB::Black;
  FastLED.show();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  setupUSBKeyboard();
}

void type(KeyboardKeycode k) {
  BootKeyboard.press(k);
  delay(TYPING_DELAY);
  BootKeyboard.release(k);
  delay(TYPING_DELAY);
}

size_t removeKey(KeyboardKeycode keycode) {
  if (currentLocalJoystickInput.autofire) {
    if (localJoystickAuxKey != keycode) {
      return BootKeyboard.remove(keycode);
    } else {
      return 0;
    }
  }
  return BootKeyboard.remove(keycode);
}

size_t addKey(KeyboardKeycode keycode) {
  if (auxButtonPressed) {
    localJoystickAuxKey = keycode;
    BootKeyboard.remove(keycode);
    programmingHappened = true;
  }
  return BootKeyboard.add(keycode);
}

bool localJoystickActive() {
  return currentLocalJoystickInput.left
         || currentLocalJoystickInput.right
         || currentLocalJoystickInput.up
         || currentLocalJoystickInput.down
         || currentLocalJoystickInput.fire
         || currentLocalJoystickInput.autofire;
}

void updateLocalJoystickState() {
  if (localJoystickActive()) {
    if (currentLocalJoystickInput.autofire) {
      BootKeyboard.add(localJoystickAuxKey);
      localJoystickAuxKeyNeedsRelease = true;
    } else {
      BootKeyboard.remove(localJoystickAuxKey);
    }
    currentLocalJoystickInput.left ? addKey(KEYPAD_7) : removeKey(KEYPAD_7);
    currentLocalJoystickInput.right ? addKey(KEYPAD_1) : removeKey(KEYPAD_1);
    currentLocalJoystickInput.up ? addKey(KEYPAD_9) : removeKey(KEYPAD_9);
    currentLocalJoystickInput.down ? addKey(KEYPAD_3) : removeKey(KEYPAD_3);
    currentLocalJoystickInput.fire ? addKey(KEYPAD_0) : removeKey(KEYPAD_0);
  } else {
    BootKeyboard.release(KEYPAD_7);
    BootKeyboard.release(KEYPAD_1);
    BootKeyboard.release(KEYPAD_9);
    BootKeyboard.release(KEYPAD_3);
    BootKeyboard.release(KEYPAD_0);
    if (localJoystickAuxKeyNeedsRelease) {
      BootKeyboard.release(localJoystickAuxKey);
      localJoystickAuxKeyNeedsRelease = false;
    }
  }
}

void pollLocalJoystick() {
  clearLocalJoystick();
  int x = (int) analogRead(LOCAL_JOYSTICK_X_PIN);
  int y = (int) analogRead(LOCAL_JOYSTICK_Y_PIN);
  currentLocalJoystickInput.autofire = digitalRead(LOCAL_JOYSTICK_PROGRAMMABLE_BUTTON_PIN);
  currentLocalJoystickInput.fire = digitalRead(LOCAL_JOYSTICK_FIRE_PIN);
  if (x < LOCAL_JOYSTICK_RIGHT_TRIGGER) {
    currentLocalJoystickInput.right = true;
  } else if (x > LOCAL_JOYSTICK_LEFT_TRIGGER) {
    currentLocalJoystickInput.left = true;
  }
  if (y > LOCAL_JOYSTICK_UP_TRIGGER) {
    currentLocalJoystickInput.up = true;
  } else if (y < LOCAL_JOYSTICK_DOWN_TRIGGER) {
    currentLocalJoystickInput.down = true;
  }
}

void loop() {
  //TODO: Add hardware and uncomment:
  /*pollLocalJoystick();
  updateLocalJoystickState();
  if (localJoystickActive()) {
    BootKeyboard.send();
  }*/
}
