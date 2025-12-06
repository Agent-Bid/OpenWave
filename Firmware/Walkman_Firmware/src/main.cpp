#include <Wire.h>
#include <cstdlib>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
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
String tracknumber[50];
int trackselectindex = 0;
unsigned long lastcheck = 0;
unsigned long screenupdate = 0;
unsigned char playerstate;
File root;
//const char* SSID; //Remove before pushing pls
//const char* PASSWORD; //Remove before pushing pls
//const char* ntpserver = "pool.ntp.org";
//const long gmtoffset = 19800;
//const long daylightoffset = 0;
//int recount = 0;
enum {home, song, tracks, stopped, settingsmenu};


void setup() {
  Serial.begin(115200);
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
  /*display.println(F("Connecting to WiFi"));
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED && recount < 20){
    delay(500);
    display.println(F("Waiting"));
    recount++;
    display.display();
    display.clearDisplay();
    display.setCursor(0, 0);
  }
  if(WiFi.status() == WL_CONNECTED){
    display.println(F("Time accquired, killing WiFi")); //Lowkey killed the code too 
    WiFi.disconnect(true); 
    WiFi.mode(WIFI_OFF);
  }
  else{
    display.println(F("Offline Mode"));
    WiFi.disconnect(true); 
    WiFi.mode(WIFI_OFF);
  }
  configTime(gmtoffset, daylightoffset, ntpserver);*/
  display.println(F("ESP Walkman V1"));
  display.display();
  delay(750);
}

void timekeeper();
void trackselector();
void playersettings();
//void datetime();

void homescreen() {
  if(digitalRead(downbutton) == HIGH){
    while(digitalRead(downbutton) == HIGH);
    menucounter = !menucounter;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("WiFi Stack Disabled"));
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

/*void datetime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    display.println(F("Time not set"));
  } else{
    display.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

}*/

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
  int count = 0;
  File root = SD.open("/");
  if(!root){
    display.println(F("SD Card Error"));
    display.display();
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Your Tracks: "));
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
  String name = entry.name();
  if (name.length() > 20){
    name = name.substring(0, 17) + "...";
  }
  display.setTextSize(1);
  display.println(name);
  entry.close();
  count++;
  if (count >= 5){
    break;
   }
  }
  display.display();
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


void loop() {
  switch(playerstate){
    case home:
    homescreen();
    if(digitalRead(selectbutton) == HIGH && menucounter == 0){
      while(digitalRead(selectbutton) == HIGH);
      delay(100);
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



