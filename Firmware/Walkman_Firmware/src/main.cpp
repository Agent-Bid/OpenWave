#include <Wire.h>
#include <cstdlib>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <secrets.h>
#include <WiFi.h>
#include <time.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int selectbutton = 25;
int backbutton = 26;
int pausebutton = 27;
int downbutton = 13;
int TotalTime = 195;
int PassedTime = 0;
int menucounter = 0;
int settingscounter = 0;
int tracksfound = 0;
int trackselectindex = 0;
bool wifirequest;
String tracklist[50];
unsigned long lastcheck = 0;
unsigned long screenupdate = 0;
const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
unsigned char playerstate;
File root;
const char* ntpserver = "pool.ntp.org";
const long gmtoffset = 19800;
const long daylightoffset = 0;
enum {home, song, tracks, stopped, settingsmenu};

void timekeeper();
void trackselector();
void playersettings();
void populatetracklist();
void networkmanager(void * parameter);

void setup() {
  Serial.begin(9600);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display Not Found"));
    for(;;);
  }
  pinMode(selectbutton, INPUT);
  pinMode(backbutton, INPUT);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Testing SD Card"));
  if(SD.begin(5)) {
    display.println(F("SD card found"));
  }
  else {
    display.println(F("SD card not found"));
    display.display();
    for(;;);
  }
  TaskHandle_t xHandle = NULL;
  xTaskCreatePinnedToCore(networkmanager,"WiFiCheck",10000,NULL,0,&xHandle,0);
  wifirequest = true;
  display.println(F("ESP Walkman V1"));
  display.display();
}

void homescreen() {
  struct tm timeinfo;
  if(digitalRead(downbutton) == HIGH){
    while(digitalRead(downbutton) == HIGH);
    menucounter = !menucounter;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2,0);
  if(getLocalTime(&timeinfo,0)){
    display.printf("Time:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }
  else{
    display.println("Time Syncing");
  }
  if(menucounter == 0){
  display.fillCircle(4, 18, 2, WHITE);
  display.setCursor(10,15);
  display.println(F("View Library"));
  display.drawCircle(4, 27, 2, WHITE);
  display.setCursor(10, 24);
  display.println(F("Settings"));
  display.display();
  } 
  else {
  display.drawCircle(4, 18, 2, WHITE);
  display.setCursor(10,15);
  display.println(F("View Library"));
  display.fillCircle(4, 27, 2, WHITE);
  display.setCursor(10, 24);
  display.println(F("Settings"));
  display.display();
  }
  }

void updatescreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Now Playing: "));
  display.setTextSize(2);
  display.setCursor(0,15);
  display.println(F("Track 01"));

  int currentminutes = PassedTime / 60;
  int currentseconds = PassedTime % 60;

  display.setTextSize(1);
  display.setCursor(0, 35);
  if (currentminutes < 10) display.print("0");
  display.print(currentminutes);
  display.print(":");
  if (currentseconds < 10) display.print("0");
  display.print(currentseconds);

  display.print(F(" / 3:15"));
  int bars = 16;
  int spacing = 2;
  int barwidth = (128 - (bars * spacing)) / bars;

  for(int i = 0; i < bars; i++) {
    int height = random(2, 20);
    int x = i * (barwidth + spacing);
    int y = 64 - height;
    display.fillRect(x, y, barwidth, height, WHITE);

  }
  display.display();
}

void paused() {}

void timekeeper() {
  if(millis() - lastcheck > 1000) {
    lastcheck = millis();
    if (PassedTime < TotalTime) {
      PassedTime++;
    } else {
      PassedTime = 0;
    }
  }
  if(millis() - screenupdate > 85) {
    screenupdate = millis();
    updatescreen();
   }
  }

void trackselector() {
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Your Tracks: ");
  display.setCursor(0,15);
 for(int x = 0; x < tracksfound; x++){
   String displayname = tracklist[x];
   if(displayname.length() > 20){
    displayname = displayname.substring(0, 17) + "...";
   }
   display.println(displayname);
 }
 display.display();
}

void populatetracklist(){
  tracksfound = 0;
  File root = SD.open("/");
  if (!root) {
    display.println(F("SD Error"));
    display.display();
    return;
 }
  root.rewindDirectory();
  while (true){
    File entry = root.openNextFile();
    if(!entry){
      break;
    }
    if(entry.isDirectory()){
      entry.close();
      continue;
    }
    tracklist[tracksfound] = entry.name();
    tracksfound ++;
    if(tracksfound > 50){
      break;
    }
  }
  root.close();
}

void playersettings(){
    if(digitalRead(downbutton) == HIGH){
    while(digitalRead(downbutton) == HIGH);
    settingscounter = !settingscounter;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Settings"));
  if(settingscounter == 0){
  display.fillCircle(4, 18, 2, WHITE);
  display.setCursor(10,15);
  display.println(F("Bluetooth"));
  display.drawCircle(4, 27, 2, WHITE);
  display.setCursor(10, 24);
  display.println(F("WiFi"));
  display.display();
  } 
  else {
  display.drawCircle(4, 18, 2, WHITE);
  display.setCursor(10,15);
  display.println(F("Bluetooth"));
  display.fillCircle(4, 27, 2, WHITE);
  display.setCursor(10, 24);
  display.println(F("WiFi"));
  display.display();
  }
  }

void networkmanager(void * parameter){
for(;;) {
  if(wifirequest == true){
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int recount = 0;
    while (WiFi.status() != WL_CONNECTED && recount < 10){
    Serial.println(".");
    vTaskDelay(xDelay);
    recount++;
    }
Serial.println("WiFi Connected");
configTime(gmtoffset, daylightoffset, ntpserver);
struct tm timeinfo;
if(getLocalTime(&timeinfo)){
  Serial.println("Time Synced");
}
else{
  Serial.println("Not Connected");
}
wifirequest = false;
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
 }
}

void loop() {
  switch(playerstate){
    case home:
    homescreen();
    if(digitalRead(selectbutton) == HIGH && menucounter == 0){
      while(digitalRead(selectbutton) == HIGH);
      delay(100);
      populatetracklist();
      playerstate = tracks;
      break;
    }
    else if(digitalRead(selectbutton) == HIGH && menucounter == 1){
      while(digitalRead(selectbutton) == HIGH);
      delay(100);
      playerstate = settingsmenu;
      break;
    }
    else{
      break;
    }
    case tracks:
    trackselector();
    if(digitalRead(selectbutton) == HIGH){
      while(digitalRead(selectbutton) == HIGH);
      delay(100);
      playerstate = song;
      break;
    }
    else if(digitalRead(backbutton) == HIGH){
      while(digitalRead(backbutton) == HIGH);
      delay(100);
      playerstate = home;
      break;
    }
    else{
      break;
    }
    case song:
    timekeeper();
    if(digitalRead(pausebutton) == HIGH){
      while(digitalRead(pausebutton) == HIGH);
      delay(100);
      playerstate = stopped;
      break;
    }
    else if(digitalRead(backbutton) == HIGH){
      while(digitalRead(backbutton) == HIGH);
      playerstate = tracks;
      delay(100);
      break;
    }
    else{
      break;
    }
    case stopped:
    paused();
    if(digitalRead(pausebutton) == HIGH){
      while(digitalRead(pausebutton) == HIGH);
        delay(100);
        playerstate = song;
      break;
    }
    else if(digitalRead(backbutton) == HIGH){
      while(digitalRead(backbutton) == HIGH);
        delay(100);
        playerstate = tracks;
      break;
    }
    else{
      break;
    }
    case settingsmenu:
    playersettings();
    if(digitalRead(backbutton) == HIGH){
     while(digitalRead(backbutton) == HIGH);
     delay(100);
     playerstate = home;
     break;
    }
    else{
      break;
    }
    default:
      playerstate = home;
      break;
  }
}




