/*
    DCC_Throttle.ino
    R.A.Lincoln   August 2026

    Arduino Baguette ESP32 C3
    USB CDC on Boot   -> enabled
    CPU Freq          -> 160MHz
    Flash mode        -> QIO
    Flash size        -> 4MB
    JTAG adapter      -> Integrated USB JTAG (for debugging)

*/

#include <LovyanGFX.hpp>

#include "debug_log.h"

#include "Screen.h"

#include "Roster.h"
#include "Address.h"
#include "Drive.h"
#include "Terminal.h"
#include "Settings.h"

#include "Comms.h"
#include "DCC.h"

//  Pins
#define ENCODER_CLK   4
#define ENCODER_DT    3
#define ENCODER_SW    1
#define MODE_SW       0


// Hardware Input State
//
unsigned long lastEncoderTime = 0;
int lastSelectBtnState = HIGH;
int lastModeBtnState = HIGH;
unsigned long lastDebounceTime = 0;


// Global App State
//
enum appMode { MODE_DRIVE, MODE_ROSTER, MODE_ADDRESS, MODE_SETTINGS, MODE_TERMINAL
} currentMode;


//  Setup
//
void setup() {
  Serial.begin(115200);
  LOG("Starting");

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT,  INPUT_PULLUP);
  pinMode(ENCODER_SW,  INPUT_PULLUP);
  pinMode(MODE_SW,     INPUT_PULLUP);

  Terminal.log("Starting");
  scr.init();
  scr.lines(true);
  scr.cls(c64::Light_blue, c64::Blue);
  
//  scr.at(11,0, "12", c64::White, c64::Green);               // Track 1 & 2 power
//  scr.at(13,0, "P", c64::White, c64::Red);                  // Programming track
//  scr.at(14,0, {0x06}, c64::White, c64::Green);             // Wifi char

  LOG("Initialising modules");

  Terminal.init();                                          // First so I can log to it
  scr.status("\xdf" "WiFi");
  Comms.init();                                             // Reconnect to the CS
  scr.status("\xdf" "Roster");
  DCC.init();                                               // Start syncing loco information
  Drive.init();                                             // Load the last driven loco
  
  LOG("Switching to drive");
  currentMode = MODE_DRIVE;                                 // Start in drive mode
  Drive.switchTo();

}


//  Loop
//
void loop() {

  //  --- REGULAR UPDATES ---
  //
  Comms.tick();                                             // Let comms regularly check state
  DCC.tick();                                               // Monitor the dcc stream


  //  --- ROTARY ENCODER READ ---
  //
  static int lastClkState = HIGH;
  int dir = 0;
  int step = 0;
  int currentClkState = digitalRead(ENCODER_CLK);

  if (currentClkState != lastClkState) {                            // Clock pin change
    lastClkState = currentClkState;

    if (currentClkState == LOW) {                                   // Falling edge
      unsigned long now = millis();
      unsigned long timeDiff = now - lastEncoderTime;
      if (timeDiff > 12) {                                          // debounce delay
        lastEncoderTime = now;

        dir = (digitalRead(ENCODER_DT) == HIGH) ? 1 : -1;           // direction +-1
        step = dir;                                                 // step +-1
        if (timeDiff <= 35)      step *= 5;                         // step +-5
        else if (timeDiff <= 70) step *= 2;                         // step +-2
      }
    }
  }


  //  --- DELEGATE ROTARY EFFECT TO ACTIVE MODE ---
  //
  if (dir != 0) {                                                    // if it moved
    switch (currentMode) {
      case MODE_DRIVE:    Drive.handleEncoder(step); break;          // +-1..8 depending on turning speed
      case MODE_ADDRESS:  Address.handleEncoder(dir); break;         // +-1
      case MODE_ROSTER:   Roster.handleEncoder(dir); break;          // +-1
      case MODE_SETTINGS: Settings.handleEncoder(step); break;       // +-1..8
      case MODE_TERMINAL: Terminal.handleEncoder(dir); break;        // +-1
    }
  }


  //  --- DELEGATE ROTARY ENCODER BUTTON TO ACTIVE MODE ---
  //
  int selectBtnRead = digitalRead(ENCODER_SW);
  if (selectBtnRead == LOW && lastSelectBtnState == HIGH && (millis() - lastDebounceTime > 50)) {
    lastDebounceTime = millis();

    switch(currentMode) {                                 // delegate to the active mode
      case MODE_DRIVE:
        Drive.handleEncoderButton();
        break;

      case MODE_ROSTER:
        if(Roster.handleEncoderButton()) {                // True = roster item selected
          currentMode = MODE_DRIVE;                       // back to drive mode
          Drive.switchTo(Roster.getSelectedLocoId());
        };
        break;

      case MODE_ADDRESS:
        if(Address.handleEncoderButton()) {               // True = address is selected
          currentMode = MODE_DRIVE;                       // back to drive mode
          Drive.switchTo(Address.selectedAddress());
        }
        break;

      case MODE_SETTINGS:
        Settings.handleEncoderButton();
        break;

      case MODE_TERMINAL:
        Terminal.handleEncoderButton();
        break;

      }
  }
  lastSelectBtnState = selectBtnRead;


  //  --- MODE BUTTON ---
  //
  int modeBtnRead = digitalRead(MODE_SW);
  if (modeBtnRead == LOW && lastModeBtnState == HIGH && (millis() - lastDebounceTime > 50)) {
    lastDebounceTime = millis();

    switch(currentMode) {
      case MODE_DRIVE:
        currentMode = MODE_ROSTER;
        Roster.switchTo(Drive.getActiveLocoId());
        break;

      case MODE_ROSTER:
        currentMode = MODE_ADDRESS;
        Address.switchTo();
        break;

      case MODE_ADDRESS:
        currentMode = MODE_SETTINGS;
        Settings.switchTo();
        break;

      case MODE_SETTINGS:
        currentMode = MODE_TERMINAL;
        Terminal.switchTo();
        break;

      case MODE_TERMINAL:
        Terminal.switchAway();              // Terminal has async inputs so needs to know when its not displayed
        currentMode = MODE_DRIVE;
        Drive.switchTo();
        break;

      default:
        currentMode = MODE_DRIVE;
        Drive.switchTo();

    }

  }
  lastModeBtnState = modeBtnRead;


  delay(1);
}
