#include <FastLED.h>

#include <LEDMatrix.h>

// Change the next 6 defines to match your matrix type and size

#define LED_PIN        2
#define COLOR_ORDER    GRB
#define CHIPSET        WS2812B

#define MATRIX_WIDTH   60 // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_HEIGHT  4  // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX  // See top of LEDMatrix.h for matrix wiring types

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

uint8_t hue;
int16_t counter;

void setup()
{
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  FastLED.setBrightness(60);
  FastLED.clear(true);
  delay(500);
  leds(1) = CRGB::Red;
  leds(30) = CRGB::Lime;
  leds(61) = CRGB::Blue;
  leds(1,5) = CRGB::White;
  FastLED.show();
  /*FastLED.showColor(CRGB::Red);
  delay(1000);
  FastLED.showColor(CRGB::Lime);
  delay(1000);
  FastLED.showColor(CRGB::Blue);
  delay(1000);
  FastLED.showColor(CRGB::White);
  delay(1000);
  FastLED.clear(true);

  hue = 0;
  counter = 0;*/
}


void loop()
{
  int16_t sx, sy, x, y;
  uint8_t h;

  FastLED.clear();

}

