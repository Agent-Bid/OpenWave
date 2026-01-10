#include <Adafruit_SSD1306.h>
#include <Adafruit_VS1053.h>
#include <PowerManagement.h>
#include <time.h>

// Function for power management and system sleep.

extern Adafruit_SSD1306 display;
extern Adafruit_VS1053_FilePlayer walkman;
extern int pausebutton;
extern int selectbutton;
extern int downbutton;
extern int backbutton;
boolean manageractive = false;
boolean isdrawn = false;

unsigned long lastactivitytime = 0;
unsigned long sleeptime = 10000;

void initpowermanager() {
  lastactivitytime = millis();
  manageractive = true;
}

void sleeptimereset() {
  if (digitalRead(pausebutton) == HIGH || digitalRead(selectbutton) == HIGH ||
      digitalRead(downbutton) == HIGH || digitalRead(backbutton) == HIGH) {
    lastactivitytime = millis();
    display.dim(false);
    isdrawn = false;
  }
}

void sleeping() {
  if ((millis() - lastactivitytime) >= sleeptime &&
      walkman.playingMusic == false) {
    if (isdrawn == false) {
      display.setCursor(25, 25);
      display.setTextSize(2);
      display.print(F("Sleeping"));
      display.display();
      isdrawn = true;
    }
    display.dim(true);
  } else if ((millis() - lastactivitytime) >= sleeptime &&
             walkman.playingMusic == true) {
    if (isdrawn == false) {
      display.setCursor(25, 25);
      display.setTextSize(2);
      display.print(F("Sleeping"));
      display.display();
      isdrawn = true;
    }
    display.dim(true);
  }
}

/*void wakefromsleep() {
  if ((millis() - lastactivitytime) >= sleeptime &&
          (digitalRead(pausebutton) == HIGH ||
      digitalRead(selectbutton) == HIGH || digitalRead(downbutton) == HIGH ||
      digitalRead(backbutton) == HIGH)) {
    lastactivitytime = millis();
    display.dim(false);
  }
}*/

void powermanager() {
  if (manageractive == false) {
    initpowermanager();
    sleeptimereset();
    sleeping();
    wakefromsleep();
  } else if (manageractive == true) {
    sleeptimereset();
    sleeping();
    wakefromsleep();
  }
}
