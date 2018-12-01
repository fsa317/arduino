#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"

//PINS
#define MULTIPLEX_PIN D1
#define ANALOGPIN A0
#define REDPIN D6
#define GREENPIN D7
#define BLUEPIN D8
#define HEATER D5
#define DHTPIN D2     // what digital pin we're connected to


// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11

//TEMP CONSTANTS
#define SERIESRESISTOR 10000 
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950

//APP SETTINGS
#define DEFAULT_DESIREDTEMP 57
#define DEFAULT_FINMINTEMP 34
#define DEFAULT_DEADBAND 3
#define DEFAULT_DEFROSTDEADBAND 2
#define DEFAULT_HEATERPOWER 650

int desiredTemp;
int finMinTemp;
int deadband;
int defrostDeadband;
int heaterPower;


int cooling = 0;    //cooling means the resistor on
int defrosting = 0;
float roomTemp;
float finTemp;
float humidity;

WiFiClient espClient;
PubSubClient mqttclient(espClient);
DHT dht(DHTPIN, DHTTYPE);

int settingsChanged = 0;

void setup() {
  // put your setup code here, to run once:

  pinMode(MULTIPLEX_PIN,OUTPUT);
  digitalWrite(MULTIPLEX_PIN,LOW);

  lightsOff();
  turnOnRed();
  desiredTemp = DEFAULT_DESIREDTEMP;
  finMinTemp = DEFAULT_FINMINTEMP;
  deadband = DEFAULT_DEADBAND;
  heaterPower = DEFAULT_HEATERPOWER;
  defrostDeadband = DEFAULT_DEFROSTDEADBAND;
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  dbg("Wine Cooler started");

  //TODO test error handling 
  dht.begin();
  connectToWifi();
  connectToMQTT();
  turnOnAC(0);
  printSettings();
}

void loop() {

  //TODO deal with disconnections
  if (!mqttclient.connected()){
    dbg("Attempting to reconnect to MQTT");
    connectToMQTT();
  }
  roomTemp = getRoomTemp();
  finTemp = getFinTemp();
  int cutoff = desiredTemp + deadband;
  
  if (roomTemp >= cutoff  && !defrosting  && !cooling){
     //room is past the cutoff and were not in a defrost so turn on the heater
     dbg("!!! Turning on the AC");
     sendMQInfo("!!! Turning on AC");
     cooling = 1;
     turnOnAC(heaterPower);
     turnOnRed();
  }

  if ((roomTemp <= desiredTemp) && cooling){
    //if we are cooling but we reached the desired temp
     dbg("Turning off the AC since we reached the temp");
     sendMQInfo("!!! Turning off AC since we reached the temp");
     lightsOff();
     turnOnAC(0);
     cooling = 0;
  }

  if ((finTemp < finMinTemp) && !defrosting){
    if (cooling){
      dbg("Turning off heating due to finTemp");
      sendMQInfo("Turning off heating due to finTemp");
      cooling = 0;
      turnOnAC(0);
      lightsOff();
    }
    defrosting = 1;   // turn on defrost
  }

  if (defrosting){
    if (finTemp > (finMinTemp + defrostDeadband)){
      dbg("Defrost Complete");
      sendMQInfo("Defrost Complete");
      defrosting = 0;
    } else {
      //still defrosting
      flashBlue();
    }
  }

  delay(1000);
  humidity = getHumidity();

  printStatus();
  //dbg("DHTTemp= "+String(getDHTTemp()));

  for (int i=0; i < 30; i++){
    mqttclient.loop();
  
    if (settingsChanged == 1){
      printSettings();
      settingsChanged = 0;
      break;
    }
    
    delay(1000);    //probably change this later
  }

}
/* End Main Loop */

void msgrcvd(char* topic, byte* payload, unsigned int length) {
  /* DONT MAKE NETWORK CALLS HERE */
  String topicStr(topic);
  payload[length] = '\0';
  String msg ((char *)payload);
  /* debug */
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  //Serial.println("_"+msg+"_");
  //Serial.println();
  int tmp;
  if (topicStr == "winebot/settemp"){
    tmp = msg.toInt();
    if (tmp!=desiredTemp){
      desiredTemp = msg.toInt();
      dbg("## Desired Temp set to "+String(desiredTemp));
      settingsChanged = 1;
    }
  } else if (topicStr == "winebot/setfinmintemp"){
    tmp = msg.toInt();
    if (tmp!=finMinTemp){
      finMinTemp = msg.toInt();
      dbg("## FinMin Temp set to "+String(finMinTemp));
      settingsChanged = 1;
    }
  } else if (topicStr == "winebot/setheaterpower"){
    tmp = msg.toInt();
    if (tmp!=heaterPower){
      heaterPower = msg.toInt();
      dbg("## heaterPower set to "+String(heaterPower));
      settingsChanged = 1;
    }
  }
  
}

float getFinTemp(){
  digitalWrite(MULTIPLEX_PIN,LOW);
  float avg = getAnalogSamples();
  float temp = calcTemp(avg);
  //dbg("Fin Temp:");
  //dbg(String(temp));
  return temp;
}

float getRoomTemp(){
  digitalWrite(MULTIPLEX_PIN,HIGH);
  float avg = getAnalogSamples();
  float temp = calcTemp(avg);
  //dbg("Room Temp:");
  //dbg(String(temp));
  return temp;
}

void turnOnAC(int power){
   analogWrite(HEATER,power);
  
}

// Helper Functions

float getAnalogSamples(){
  int i;
  int samples[NUMSAMPLES];
  float average;
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(ANALOGPIN);
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

float getHumidity(){
  int i;
  int samples[NUMSAMPLES];
  float average;
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] =  dht.readHumidity();
   delay(500);
  }
 
  // average all the samples out
  average = 0;
  int cnt = 0;
  for (i=0; i< NUMSAMPLES; i++) {
    if (samples[i] < 100){
       average += samples[i];
       cnt++;
    }
  }
  if (cnt==0){
    return 0;
  }
  average /= cnt;
  return average;
}

float getDHTTemp(){
  int i;
  int samples[NUMSAMPLES];
  float average;
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] =  dht.readTemperature(true);
   delay(500);
  }
 
  // average all the samples out
  average = 0;
  int cnt = 0;
  for (i=0; i< NUMSAMPLES; i++) {
    if (samples[i] < 100){
     average += samples[i];
     cnt++;
    }
  }
  if (cnt==0){
    return 0;
  }
  average /= cnt;
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

void flashGreen(){
  analogWrite(GREENPIN,1024);
  delay(500);
  analogWrite(GREENPIN,0);
}

void flashRed(){
  analogWrite(REDPIN,1024);
  delay(500);
  analogWrite(REDPIN,0);
}

void flashBlue(){
  analogWrite(BLUEPIN,1024);
  delay(500);
  analogWrite(BLUEPIN,0);
}

void turnOnRed(){
  analogWrite(BLUEPIN,0);
  analogWrite(GREENPIN,0);
  analogWrite(REDPIN,1024);
}

void connectToWifi(){
  const char* ssid = "TruffleShuffle";
  const char* password = "Mets1234";
  WiFi.begin(ssid, password);
  dbg("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    flashRed();
    delay(50);
    dbg(".");
  }
  dbg("Connected");
  flashGreen();
}

void connectToMQTT(){
  mqttclient.setServer("pizero1", 1883);
 
  String clientId = "WINECOOLER-";
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      dbg("mqtt connected");
      sendMQInfo("Wine Cooler Connected");
      regServer();
      mqttclient.setCallback(msgrcvd);
      mqttclient.subscribe("winebot/#");
  } else {
      dbg("mqtt failed, rc=");
      flashRed();
      flashRed();
      flashRed();
      dbg(String(mqttclient.state()));   
  }
  
}

void lightsOff(){
  analogWrite(BLUEPIN,0);
  analogWrite(REDPIN,0);
  analogWrite(GREENPIN,0);
}

void printStatus(){
  dbg("***************");
  dbg("Room Temp= "+String(roomTemp));
  dbg("Fin Temp= "+String(finTemp));
  dbg("Cooling= "+String(cooling));
  dbg("Defrosting= "+String(defrosting));
  dbg("Humidity= "+String(humidity));
  dbg("***************");

  
  sendMQData("Room Temp="+String(roomTemp));
  sendMQData("Fin Temp="+String(finTemp));
  sendMQData("Cooling="+String(cooling));
  sendMQData("Defrosting="+String(defrosting));
  sendMQData("Humidity="+String(humidity));
  
}

void printSettings(){
  dbg("PRINTING SETTINGS");
  dbg("DesiredTemp= "+String(desiredTemp));
  dbg("finMinTemp= "+String(finMinTemp));
  dbg("deadband= "+String(deadband));
  dbg("heaterPower= "+String(heaterPower));
  sendMQData("DesiredTemp="+String(desiredTemp));
  sendMQData("finMinTemp="+String(finMinTemp));
  sendMQData("deadband="+String(deadband));
  sendMQData("heaterPower="+String(heaterPower));
}

void regServer(){
  String s = "Wine:"+WiFi.localIP().toString();
  char temp[100];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("regserver",temp);
  mqttclient.loop();
}

void sendMQInfo(String s){
  char temp[1000];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("wine/debug",temp);
  mqttclient.loop();
}

void sendMQData(String s){
  char temp[1000];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("wine/data",temp);
  mqttclient.loop();
}

void dbg(String s){
  Serial.println(s);
  Serial.flush();
}

