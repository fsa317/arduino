/*
  Button

 Turns on and off a light emitting diode(LED) connected to digital
 pin 13, when pressing a pushbutton attached to pin 2.


 The circuit:
 * LED attached from pin 13 to ground
 * pushbutton attached to pin 2 from +5V
 * 10K resistor attached to pin 2 from ground

 * Note: on most Arduinos there is already an LED on the board
 attached to pin 13.


 created 2005
 by DojoDave <http://www.0j0.org>
 modified 30 Aug 2011
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/Button
 */

// constants won't change. They're used here to
// set pin numbers:
const int gateOffPin = D1;     // the number of the pushbutton pin
const int gateOnPin =  D2;      // the number of the LED pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int prevState = -1;

void setup() {
  // initialize the pushbutton pin as an input:
  //pinMode(buttonPin, INPUT_PULLUP);
  pinMode(gateOnPin,OUTPUT);
  pinMode(gateOffPin,OUTPUT);
  
  Serial.begin(115200);

  
}

void loop() {
   digitalWrite(gateOffPin, HIGH);
   delay(2000);
   digitalWrite(gateOffPin, LOW);
   delay(2000);
}

