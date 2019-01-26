//#define FASTLED_ALLOW_INTERRUPTS 0  //may help with flickering , may cause other headaches https://github.com/FastLED/FastLED/issues/306
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

//FLICKERING
//https://www.youtube.com/watch?v=s5yLLtPrKAM&feature=youtu.be&t=308

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

#define MODE_SCOREBOARD 1
#define MODE_NEWS 2

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;
cLEDText ScrollingMsg;

//BOOKEEPING
int hasWifi = 1;
int isScrollingText = 0;
String msgList[MAXMSG];
char * msgStr;
int currentMsgIdx=0;
int totalMsg=0;
int nextSlot = 0;

//SCOREBOARD
int GAMEMAX_LIST[] = {11, 15, 21, 50};
int gameMaxIdx = 0;
int currentBlueScore = -1;
int currentRedScore = -1;

int mode = MODE_SCOREBOARD;

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
  FastLED.setBrightness(25); //64
  FastLED.clear(true);
  delay(100);
  int i = 0;
 while ( WiFi.status() != WL_CONNECTED ) {
    delay (250);
    leds[0][i] = CRGB::Red;
    i++;
    FastLED.show();
    if (i>40){
      hasWifi = 0;
    }
  }
  connectToMQTT();

  dbg("clearing");
  FastLED.clear(true);

  msgList[0]= "          Default Message        ";
  totalMsg=1;
  nextSlot=0;

  dbg("initializing text");
  ScrollingMsg.SetFont(RobotronFontData);
  //ScrollingMsg.SetFont(MatriseFontData);
  ScrollingMsg.Init(&leds, leds.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
  if (hasWifi==0){
    scrollText("   NO WIFI!     WELCOME ");
  } else {
    scrollText("     WELCOME APAP'S ");
  }
}

//BUTTON HANDLERS

void blueBtnLongPressed(){
  if (mode == MODE_SCOREBOARD){
    if (currentBlueScore>0)
      currentBlueScore--;
    checkGame();
  }
}

void redBtnLongPressed(){
  if (mode == MODE_SCOREBOARD){
    if (currentBlueScore>0)
      currentRedScore--;
    checkGame();
  }
}

void blueBtnPressed(){
  if (mode == MODE_SCOREBOARD){
    currentBlueScore++;
    checkGame();
  }
}

void redBtnPressed(){
  if (mode == MODE_SCOREBOARD){
    currentRedScore++;
    checkGame();
  }
}

void menuBtnPressed(){
  if (mode == MODE_SCOREBOARD){
    if (hasScore){
      //nothing
    } else {
      dbg("change max score");
      gameMaxIdx++;
      int arrLen = sizeof(GAMEMAX_LIST) / sizeof(GAMEMAX_LIST[0]);
      if (gameMaxIdx >= arrLen){
        gameMaxIdx = 0;
      }
    }
  }
}

void menuBtnLongPressed(){
  if (mode == MODE_SCOREBOARD){
    if (hasScore){
      dbg("resetting score");
      startGame();
    } else {
      dbg("switch mode");
    }
  }
}

// ***********SCOREBOARD STUFF***********

bool hasScore(){
  if (currentBlueScore > 0 || currentRedScore >0)
    return true;
  else 
    return false;
}

void checkGame(){
  showScore(currentBlueScore,currentRedScore);
  //TODO 
}

void startGame(){
  dbg("Start game");
  scrollText("      GAME to "+GAMEMAX_LIST[gameMaxIdx]);
  while (isScrollingText==1){
    
  }
  currentBlueScore=0;
  currentRedScore=0;
  checkGame();
}


void showScore(int blue, int red){
  char blueScore[2];
  blueScore[0] = (int)'0'+blue / 10;
  blueScore[1] = (int)'0'+blue % 10;
  char redScore[2];
  redScore[0] = (int)'0'+red / 10;
  redScore[1] = (int)'0'+red % 10;
  char tmp[]= {'\xe0','\x01','\x01','\xFF',' ',blueScore[0],blueScore[1],'-','\xe0','\xFF','\x01','\x01',redScore[0],redScore[1],'\0'};
  ScrollingMsg.SetFont(RobotronFontData); 
  showText(tmp);
}

/* *********** THE LOOP *********** */

void loop(){
  //dbg("LOOP");
  if (isScrollingText == 0){
    if (mode == MODE_SCOREBOARD){
      //dbg("mode is scoreboard");
      if(currentBlueScore < 0){
        startGame();
      }
    } else {
      //dbg("Mode not supported "+mode);    //test this
      showText("Error 1x"+mode);
    }
  }
  //handle any scrolling text
  
  if (isScrollingText == 1){
    if (ScrollingMsg.UpdateText() == -1){
      isScrollingText= 0;
      dbg("scrolling complete");
    }
    FastLED.show();
  } 
}


//LED UTILS
void showText(String msg){
  FastLED.clear(true);
  int len = msg.length();
  char * mStr = (char *)malloc(len+1);
  msg.toCharArray(mStr,len+1);      //malloc with no free, is this a problem?
  mStr[len]='\0';
  ScrollingMsg.SetText((unsigned char *)mStr, len);
  ScrollingMsg.UpdateText();
  FastLED.show();
}

void scrollText(String msg){
  ScrollingMsg.SetFrameRate(3);
  ScrollingMsg.SetFont(MatriseFontData);
  //ScrollingMsg.SetFont(RobotronFontData);
  //ScrollingMsg.SetScrollDirection(SCROLL_UP);
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0x00);
  FastLED.clear(true);
  int len = msg.length();
  char * mStr = (char *)malloc(len+1);  //malloc with no free, is this a problem?
  msg.toCharArray(mStr,len+1);
  mStr[len]='\0';
  ScrollingMsg.SetText((unsigned char *)mStr, len);
  isScrollingText = 1;
  while (ScrollingMsg.UpdateText() != -1){
      dbg("scrolling");
      FastLED.show();
  }
  dbg("scrolling complete");
  isScrollingText = 0;   
}

/*
void loop2()
{
  if (ScrollingMsg.UpdateText() == -1){
    if (msgStr){
      free(msgStr);
    }
    mqttclient.loop();
    delay(150);
    currentMsgIdx++;
    int max = (totalMsg > MAXMSG ? MAXMSG : totalMsg);
    if (currentMsgIdx >= max){
      currentMsgIdx = 0;
    }
    
    dbg("Displaying msgidx "+String(currentMsgIdx));
    int len = msgList[currentMsgIdx].length();
    msgStr = (char *)malloc(len+1);
    dbg("Length of msg: "+String(len));
  

    msgList[currentMsgIdx].toCharArray(msgStr,len+1);
    msgStr[len]='\0';
  
    ScrollingMsg.SetText((unsigned char *)msgStr, len);
   
 
  }
  else{
    FastLED.show();
  }
  delay(10);
 
}

*/

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
  } else if (topicStr == "ticker/button"){
    dbg("btn click "+msg);
  }
}

void dbg(String s){
  Serial.println(s);
  Serial.flush();
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

void regServer(){
  String s = "Ticker:"+WiFi.localIP().toString();
  dbg(s);
  char temp[100];
  s.toCharArray(temp, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("regserver",temp);
  mqttclient.loop();
}
