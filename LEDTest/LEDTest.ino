#include "FastLED.h"

#define NUM_LEDS 1
#define DATA_PIN 2


CRGB leds[NUM_LEDS];


void setup() {
  // put your setup code here, to run once:
   FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
}

void loop() {
  // put your main code here, to run repeatedly:

}
