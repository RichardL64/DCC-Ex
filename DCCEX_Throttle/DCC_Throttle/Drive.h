/*
    Drive.h
    R.A.Lincoln       2026

    Drive the selected loco
    
*/

#pragma once
#include <Preferences.h>
#include "DCC.h"
#include "DCCLocoCache.h"
#include "Roster.h"


class driveClass {

private:
  Preferences Prefs;                                  // Non volatile storage

  int  activeLocoId;
  int  targetThrottle;
  bool isForward;


  // Save the active locomotive address to flash NVS
  //
  void saveLastLocoId(uint16_t locoId) {
    Prefs.begin("drive_cfg", false);                         // Namespace: drive_cfg
    Prefs.putUShort("last_loco", locoId);
    Prefs.end();
  }

  // Load the active locomotive address on boot (returns 0 if none saved)
  //
  uint16_t loadLastLocoId() {
    Prefs.begin("drive_cfg", true);                        // Read-only
    uint16_t locoId = Prefs.getUShort("last_loco", 3);     // Default to DCC Address 3 if empty
    Prefs.end();
    return locoId;
  }


  //  Initial UI
  //
  void drawUI() {
    char buf[16];

    scr.fb(c64::Light_blue, c64::Blue);
    scr.cls("Drive");

    //  DCC id
    snprintf(buf, sizeof(buf), "#%04d", activeLocoId);
    scr.at(1, 1, "\xe7\xe7\xe7\xe7\xe7", c64::Blue, c64::White);
    scr.at(1, 2, buf, c64::Blue, c64::White);

    //  Loco description
    LocoInfo* info = LocoCache.getSlotByLocoId(activeLocoId);
    scr.fg(c64::White);
    scr.at(1, 4, info->lines[0]);                    // roster formats the loco info correctly
    scr.at(1, 5, info->lines[1]);
    scr.at(1, 6, info->lines[2]);

    //  Speed %
    updateUISpeed();
    updateUIBar();


    //  Speed bar
    scr.fg(c64::Black);
    scr.at(0, 13, "\x98\xcb\xcb\xcb\xcb\xcb\xcb\x99\xcb\xcb\xcb\xcb\xcb\xcb\x9a}");
    scr.at(0, 14, "\xc4\x20\x20\x20\x20\x20\x20\xc4\x20\x20\x20\x20\x20\x20\xc4}");
    scr.at(0, 15, "\xc4\x20\x20\x20\x20\x20\x20\xc4\x20\x20\x20\x20\x20\x20\xc4}");
    scr.at(0, 16, "\xb8\xcb\xcb\xcb\xcb\xcb\xcb\xb9\xcb\xcb\xcb\xcb\xcb\xcb\xba}");

    scr.fg(c64::Light_blue);
  }


  //  Update speed components
  //
  void updateUISpeed() {
    char buf[4];
    snprintf(buf, sizeof(buf), "%02d", (targetThrottle > 99) ? 99 : targetThrottle);        // limit to 99% even when 100% for layout reasons

    c64 c = isForward ? c64::Green : c64::Yellow;
    scr.at4x4(3, 8, buf[0], c);
    scr.at4x4(7, 8, buf[1], c);
    scr.at(12, 8, "%", c);
  }

  //  Stop - $c4
  //  Forward - 4 pixels $db, 6 full chars $ff
  //  Reverse - 5 pixels $d5, 6 full chars $ff
  void updateUIBar() {
    char barBuffer[14];
    memset(barBuffer, 0xf0, 13);          // Init to empty string
    barBuffer[13] = '\0';

    
    //  No throttle
    if(targetThrottle == 0) {             // Zero - just draw the vertical black bar
        barBuffer[6] = '\xc4';            // Stop
        scr.at(1, 14, barBuffer, c64::Black);     // Draw empty
        scr.at(1, 15, barBuffer, c64::Black);
        return;                           // -->
    }

    //  6 chars left or right, minus the 4/5 fixed central pixels
    //  Its actually 5 pixels fixed left - will assume 4 for both directions
    int pixels = map(targetThrottle, 0, 100, 0, 6*8);
    int fullBlocks = pixels / 8;
    int partBlocks = pixels % 8;

    //  Forward centre to the right
    if(isForward) {
      barBuffer[6] = '\xdb';                      // Forward (4 pixels)
      memset(&barBuffer[7], '\xff', fullBlocks);

      if(partBlocks) {
        char partChar = 0xd0 + partBlocks;        // $d1-d7 = fill cols 1-7 left to right
        barBuffer[fullBlocks +7] = partChar;      // On the right hand end
      }

    //  Revese centre to the left
    } else {
      barBuffer[6] = '\xd5';                      // Reverse (5 pixels)
      memset(&barBuffer[6 -fullBlocks], '\xff', fullBlocks);

      if(partBlocks) {
        char partChar = 0xD8 +7 -partBlocks;      // $de-d8 = fill cols 1-7 right to left
        barBuffer[6 -fullBlocks -1] = partChar;   // On the left hand end
      }
    }

    c64 c = isForward ? c64::Green : c64::Yellow;
    scr.at(1, 14, barBuffer, c);                  // Draw it
    scr.at(1, 15, barBuffer, c);
  }

public:


  // Called during initial setup
  //
  void init() {
    activeLocoId = loadLastLocoId();            // Loco from last session
  }


  //  Switch to a specific loco
  //
  void switchTo(int locoId) {
    if(locoId != activeLocoId) {                // ? changed loco
      activeLocoId = locoId;
      saveLastLocoId(activeLocoId);             // Save for the next session
    }
    switchTo();                                 // Sync up with cached telemetry
  }

  //  Switch to the current/active loco
  //  Sync telemetry from the Loco cache
  //
  void switchTo() {
    LocoInfo* info = LocoCache.getSlotByLocoId(activeLocoId);      
    isForward      = info->forward;
    targetThrottle = info->percentSpeed;

    drawUI();
  }


  //  Rotary encoder, new requested speed
  //
  void handleEncoder(int step) {
    targetThrottle += isForward ? step : -step;    
    if (targetThrottle > 100) targetThrottle = 100;
    if (targetThrottle < 0)   targetThrottle = 0;

    updateUISpeed();                                              // Draw it
    updateUIBar();
    DCC.sendSpeed(activeLocoId, targetThrottle, isForward);       // Send it
  }


  //  Encoder button, switch direction
  //
  bool handleEncoderButton() {
    targetThrottle = 0;
    isForward = !isForward;

    updateUISpeed();                                              // Draw it
    updateUIBar();
    DCC.sendSpeed(activeLocoId, targetThrottle, isForward);       // Send it
    return false;                                                 // stay on this screen
  } 

  //  Return the loco id currently being driven
  //  Useful to seed the roster page
  //
  int getActiveLocoId() {
    return activeLocoId;
  }


} inline Drive;
