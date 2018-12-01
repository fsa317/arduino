#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
  
 
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
#define BCOEFFICIENT 3950

WiFiClient espClient;
PubSubClient mqttclient(espClient);
 
void setup(void) {
  Serial.begin(9600);
  connectToWifi();
  connectToMQTT();
}
 
void loop(void) {


  float avg = getAnalogSamples();
  float temp = calcTemp(avg);
 
  Serial.print("Temp"); 
  Serial.println(temp);
  sendMQInfo(String(temp));

 
  delay(2000);
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

void dbg(String s){
  Serial.println(s);
  Serial.flush();
}

void connectToMQTT(){
  mqttclient.setServer("pizero1", 1883);
 
  String clientId = "TEMPPROBE-";
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      dbg("mqtt connected");
  } else {
      dbg("mqtt failed, rc=");
      dbg(String(mqttclient.state()));   
  }
}

void sendMQInfo(String s){
  char temp[1000];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("tempprobe/update",temp);
  mqttclient.loop();
  dbg("MQ msg sent");
}
