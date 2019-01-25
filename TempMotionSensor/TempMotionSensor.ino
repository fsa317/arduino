#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>


#define DEVICEID "TEMPMOTION1"
#define DELAY 50
#define TEMPDELAY 30000
 
// What pin to connect the sensor to
#define THERMISTORPIN A0 

#define SERIESRESISTOR 10000 
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3050


WiFiClient espClient;
PubSubClient mqttclient(espClient);

int ledPin = LED_BUILTIN;                // choose the pin for the LED
int pirInputPin = D5;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status
int delayCounter = 0; 

 
void setup(void) {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(pirInputPin, INPUT);     // declare sensor as input
  Serial.print("LED: ");
  Serial.println(LED_BUILTIN);
  connectToWifi();
  connectToMQTT();
  doTempReading();
  Serial.println("Waiting for motion sensor");
  delay(30*1000);
  Serial.println("Begin monitoring");
}

void readPIR(){
  val = digitalRead(pirInputPin);  // read input value
  //Serial.print("val: ");
  //Serial.println(val);
 // Serial.println("INPUTPIN:"+pirInputPin);
  if (val == HIGH) {            // check if the input is HIGH
    digitalWrite(ledPin, LOW);  // turn LED ON
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      //Serial.println(delayCounter);
      // We only want to print on the output change, not state
      pirState = HIGH;
      sendMotionMQInfo("ON");
    }
  } else {
    digitalWrite(ledPin, HIGH); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      //Serial.println(delayCounter);
      // We only want to print on the output change, not state
      pirState = LOW;
      sendMotionMQInfo("OFF");
    }
  }
}
 
void loop(void) {
  if (!mqttclient.connected()){
    dbg("Attempting to reconnect to MQTT");
    connectToMQTT();
  }
  readPIR();
  delay(DELAY);

  delayCounter += DELAY;
 
  if (delayCounter >= TEMPDELAY){
 
    doTempReading();
    delayCounter = 0;
  }
 

  
}

void doTempReading(){
    float avg = getAnalogSamples();
    float temp = calcTemp(avg);
   
    Serial.print("Temp"); 
    Serial.println(temp);
    sendTempMQInfo(String(temp));
}

float getAnalogSamples(){
  int i;
  int samples[NUMSAMPLES];
  float average;
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(A0);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
  return average;
}

float calcTemp(float RawADC){
  float res;
  res = SERIESRESISTOR*(1023/RawADC-1);
  float steinhart;
  steinhart = res / THERMISTORNOMINAL; // (R/Ro)
  steinhart = log(steinhart); // ln(R/Ro)
  steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15; // convert to C

  float f = steinhart*1.8+32;
  return f;
}

void dbg(String s){
  Serial.println(s);
  Serial.flush();
}


void connectToWifi(){
  const char* ssid = "TruffleShuffle";
  const char* password = "Mets1234";
  WiFi.begin(ssid, password);
  dbg("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    dbg(".");
  }
  dbg("Connected");
}



void connectToMQTT(){
  mqttclient.setServer("pizero1", 1883);
 
  String clientId = DEVICEID;
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      dbg("mqtt connected");
  } else {
      dbg("mqtt failed, rc=");
      dbg(String(mqttclient.state()));   
  }
}


void sendMotionMQInfo(String s){
  char motion[100];
  s.toCharArray(motion, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  char topic[100];
  strcpy(topic,"sensors/motion/");
  strcat(topic,DEVICEID);
  mqttclient.publish(topic,motion);
  mqttclient.loop();
  dbg("sendMotionMQInfo msg sent");
}

void sendTempMQInfo(String s){
  char temp[100];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  char topic[100];
  strcpy(topic,"sensors/temp/");
  strcat(topic,DEVICEID);
  mqttclient.publish(topic,temp);
  mqttclient.loop();
  dbg("sendTempMQInfo msg sent");
}

