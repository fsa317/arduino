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
#define LED_PIN        7  //GPIO13 / D7 on NODEMCU
#define COLOR_ORDER    GRB
#define CHIPSET        WS2812B
#define MATRIX_WIDTH   60
#define MATRIX_HEIGHT  7
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX

#define RED_PIN   D2      //Dont use D0 seems to be a problem
#define BLUE_PIN  D1
#define MENU_PIN  D5

#define NOCLICK 0
#define REGCLICK 1
#define LONGCLICK 2

#define LONGCLICK_THRESHOLD 800

#define MAXMSG 10
#define MODE_SCOREBOARD 1
#define MODE_MSGS 2
#define MODE_COUNT 2

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;
cLEDText ScrollingMsg;

//BOOKEEPING
int hasWifi = 1;
int isScrollingText = 0;
String msgList[MAXMSG];
int currentMsgIdx=MAXMSG;
int redBtnState = HIGH;
int blueBtnState = HIGH;
int menuBtnState = HIGH;
unsigned long lastDebounceTime = 0;
int waitingForSource = 0;

char * mStr;


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

  pinMode(RED_PIN,INPUT_PULLUP);
  pinMode(BLUE_PIN,INPUT_PULLUP);
  pinMode(MENU_PIN,INPUT_PULLUP);
  
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  FastLED.setBrightness(48); //64
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
  
  setDefaultMsgList();
  getNextSource();
  dbg("initializing text");
  ScrollingMsg.SetFont(RobotronFontData);
  //ScrollingMsg.SetFont(MatriseFontData);
  ScrollingMsg.Init(&leds, leds.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
  if (hasWifi==0){
    scrollText("   NO WIFI!     WELCOME ");
  } else {
    scrollText("     TIME FOR SOME FUN! ");
  }
  dbg("setup done");
  printFreeHeap();
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
    if (currentRedScore>0)
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
    if (hasScore()==true){
      //nothing
    } else {
      dbg("change max score");
      gameMaxIdx++;
      int arrLen = sizeof(GAMEMAX_LIST) / sizeof(GAMEMAX_LIST[0]);
      if (gameMaxIdx >= arrLen){
        gameMaxIdx = 0;
      }
      startGame();
    }
  }
}

void menuBtnLongPressed(){
  if (mode == MODE_SCOREBOARD){
    if (hasScore()){
      dbg("resetting score");
      startGame();
    } else {
      dbg("switch mode");
      mode++;
      currentBlueScore = -1;
      if (mode > MODE_COUNT){
        mode = 1;
      }
    }
  } else {
    dbg("switch mode");
    mode++;
    if (mode > MODE_COUNT){
      mode = 1;
    }
    isScrollingText = 0;
    currentMsgIdx=MAXMSG;
  }
}

// *********** MSG STUFF ***************

void setDefaultMsgList(){
  msgList[0] = paddedMsg("Lets Go Mets.");
  msgList[1] = paddedMsg("This is a default message");
  msgList[2] = paddedMsg("The last msg");
}

void doSetMsg(char * msg){
  int idx = (int)msg[0]-(int)'0';
  msgList[idx] = paddedMsg(String(msg+1));
  waitingForSource = 0;
}

void clearMsgs(){
  for( unsigned int a = 0; a < MAXMSG; a = a + 1 ){
    msgList[a] ="";
  }
}

void scrollNextMessage(){
  if (currentMsgIdx >= MAXMSG){
    currentMsgIdx = 0;
  } else {
    currentMsgIdx++;
  }
  if (currentMsgIdx == (MAXMSG-1)){
    
    if (waitingForSource == 0){
      dbg("requesting new messages");
      getNextSource();
    }
    
  }
  if (msgList[currentMsgIdx] && msgList[currentMsgIdx].length() > 11){     //9 because of padding
    dbg("scrolling msg "+String(currentMsgIdx)+" length "+String(msgList[currentMsgIdx].length()));
    scrollText(msgList[currentMsgIdx]);
  } else {
    dbg("skipping msg "+String(currentMsgIdx));
  }
  
}

String paddedMsg(String msg){
  return "        "+msg+" ";
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
  if (currentBlueScore >= GAMEMAX_LIST[gameMaxIdx]){
    scrollText("    Blue Player Wins!!    ");
    currentBlueScore = 0;
    currentRedScore = 0;
  } else if ( currentRedScore >= GAMEMAX_LIST[gameMaxIdx]){
    scrollText("    Red Player Wins!!    ");
    currentBlueScore = 0;
    currentRedScore = 0;
  }
}

void startGame(){
  dbg("Start game");
  ScrollingMsg.SetFont(MatriseFontData);
  String maxStr = String(GAMEMAX_LIST[gameMaxIdx]);
  showText(" GAME: "+maxStr);
  delay(2000);
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
    } else if (mode == MODE_MSGS) {
      dbg("mode is messages");
      scrollNextMessage();
      printFreeHeap();
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
  
  handleButtons();
  mqttclient.loop();
}

//************* BUTTONS interrupts 

void handleButtons(){
  int state = handleButton(RED_PIN, redBtnState);
  
  if (state == REGCLICK){
    redBtnPressed();
  } else if (state == LONGCLICK){
    redBtnLongPressed();
  }
  
  state = handleButton(BLUE_PIN, blueBtnState);
  if (state == REGCLICK){
    blueBtnPressed();
  } else if (state == LONGCLICK){
    blueBtnLongPressed();
  }
  
  state = handleButton(MENU_PIN, menuBtnState);
  if (state == REGCLICK){
    menuBtnPressed();
  } else if (state == LONGCLICK){
    menuBtnLongPressed();
  }
}


int handleButton(int pin, int &btnstate){
  int val = digitalRead(pin);
  int clicked = NOCLICK;
  if (val != btnstate){
    dbg("New button"+String(pin)+" state "+String(val));
    btnstate = val;
    if (btnstate == HIGH){
      unsigned long pushTime = millis() - lastDebounceTime;
      if (pushTime > LONGCLICK_THRESHOLD){
        clicked = LONGCLICK;
      } else {
        clicked = REGCLICK;
      }
    } else {
      //capture when button was pushed
      lastDebounceTime = millis();
      printFreeHeap();
    }
  }
  return clicked;
}

//*************LED UTILS
void showText(String msg){
  FastLED.clear(true);
  int len = msg.length();
  if (mStr)
    free(mStr);
  mStr = (char *)malloc(len+1);
  msg.toCharArray(mStr,len+1);      
  mStr[len]='\0';
  ScrollingMsg.SetText((unsigned char *)mStr, len);
  ScrollingMsg.UpdateText();
  isScrollingText = 0;
  FastLED.show();
  
}

void scrollText(String msg){
  ScrollingMsg.SetFrameRate(2);   //1 is fastest
  ScrollingMsg.SetFont(MatriseFontData);
  //ScrollingMsg.SetFont(RobotronFontData);
  //ScrollingMsg.SetScrollDirection(SCROLL_UP);
  //ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0x00);
  ScrollingMsg.SetTextColrOptions(COLR_HSV | COLR_GRAD_AH, 0x00, 0xff, 0xff,0xff,0xff,0xff); //\x00\xff\xff\xff\xff\xff
  FastLED.clear(true);
  int len = msg.length();
  if (mStr)
    free(mStr);
  mStr = (char *)malloc(len+1);  //malloc with no free, is this a problem?
  msg.toCharArray(mStr,len+1);
  mStr[len]='\0';
  ScrollingMsg.SetText((unsigned char *)mStr, len);
  isScrollingText = 1;
}


/** NETWORK STUF **/

void msgrcvd(char* topic, byte* payload, unsigned int length) {
  /* DONT MAKE NETWORK CALLS HERE */
  dbg("msg rcvd");
  payload[length] = '\0';
  /* debug */
  //dbg("Message arrived: "+String(topic)+"/"+msg);
  if (strcmp(topic,"toticker/setmsg")==0){
    dbg("setmsg called");
    doSetMsg((char *)payload);
  } else if (strcmp(topic,"toticker/clearmsgs")==0){
    dbg("clearmsgs called");
    clearMsgs();
  }
    else if (strcmp(topic,"toticker/btn")==0){
    String msg ((char *)payload);
    dbg("btn click "+msg);
    doMQButtonPress(msg);
  }
  printFreeHeap();
}

void doMQButtonPress(String btn){
  if (btn == "bluebtn"){
    blueBtnPressed();
  } else if (btn == "bluebtn_long"){
    blueBtnLongPressed();
  } else if (btn == "redbtn"){
    redBtnPressed();
  } else if (btn == "redbtn_long"){
    redBtnLongPressed();
  } else if (btn == "menubtn"){
    menuBtnPressed();
  } else if (btn == "menubtn_long"){
    menuBtnLongPressed();
  } 
  else {
    dbg("button "+btn+" not supported ");
  }
}

void dbg(String s){
  Serial.println(s);
  Serial.flush();
}

void printFreeHeap(){
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void getNextSource(){
  dbg("getNextSource");
  waitingForSource = 1;
  mqttclient.publish("fromticker/getnextsource","getnextsource");
}

void connectToMQTT(){
  mqttclient.setServer("pizero1", 1883);
 
  String clientId = "TICKER-";
  clientId += String(random(0xffff), HEX);
  if (mqttclient.connect(clientId.c_str())) {
      dbg("Connected to MQTT");
      regServer();
      mqttclient.setCallback(msgrcvd);
      mqttclient.subscribe("toticker/#");
      
  } else {
      dbg("mqtt failed, rc=");
      dbg(String(mqttclient.state()));   
  }
  
}

void regServer(){
  String s = "Ticker:"+WiFi.localIP().toString();
  dbg(s);
  char ip[100];
  s.toCharArray(ip, s.length() + 1); //packaging up the data to publish to mqtt whoa...
  mqttclient.publish("regserver",ip);
  mqttclient.loop();
}
