#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontRobotron.h>
#include <FontMatrise.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

//https://github.com/FastLED/FastLED/wiki
//https://github.com/AaronLiddiment/LEDText/wiki
//Set BOARD to NodeMCU1.0

const char *ssid = "TruffleShuffle";
const char *password = "Mets1234";

WiFiClient espClient;
PubSubClient mqttclient(espClient);

// Change the next 6 defines to match your matrix type and size

#define LED_PIN        7  //GPIO13
#define COLOR_ORDER    GRB
#define CHIPSET        WS2812B

#define MATRIX_WIDTH   60
#define MATRIX_HEIGHT  7
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX

#define MAXMSG 5

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

cLEDText ScrollingMsg;

char TxtDemo[] = { "          Hello!         "};

String msgList[MAXMSG];
char * msgStr;

int currentMsgIdx=0;
int totalMsg=0;
int nextSlot = 0;

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  dbg("MyTextDemo starting up...");
  dbg("Attempting wifi connection");
  delay(100);
  WiFi.begin ( ssid, password );
  delay(500);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  
  FastLED.setBrightness(50); //64
  FastLED.clear(true);
  delay(100);
  int i = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 250);
    leds[0][i] = CRGB::Red;
    i++;
    FastLED.show();
  }
 //connectToMQTT();
  dbg("clearing");
  FastLED.clear(true);

  msgList[0]= "          Default Message        ";
  totalMsg=1;
  nextSlot=0;

  dbg("initializing text");
  //ScrollingMsg.SetFont(RobotronFontData);
  ScrollingMsg.SetFont(MatriseFontData);
  ScrollingMsg.Init(&leds, leds.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
  ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0x00);
}

void loop(){
  dbg("LOOP");
  char x = '\x00';
  char tmp[] = {EFFECT_RGB "\x01\x01\xFF 5 -" EFFECT_RGB  "\xFF\x01\x01 5"};  //cant have x00 messes up strings
  showText(tmp);
  delay(5000);
  char tmp2[] = {EFFECT_RGB "\x01\x01\xFF 6 -" EFFECT_RGB  "\xFF\x01\x01 5"};
  showText(tmp2);
  delay(3000);
  char tmp3[] = {EFFECT_RGB "\x01\x01\xFF 6 -" EFFECT_RGB  "\xFF\x01\x01 6"};
  showText(tmp3);
  delay(2000);
}

void showText(String msg){
  FastLED.clear(true);
  int len = msg.length();
  char * mStr = (char *)malloc(len+1);
  msg.toCharArray(mStr,len+1);
  mStr[len]='\0';
  ScrollingMsg.SetText((unsigned char *)mStr, len);
  ScrollingMsg.UpdateText();
  FastLED.show();
}

void loop2()
{
  if (ScrollingMsg.UpdateText() == -1){
    if (msgStr)
      free(msgStr);
    mqttclient.loop();
    delay(150);
    currentMsgIdx++;
    int max = (totalMsg > MAXMSG ? MAXMSG : totalMsg);
    if (currentMsgIdx >= max){
      currentMsgIdx = 0;
    }
    dbg("Displaying msgidx "+String(currentMsgIdx));
    int len = msgList[currentMsgIdx].length();
   // char textPtr[len+1];
    msgStr = (char *)malloc(len+1);
    dbg("Length of msg: "+String(len));
  
    /*msgList[currentMsgIdx].toCharArray(textPtr,len+1);
    textPtr[len]='\0';
    dbg("length of textptr"+String(strlen(textPtr)));
    dbg("Setting text to ");
    dbg(String(textPtr));*/

    msgList[currentMsgIdx].toCharArray(msgStr,len+1);
    msgStr[len]='\0';
  
    ScrollingMsg.SetText((unsigned char *)msgStr, len);
   
 
  }
  else
    FastLED.show();
  delay(10);
 
}

void connectToMQTT(){
  mqttclient.setServer("pizero1", 1883);
 
  String clientId = "TICKER-";
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      dbg("Connected to MQTT");
      regServer();
      mqttclient.setCallback(msgrcvd);
      mqttclient.subscribe("ticker/#");
  } else {
      dbg("mqtt failed, rc=");
      dbg(String(mqttclient.state()));   
  }
  
}

void msgrcvd(char* topic, byte* payload, unsigned int length) {
  /* DONT MAKE NETWORK CALLS HERE */
  String topicStr(topic);
  payload[length] = '\0';
  String msg ((char *)payload);
  /* debug */
  dbg("Message arrived: "+topicStr+"/"+msg);
  if (topicStr == "ticker/addmsg"){
    msgList[nextSlot]= msg;
    if (nextSlot!=0){
      totalMsg++;
    }
    nextSlot++;
    if (nextSlot >= MAXMSG){
      nextSlot = 0;
    }
  }
}

void dbg(String s){
  Serial.println(s);
  Serial.flush();
}

void regServer(){
  String s = "Ticker:"+WiFi.localIP().toString();
  dbg(s);
  char temp[100];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("regserver",temp);
  mqttclient.loop();
}
