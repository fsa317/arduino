/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


WiFiClient wificlient;
int d2State;
int retries = 0;
int LED = D7; //LED_BUILTIN;

int MAX_RETRIES = 25;

void setup() {


    Serial.begin(115200);
    Serial.setDebugOutput(true);
    pinMode(D2, INPUT);
    
    
    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(192,168,1,44),IPAddress(192,168,1,1),IPAddress(255,255,255,0),IPAddress(192,168,1,1));
    WiFi.begin("TruffleShuffle", "Mets1234");
   
    Serial.println();
    Serial.println();
    Serial.println();
    
    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(10);
    }

    

}

void blinkLED(){
  pinMode(LED, OUTPUT);
  for (int i = 0; i < 3; i++){
    digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level                               // but actually the LED is on; this is becaus                                 // it is acive low on the ESP-01)
    delay(500);                      // Wait for a second
    digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(500);  
  }    
 
}

void blinkFail(){
  pinMode(LED, OUTPUT);
  for (int i = 0; i < 10; i++){
    digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level                               // but actually the LED is on; this is becaus                                 // it is acive low on the ESP-01)
    delay(200);                      // Wait for a second
    digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(200);  
  }   
}


void loop() {
    // wait for WiFi connection
    
    if((WiFi.status() == WL_CONNECTED)) {

        HTTPClient http;

        d2State = digitalRead(D5);
        Serial.println();
        Serial.println("D2 state:");
        Serial.println(d2State);

        Serial.print("[HTTP] begin...\n");
        // configure traged server and url

        //http.begin("http://pizero1/frontdoor/unlock"); //HTTP
         if (d2State == 0){
           Serial.println("http://pizero1/fireplace/off");
           http.begin("http://pizero1/fireplace/off");
         } else {
           Serial.println("http://pizero1/fireplace/on");
           http.begin("http://pizero1/fireplace/on");  
         }
         //HTTP

         
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                blinkLED();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            blinkFail();
        }

        http.end();
        
        Serial.println("Sleeping");
        Serial.flush();
        ESP.deepSleep(0);
    } else {
      retries++;
      Serial.print(".");
      Serial.flush();
      delay(100);
      if (retries > MAX_RETRIES){
        blinkFail();
        Serial.println("Max time spent trying to connect");
        Serial.flush();
        ESP.deepSleep(0);
      }
    }
    Serial.flush();
}

