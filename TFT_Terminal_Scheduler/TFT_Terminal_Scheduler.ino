#include <TaskSchedulerSleepMethods.h>
#include <TaskSchedulerDeclarations.h>
#include <TaskScheduler.h>
Scheduler runner;

#include <WiFi.h>
#define WIFI_SSID "********"
#define WIFI_PASSWD "********"
#define PORT 1024

#include <WiFiClient.h>
WiFiClient client;
#define TCP_HOST "192.168.1.69" // TCP Server address
#define TCP_PORT 1024 // TCP Server port

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite disply = TFT_eSprite(&tft); // Scrolling display sprite

uint16_t yDraw = 220;
uint16_t xPos = 0;

char buffer[256] = "First line\nSecond long line full of text about something something something...\nNew line";

char netbuffer[256];

char serialbuffer[256];
byte serialbytes = 0;


////////// Display vertical scrolling printer begin //////////
void printftc(char c, TFT_eSprite &disply) {
  if (xPos >= 126 || c == '\n' || c == '\r') {
    //disply.scroll(0, -16);
    for (int x = 0; x < 16; x++) { disply.scroll(0, -1); disply.pushSprite(0, 0); /*delay(2);*/ }
    /*yDraw+= 16;*/ xPos = 0;
  }
  xPos += disply.drawChar(c, xPos, yDraw, 2);
  disply.pushSprite(0, 0);
}

void printft(char buffer[], uint16_t sz, TFT_eSprite &disply) {
  uint16_t i = 0;
  while (buffer[i] != '\0') {
    printftc(buffer[i], disply);
    i++;
  }
  //scroll?
}

void printftln(char buffer[], uint16_t sz, TFT_eSprite &disply) {
  //scroll?
  printft(buffer, sz, disply);
  xPos = 0; //buffer[0] == '\r' ? 0 : xPos;
  for (int x = 0; x < 16; x++) { disply.scroll(0, -1); disply.pushSprite(0, 0); /*delay(2);*/ }
}

void printft(char buffer[]) {
  printft(buffer, 100, disply);
}

void printftln(char buffer[]) {
  printftln(buffer, 100, disply);
}

void printft(String str) {
  char buf[str.length()];
  str.toCharArray(buf, str.length());
  printft(buf);
}

void printftln(String str) {
  char buf[str.length()];
  str.toCharArray(buf, str.length());
  printftln(buf);
}
////////// Display vertical scrolling printer end //////////




//==========================================================================================
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tft.setSwapBytes(true);

  ////////// Display sprite creation
  disply.setColorDepth(8);
  disply.createSprite(135, 240); ////////// Sprite wider than the display plus the text to allow text to scroll from the right.
  disply.fillSprite(TFT_BLACK);
  disply.setScrollRect(0, 0, 135, 240, TFT_BLACK); ////////// Sprite wider than the display plus the text to allow text to scroll from the right.
  disply.setTextColor(TFT_WHITE); ////////// White text, no background


  ////////// WiFi init
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  printftln("Connecting to");
  printft(WIFI_SSID);

  ////////// Connecting to WiFi
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    printftc('.', disply);
    delay(500);
    if ((++i % 16) == 0) {
      printftln(" still trying to connect");
    }
  }
  printftln("");

  ////////// IPAddress to char array convertion
  char IP[] = "xxx.xxx.xxx.xxx"; 
  IPAddress ip = WiFi.localIP();
  ip.toString().toCharArray(IP, 16);
  
  printftln("Connected.\nMy IP address is:");
  printftln(IP);


  ////////// Connecting to TCP server
  if (!client.connect(TCP_HOST, TCP_PORT)) {
    printftln("Link to host failed");
    delay(1000);
    return;
  }
  printftln("Link successful");


  printft("\nFree HEAP: ");
  printftln(  String(ESP.getFreeHeap()) );
}

//==========================================================================================//



void loop() {
  runner.execute(); // Scheduler execute
}



Task SerialDataIO(0, TASK_FOREVER, []() {
  ////////// Read data from Network
  if (!client.connect(TCP_HOST, TCP_PORT)) {
    printftln("Link to host failed");
    delay(1000);
    return;
  }
  client.write("MSG");
  client.readBytesUntil('\n', netbuffer, 256); // readBytes without until if shit
  printft("-> ");
  printftln(netbuffer);

  ////////// Read data from Serial
  if (Serial.available()) {
    serialbytes = Serial.readBytesUntil('\n', serialbuffer, 256);
    serialbuffer[serialbytes] = '\0';
    if (serialbuffer > 0) {
      printft("usb -> ");
      printftln(serialbuffer);
    }
  }
}, &runner, true, NULL , NULL );
