#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

//https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

const char* ssid = "TruffleShuffle";
const char* password = "Mets1234";
const char* mqtt_server = "pizero1";

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqttclient(espClient);

const int gateOffPin = D1;
const int gateOnPin =  D2;      // the number of the LED pin




void handleRoot() {

  server.send(200, "text/plain", "OK");

}


void handleNotFound(){
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
 
}

void flashLED(int d){
  digitalWrite(LED_BUILTIN, LOW);
  delay(d);
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup(void){
  flashLED(100);
  Serial.begin(115200);
  pinMode(gateOnPin,OUTPUT);
  pinMode(gateOffPin,OUTPUT);
  digitalWrite(gateOffPin, LOW);
  digitalWrite(gateOnPin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  flashLED(100);
  WiFi.begin(ssid, password);
  Serial.println("");
  flashLED(1000);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    flashLED(250);
    delay(250);
    Serial.print(".");
  }
  flashLED(1500);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  mqttclient.setServer(mqtt_server, 1883);
 
  String clientId = "FIREPLACE-";
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      Serial.println("mqtt connected");
      // Once connected, publish an announcement...
      String s = "Fireplace:"+WiFi.localIP().toString();
      Serial.print("Sending "+s);
      char temp[100];
      s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...

      mqttclient.publish("regserver",temp);
      mqttclient.loop();
    } 
  else {
      Serial.print("mqtt failed, rc=");
      Serial.print(mqttclient.state());   
  }
    
  
  
  server.on("/", handleRoot);

  server.on("/fireplaceon", [](){
    turnOnFireplace();
    server.send(200, "text/plain", "OK-ON");
  });

  server.on("/fireplaceoff", [](){
    turnOffFireplace();
    server.send(200, "text/plain", "OK-OFF");
  });

  server.onNotFound(handleNotFound);

  //turnOnFireplace();

  server.begin();
  Serial.println("HTTP server started");
}

void turnOnFireplace(){
  Serial.println("Turning On Fireplace");
  digitalWrite(gateOnPin, HIGH);  //GATEPIN High makes fireplace go on
  Serial.flush();
  delay(300);
  digitalWrite(gateOnPin, LOW);
}

void turnOffFireplace(){
  Serial.println("Turning off Fireplace");
  digitalWrite(gateOffPin, HIGH);  //GATEPIN High makes fireplace go off
  Serial.flush();
  delay(300);
  digitalWrite(gateOffPin, LOW);
}


void loop(void){
  server.handleClient();
}
