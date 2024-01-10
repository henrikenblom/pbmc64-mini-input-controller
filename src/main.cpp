#include <Arduino.h>
#include <FastLED.h>
#include "HID-Project.h"
#include "PBMC64Wireless.h"

#define NUM_LEDS 1
#define TYPING_DELAY 70

CRGB leds[NUM_LEDS];

void setupUSBKeyboard() {
  BootKeyboard.begin();
  BootKeyboard.releaseAll();
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

void loop() {
}
