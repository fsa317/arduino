#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 30

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 13


// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 

       FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

}

void loop() { 
  // Turn the LED on, then pause
  FastLED.clear();
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[1] = CRGB::Lime;
  FastLED.show();
  delay(500);
  
  leds[2] = CRGB::Blue;
  FastLED.show();
  delay(500);
/*
  leds[10] = CRGB::Red;
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[11] = CRGB::Lime;
  FastLED.show();
  delay(500);
  
  leds[12] = CRGB::Blue;
  FastLED.show();
  delay(500); */


  
  delay(60000);
  
}
