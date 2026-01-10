#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VS1053.h>
#include <Arduino.h>
#include <PowerManagement.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <cstdlib>
#include <iostream>
#include <secrets.h>
#include <symbols.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDCS 5
#define XDCS 4
#define CS 33
#define RESET 15
#define DREQ 35

int selectbutton = 25;
int backbutton = 26;
int pausebutton = 27;
int downbutton = 13;
int PassedTime;
int menucounter = 0;
int settingscounter = 0;
int tracksfound = 0;
int trackselectindex = 0;
int buttondelay = 150;
int lastselectstate = LOW;
int lastbackstate = LOW;
int lastpausestate = LOW;
int lastdownstate = LOW;
int menustartindex = 0;
int tracksperscreen = 5;
int currentsongduration = 0;
int eqmenuindex = 0;
int bassamp = 15;         // Default Sound Settings
int trebleamp = 6;        // Default Sound Settings
int bassfreq = 12;        // Default Sound Settings
int treblefreq = 10;      // Default Sound Settings
int leftchannelvol = 10;  // Default Sound Settings
int rightchannelvol = 10; // Default Sound Settings
bool wifirequest;
unsigned long lastcheck = 0;
unsigned long lastbuttontime = 0;
unsigned long screenupdate = 0;
const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
unsigned char playerstate;
char currentsongname[200];
char tracklist[50][30];
char buffer[50];
File root;
const char *ntpserver = "pool.ntp.org";
const long gmtoffset = 19800;
const long daylightoffset = 0;
enum { home, song, tracks, stopped, settingsmenu, equalizermenu };

Adafruit_VS1053_FilePlayer walkman =
    Adafruit_VS1053_FilePlayer(RESET, CS, XDCS, DREQ, SDCS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void timekeeper();
void trackmenu();
void playersettings();
void populatetracklist();
void trackselector();
void equalizer();
void equalizerfunc();
void networkmanager(void *parameter);

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display Not Found"));
    for (;;)
      ;
  }
  pinMode(selectbutton, INPUT);
  pinMode(backbutton, INPUT);
  pinMode(downbutton, INPUT);
  pinMode(pausebutton, INPUT);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Testing Audio Systems"));
  if (!walkman.begin()) {
    display.println(F("Audio Systems Malfunctioning"));
    display.display();
    for (;;)
      ;
  } else {
    display.println(F("Audio Systems Functional"));
  }
  walkman.setVolume(leftchannelvol, rightchannelvol);
  // walkman.useInterrupt(DREQ);
  display.println(F("Testing SD Card"));
  if (SD.begin(5, SPI, 4000000)) {
    display.println(F("SD card found"));
  } else {
    display.println(F("SD card not found"));
    display.display();
    // for(;;);
  }
  TaskHandle_t xHandle = NULL;
  xTaskCreatePinnedToCore(networkmanager, "WiFiCheck", 4000, NULL, 0, &xHandle,
                          0);
  wifirequest = true;
  display.println(F("ESP Walkman V1"));
  display.display();
}

void homescreen() {
  struct tm timeinfo;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2, 0);
  if (getLocalTime(&timeinfo, 0)) {
    display.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    display.println("Time Syncing");
  }
  display.drawBitmap(110, 0, battery, 16, 8, WHITE);
  if (menucounter == 0) {
    /*display.fillCircle(4, 18, 2, WHITE);
    display.setCursor(10,15);
    display.println(F("View Library"));
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Settings"));
    display.display();*/
    display.drawBitmap(35, 13, musicnote1, 32, 32, WHITE);
    display.drawBitmap(60, 10, musicnote2, 32, 32, WHITE);
    display.setCursor(25, 45);
    display.setTextSize(2);
    display.println(F("Tracks"));
    display.display();
  } else {
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
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
  display.setCursor(0, 0);
  display.println(F("Now Playing: "));
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.println(currentsongname + 1);

  int currentminutes = PassedTime / 60;
  int currentseconds = PassedTime % 60;

  display.setTextSize(1);
  display.setCursor(0, 35);
  if (currentminutes < 10)
    display.print("0");
  display.print(currentminutes);
  display.print(":");
  if (currentseconds < 10)
    display.print("0");
  display.print(currentseconds);
  display.print(F(" / "));

  int totalminutes = currentsongduration / 60;
  int totalseconds = currentsongduration % 60;
  // if (totalminutes < 10) display.print("0");
  display.print(totalminutes);
  display.print(':');
  if (totalseconds < 10)
    display.print("0");
  display.print(totalseconds);

  int bars = 16;
  int spacing = 2;
  int barwidth = (128 - (bars * spacing)) / bars;

  for (int i = 0; i < bars; i++) {
    int height = random(2, 20);
    int x = i * (barwidth + spacing);
    int y = 64 - height;
    display.fillRect(x, y, barwidth, height, WHITE);
  }
  display.display();
}

void paused() {}

void timekeeper() {
  if (millis() - lastcheck > 1000) {
    lastcheck = millis();
    if (PassedTime < currentsongduration) {
      PassedTime++;
    } else {
      PassedTime = 0;
    }
  }
  if (millis() - screenupdate > 85) {
    screenupdate = millis();
    updatescreen();
  }
}

void trackmenu() {
  // static char buffer[25];
  int y = 15;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Your Tracks: ");
  display.setCursor(0, 15);
  for (int x = menustartindex; x < (tracksperscreen + menustartindex); x++) {
    if (x >= tracksfound) {
      break;
    }
    display.setCursor(0, y);
    if (x == trackselectindex) {
      display.print(">");
    } else {
      display.print(" ");
    }
    strcpy(buffer, tracklist[x]);
    if (strlen(buffer) > 18) {
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

void populatetracklist() {
  tracksfound = 0;
  File root = SD.open("/");
  if (!root) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("SD Error"));
    display.display();
    // for(;;);
    return;
  }
  root.rewindDirectory();
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }
    strncpy(tracklist[tracksfound], entry.name(), 29);
    tracklist[tracksfound][29] = '\0';
    tracksfound++;
    if (tracksfound >= 50) {
      break;
    }
    entry.close();
  }
  root.close();
}

void playersettings() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Settings"));
  if (settingscounter == 0) {
    display.fillCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bluetooth"));
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("WiFi"));
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println("Equalizer");
    display.display();
  } else if (settingscounter == 1) {
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bluetooth"));
    display.fillCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("WiFi"));
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println("Equalizer");
    display.display();
  } else {
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bluetooth"));
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("WiFi"));
    display.fillCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println("Equalizer");
    display.display();
  }
}

void networkmanager(void *parameter) {
  for (;;) {
    if (wifirequest == true) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      int recount = 0;
      while (WiFi.status() != WL_CONNECTED && recount < 10) {
        Serial.println(".");
        vTaskDelay(xDelay);
        recount++;
      }
      Serial.println("WiFi Connected");
      configTime(gmtoffset, daylightoffset, ntpserver);
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        Serial.println("Time Synced");
      } else {
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

void trackselector() {
  currentsongname[0] = '\0';
  int i;
  int loopcounter = 0;
  walkman.stopPlaying();
  File root = SD.open("/");
  root.rewindDirectory();
  while (true) {
    File current = root.openNextFile();
    if (!current) {
      break;
    }
    if (current.isDirectory()) {
      current.close();
      continue;
    }
    if (loopcounter == trackselectindex) {
      strcpy(currentsongname, "/");
      strncat(currentsongname, current.name(), 100);
      currentsongname[100] = '\0';
      currentsongduration = current.size() / 32000;
      current.close();
      // walkman.startPlayingFile(currentsongname);
      // current.close();
      break;
    }
    loopcounter++;
    current.close();
  }
  root.close();
  if (strlen(currentsongname) > 0) {
    walkman.softReset();
    equalizerfunc();
    walkman.startPlayingFile(currentsongname);
  }
}

void equalizerfunc() {
  if (bassfreq > 15 || bassfreq < 0) {
    bassfreq = 0;
  }

  if (bassamp > 15 || bassamp < 0) {
    bassamp = 0;
  }

  if (treblefreq > 15 || treblefreq < 0) {
    treblefreq = 0;
  }

  if (trebleamp > 15 || trebleamp < 0) {
    trebleamp = 0;
  }

  if (rightchannelvol < 10) {
    rightchannelvol = 10;
  }

  if (rightchannelvol > 50) {
    rightchannelvol = 50;
  }

  if (leftchannelvol < 10) {
    leftchannelvol = 10;
  }

  if (leftchannelvol > 50) {
    leftchannelvol = 50;
  }

  uint16_t eqstats = ((trebleamp & 0xF) << 12 | (treblefreq & 0xF) << 8 |
                      (bassamp & 0xF) << 4 | (bassfreq & 0xF));
  walkman.sciWrite(2, eqstats);
}

void equalizer() {

  if (eqmenuindex == 0) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Equalizer"));
    display.fillCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bass Amp: "));
    display.setCursor(62, 15);
    display.print(bassamp);
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Treble Amp: "));
    display.setCursor(75, 24);
    display.print(trebleamp);
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println(F("Bass Frequency: "));
    display.setCursor(98, 33);
    display.print(bassfreq);
    display.drawCircle(4, 45, 2, WHITE);
    display.setCursor(10, 42);
    display.println(F("Treble Frequency: "));
    display.setCursor(110, 42);
    display.print(treblefreq);
    display.drawCircle(4, 54, 2, WHITE);
    display.setCursor(10, 51);
    display.println(F("Volume: "));
    display.setCursor(53, 51);
    display.print(rightchannelvol);
    display.display();
  } else if (eqmenuindex == 1) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Equalizer"));
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bass Amp: "));
    display.setCursor(62, 15);
    display.print(bassamp);
    display.fillCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Treble Amp: "));
    display.setCursor(75, 24);
    display.print(trebleamp);
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println(F("Bass Frequency: "));
    display.setCursor(98, 33);
    display.print(bassfreq);
    display.drawCircle(4, 45, 2, WHITE);
    display.setCursor(10, 42);
    display.println(F("Treble Frequency: "));
    display.setCursor(110, 42);
    display.print(treblefreq);
    display.drawCircle(4, 54, 2, WHITE);
    display.setCursor(10, 51);
    display.println(F("Volume: "));
    display.setCursor(53, 51);
    display.print(rightchannelvol);
    display.display();
  } else if (eqmenuindex == 2) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Equalizer"));
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bass Amp: "));
    display.setCursor(62, 15);
    display.print(bassamp);
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Treble Amp: "));
    display.setCursor(75, 24);
    display.print(trebleamp);
    display.fillCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println(F("Bass Frequency: "));
    display.setCursor(98, 33);
    display.print(bassfreq);
    display.drawCircle(4, 45, 2, WHITE);
    display.setCursor(10, 42);
    display.println(F("Treble Frequency: "));
    display.setCursor(110, 42);
    display.print(treblefreq);
    display.drawCircle(4, 54, 2, WHITE);
    display.setCursor(10, 51);
    display.println(F("Volume: "));
    display.setCursor(53, 51);
    display.print(rightchannelvol);
    display.display();
  } else if (eqmenuindex == 3) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Equalizer"));
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bass Amp: "));
    display.setCursor(62, 15);
    display.print(bassamp);
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Treble Amp: "));
    display.setCursor(75, 24);
    display.print(trebleamp);
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println(F("Bass Frequency: "));
    display.setCursor(98, 33);
    display.print(bassfreq);
    display.fillCircle(4, 45, 2, WHITE);
    display.setCursor(10, 42);
    display.println(F("Treble Frequency: "));
    display.setCursor(110, 42);
    display.print(treblefreq);
    display.drawCircle(4, 54, 2, WHITE);
    display.setCursor(10, 51);
    display.println(F("Volume: "));
    display.setCursor(53, 51);
    display.print(rightchannelvol);
    display.display();
  } else if (eqmenuindex == 4) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Equalizer"));
    display.drawCircle(4, 18, 2, WHITE);
    display.setCursor(10, 15);
    display.println(F("Bass Amp: "));
    display.setCursor(62, 15);
    display.print(bassamp);
    display.drawCircle(4, 27, 2, WHITE);
    display.setCursor(10, 24);
    display.println(F("Treble Amp: "));
    display.setCursor(75, 24);
    display.print(trebleamp);
    display.drawCircle(4, 36, 2, WHITE);
    display.setCursor(10, 33);
    display.println(F("Bass Frequency: "));
    display.setCursor(98, 33);
    display.print(bassfreq);
    display.drawCircle(4, 45, 2, WHITE);
    display.setCursor(10, 42);
    display.println(F("Treble Frequency: "));
    display.setCursor(110, 42);
    display.print(treblefreq);
    display.fillCircle(4, 54, 2, WHITE);
    display.setCursor(10, 51);
    display.println(F("Volume: "));
    display.setCursor(53, 51);
    display.print(rightchannelvol);
    display.display();
  }
  equalizerfunc();
}

void loop() {

  int currentselectstate = digitalRead(selectbutton);
  int currentbackstate = digitalRead(backbutton);
  int currentdownstate = digitalRead(downbutton);
  int currentpausestate = digitalRead(pausebutton);

  powermanager();

  switch (playerstate) {

  case home:
    homescreen();
    if (lastdownstate == LOW && currentdownstate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay)) {
      menucounter = !menucounter;
      break;
    }
    if (lastselectstate == LOW && currentselectstate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay) && menucounter == 0) {
      populatetracklist();
      playerstate = tracks;
      lastbuttontime = millis();
      break;
    } else if (lastselectstate == LOW && currentselectstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay) &&
               menucounter == 1) {
      playerstate = settingsmenu;
      lastbuttontime = millis();
      break;
    } else {
      break;
    }

  case tracks:
    trackmenu();
    if (lastdownstate == LOW && currentdownstate == HIGH &&
        (millis() - lastbuttontime) > buttondelay) {
      trackselectindex++;
      if (trackselectindex >= tracksfound) {
        trackselectindex = 0;
        menustartindex = 0;
      } else if (trackselectindex >= (tracksperscreen + menustartindex)) {
        menustartindex++;
      }
      lastbuttontime = millis();
      break;
    }
    if (lastselectstate == LOW && currentselectstate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay)) {
      trackselectindex;
      trackselector();
      playerstate = song;
      PassedTime = 0;
      lastbuttontime = millis();
      break;
    } else if (lastbackstate == LOW && currentbackstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      playerstate = home;
      lastbuttontime = millis();
      break;
    } else {
      break;
    }

  case song:
    walkman.feedBuffer();
    timekeeper();
    if (playerstate == song && walkman.playingMusic == false) {
      trackselectindex++;
      PassedTime = 0;
      if (trackselectindex >= tracksfound) {
        trackselectindex = 0;
      }
      trackselector();
    } else if (lastpausestate == LOW && currentpausestate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      walkman.pausePlaying(true);
      playerstate = stopped;
      lastbuttontime = millis();
      break;
    } else if (lastbackstate == LOW && currentbackstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      walkman.pausePlaying(true);
      playerstate = tracks;
      lastbuttontime = millis();
      break;
    } else if (lastdownstate == LOW && currentdownstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      trackselectindex++;
      PassedTime = 0;
      trackselector();
      lastbuttontime = millis();
      break;
    } else if (lastselectstate == LOW && currentselectstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      trackselectindex--;
      PassedTime = 0;
      trackselector();
      lastbuttontime = millis();
      break;
    } else {
      break;
    }

  case stopped:
    paused();
    if (lastpausestate == LOW && currentpausestate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay)) {
      walkman.pausePlaying(false);
      playerstate = song;
      lastbuttontime = millis();
      break;
    } else if (lastbackstate == LOW && currentbackstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      walkman.pausePlaying(true);
      playerstate = tracks;
      lastbuttontime = millis();
      break;
    } else if (lastdownstate == LOW && currentdownstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      trackselectindex++;
      trackselector();
      playerstate = song;
      lastbuttontime = millis();
      break;
    } else {
      break;
    }

  case settingsmenu:
    playersettings();
    if (lastdownstate == LOW && currentdownstate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay)) {
      settingscounter++;
      if (settingscounter > 2) {
        settingscounter = 0;
      }
      lastbuttontime = millis();
      break;
    } else if (lastselectstate == LOW && currentselectstate == HIGH &&
               settingscounter == 2 &&
               ((millis() - lastbuttontime) > buttondelay)) {
      playerstate = equalizermenu;
      lastbuttontime = millis();
      break;
    } else if (lastbackstate == LOW && currentbackstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      playerstate = home;
      lastbuttontime = millis();
      break;
    } else {
      break;
    }

  case equalizermenu:
    equalizer();
    if (eqmenuindex >= 5) {
      eqmenuindex = 0;
      break;
    }

    if (lastdownstate == LOW && currentdownstate == HIGH &&
        ((millis() - lastbuttontime) > buttondelay)) {
      eqmenuindex++;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 0 && currentselectstate == HIGH &&
               lastselectstate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      bassamp++;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 0 && currentpausestate == HIGH &&
               lastpausestate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      bassamp--;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 1 && currentselectstate == HIGH &&
               lastselectstate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      trebleamp++;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 1 && currentpausestate == HIGH &&
               lastpausestate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      trebleamp--;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 2 && currentselectstate == HIGH &&
               lastselectstate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      bassfreq++;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 2 && currentpausestate == HIGH &&
               lastpausestate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      bassfreq--;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 3 && currentselectstate == HIGH &&
               lastselectstate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      treblefreq++;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 3 && currentpausestate == HIGH &&
               lastpausestate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      treblefreq--;
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 4 && currentselectstate == HIGH &&
               lastselectstate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      rightchannelvol -= 10;
      leftchannelvol -= 10;
      walkman.setVolume(leftchannelvol, rightchannelvol);
      lastbuttontime = millis();
      break;
    } else if (eqmenuindex == 4 && currentpausestate == HIGH &&
               lastpausestate == LOW &&
               ((millis() - lastbuttontime) > buttondelay)) {
      rightchannelvol += 10;
      leftchannelvol += 10;
      walkman.setVolume(leftchannelvol, rightchannelvol);
      lastbuttontime = millis();
      break;
    } else if (lastbackstate == LOW && currentbackstate == HIGH &&
               ((millis() - lastbuttontime) > buttondelay)) {
      playerstate = settingsmenu;
      break;
    } else {
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
