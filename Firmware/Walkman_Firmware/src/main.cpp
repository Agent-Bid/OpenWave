#include <Wire.h>
#include <cstdlib>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <secrets.h>
#include <WiFi.h>
#include <time.h>
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
int buttondelay = 150;
int lastselectstate = LOW;
int lastbackstate = LOW;
int lastpausestate = LOW;
int lastdownstate = LOW;
int menucursor = 0;
int menustartindex = 0;
int tracksperscreen = 5;
bool wifirequest;
String tracklist[50];
unsigned long lastcheck = 0;
unsigned long lastbuttontime = 0;
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
  if(SD.begin(5, SPI, 4000000)) {
    display.println(F("SD card found"));
  }
  else {
    display.println(F("SD card not found"));
    display.display();
    //for(;;);
  }
  TaskHandle_t xHandle = NULL;
  xTaskCreatePinnedToCore(networkmanager,"WiFiCheck",4000,NULL,0,&xHandle,0);
  wifirequest = true;
  display.println(F("ESP Walkman V1"));
  display.display();
}

void homescreen() {
  struct tm timeinfo;
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
  static char buffer[25];
  int y = 15;
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Your Tracks: ");
  display.setCursor(0,15);
  for(int x = menustartindex; x < (tracksperscreen + menustartindex) ; x++){
   if(x >= tracksfound){break;}
   display.setCursor(0, y);
   if(x == menucursor){
    display.print(">");
   }
   else{
    display.print(" ");
   }
   tracklist[x].toCharArray(buffer, 20);
   if(tracklist[x].length() > 18){
    buffer[15] = '.';
    buffer[16] = '.';
    buffer[17] = '.';
    buffer[18] = '\0';
   }
   display.println(buffer);
   y += 10;
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
Serial.print("water mark : ");
Serial.println(uxTaskGetStackHighWaterMark(NULL));
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
 }
}

void loop() {
  int currentselectstate = digitalRead(selectbutton);
  int currentbackstate = digitalRead(backbutton);
  int currentdownstate = digitalRead(downbutton);
  int currentpausestate = digitalRead(pausebutton);
  switch(playerstate){

    case home:
    homescreen();
    if(lastdownstate == LOW && currentdownstate == HIGH && ((millis() - lastbuttontime) > buttondelay)) {
      menucounter = !menucounter;
      break;
    }
    if(lastselectstate == LOW && currentselectstate == HIGH && ((millis() - lastbuttontime) > buttondelay) && menucounter == 0){
       populatetracklist();
       playerstate = tracks;
       lastbuttontime = millis();
       break;
     }
      else if(lastselectstate == LOW && currentselectstate == HIGH && ((millis() - lastbuttontime) > buttondelay) && menucounter == 1){
       playerstate = settingsmenu;
       lastbuttontime = millis();
       break;
      }
    else{
      break;
    }

    case tracks:
    trackselector();
    if(lastdownstate == LOW && currentdownstate == HIGH && (millis() - lastbuttontime) >  buttondelay){
      menucursor++;
      if(menucursor >= tracksfound){
        menucursor = 0;
        menustartindex = 0;
      }
      else if(menucursor >= (tracksperscreen + menustartindex)){
        menustartindex++;
      }
      lastbuttontime = millis();
      break;
    }
    if(lastselectstate == LOW && currentselectstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      playerstate = song;
      trackselectindex = menucursor;
      lastbuttontime = millis();
      break;
     }
    else if(lastbackstate == LOW && currentbackstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      playerstate = home;
      lastbuttontime = millis();
      break;
     }
    else{
      break;
    }

    case song:
    timekeeper();
    if(lastpausestate == LOW && currentpausestate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      playerstate = stopped;
      lastbuttontime = millis();
      break;
    }
    else if(lastbackstate == LOW && currentbackstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      playerstate = tracks;
      lastbuttontime = millis();
      break;
    }
    else{
      break;
    }

    case stopped:
    paused();
    if(lastpausestate == LOW && currentpausestate == HIGH && ((millis() - lastbuttontime) > 300)){
      playerstate = song;
      lastbuttontime = millis();
      break;
    }
    else if(lastbackstate == LOW && currentbackstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      playerstate = tracks;
      lastbuttontime = millis();
      break;
    }
    else{
      break;
    }

    case settingsmenu:
    playersettings();
    if(lastdownstate == LOW && currentdownstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
      settingscounter = !settingscounter;
      break;
    }
    if(lastbackstate == LOW && currentbackstate == HIGH && ((millis() - lastbuttontime) > buttondelay)){
     playerstate = home;
     lastbuttontime = millis();
     break;
    }
    else{
      break;
    }
    default:
      playerstate = home;
      break;
  }

  lastselectstate = currentselectstate;
  lastbackstate = currentbackstate;
  lastdownstate = currentdownstate;
  lastpausestate = currentpausestate;
}




