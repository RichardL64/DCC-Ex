/*
    DCC.h
    R.A.Lincoln       2026

    DCC-EX Protocol Parser, Stream Dispatcher, and Command Gateway
*/

#pragma once
#include <Arduino.h>
#include <WiFi.h>

#include "Comms.h"
#include "DCCLocoCache.h"


// --- Core DCC Control Gateway -----------------------------------------------
//
class dccClass {

private:

  //  Speed outbound change detection
  //
  int setLoco = 0;
  int setSpeed = 0;
  int setForward = 0;

  int sentLoco = 0;
  int sentSpeed = 0;
  int sentForward = 0;

  //  DCC message inbound frames
  //
  char rxBuf[128];
  size_t rxIdx = 0;
  bool mainPowerState = false;

 //  Parse inbound messages, fields delimited with space
  //
  //  Loco info     <l cab reg speedByte functMap>
  //  Roster index  <jR id1 id2 id3...>
  //  Roster info   <jR id "desc" "funct1/funct2/funct3/...">
  //
  void parseFrame(char* frame) {

    if (!frame || frame[0] != '<') return;                   // -->

    //  Split out the frame by space delimiters
    //
    char buffer[128];                                        // Local copy for strtok to modify
    strncpy(buffer, frame, sizeof(buffer)-1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* tokens[MAX_LOCO_SLOTS] = {nullptr};                // Largest num of parms is a list of loco id's
    int count = 0;
    char* ptr = strtok(buffer, " ");                         // Space delimited data
    while (ptr && count < MAX_LOCO_SLOTS) {
      tokens[count++] = ptr;
      ptr = strtok(nullptr, " ");
    }
    if(count == 0) return;                                  // --> no space delimiters

    //  Interpret the tokens
    //
    switch(tokens[0][1]) {                                  // first char after the <

    // Power status broadcast (<p 1 MAIN> / <p 0>)
    //
    case 'p':                                               
    case 'P':
        mainPowerState = (frame[3] == '1');
        break;

    // Loco info      <l cab reg speedByte functMap>
    //
    case 'l': 
      if(count != 5) return;                                // -->
      LocoCache.updateTelemetry(atoi(tokens[1]), atoi(tokens[3]));
      break;

    // Roster <jR
    //
    case 'j':
      // Roster info  <jR id "des c" "funct1/funct2/funct3/...">
      // Inside the quotes could be spaces so split across tokens
      //
      if(count > 2 && tokens[2][0] == '"') {
        uint16_t address = (uint16_t)atoi(tokens[1]);

        char* startQuote = frame + (tokens[2] - buffer);  // Directly points to first '"' in frame
        char* endQuote   = strchr(startQuote + 1, '"');   // Search forward for second '"'

        if (endQuote) {
          *endQuote = '\0';                               // Overwrite closing quote in frame
          char* name = startQuote + 1;                    // Direct pointer to "BR Class A3 60084 Trigo"

          LocoCache.updateInfo(address, name);
        }

      // Roster index <jR id1 id2 id3...>
      // Generate requests for loco roster and speed information
      //
      } else {
        for(int i = 1; i < count; i++) {
          uint16_t address = (uint16_t)atoi(tokens[i]);
          requestRoster(address);
          requestLocoStatus(address);
        }
        
      }

    default:
      return;                                               // --> dont know what it is
    }
  }


  //  Wait for the roster to populate and comms traffic slow down
  //
  void waitForRoster() {
      unsigned long start = millis();

      const unsigned long maxTimeout = 3000;        // Hard safety net (3 seconds max)
      const unsigned long silenceDelay = 500;       // Wait 500ms after the last packet

      // Keep track of the last known timestamp from the cache

      while (millis() - start < maxTimeout) {
          Comms.tick();                             // Keep wifi up
          tick();                                   // Keep processing inbound messages

          // If we've received at least something, and it's been quiet for 500ms, we're done
          unsigned long lastReceipt = LocoCache.getLastRosterMillis();
          if (lastReceipt > 0 && (millis() - lastReceipt > silenceDelay)) {
              break;                                // ->
          }

          yield();                                  // Keep ESP32 watchdog happy
      }
  }


public:

  //  At startup - kick off the roster request to populate the locoCache
  //  Broadcast messages will keep it up to date moving forward
  //
  void init() {
    requestRoster();
    waitForRoster();
  }


  //  Outbound Commands
  //
  void setTrackPower(char track, bool enable) {         // Track A/B enable
    if (!Comms.dccConnected()) return;                  // -->
    if (track) {
      Comms.dccPrintf("<%c %c>\n", enable ? '1' : '0', track);
    } else {
      Comms.dccPrintf("<%c>\n", enable ? '1' : '0');
    }
  }

  void setProgTrack(char track, bool enable) {          // Track A/B programming track
    if (!Comms.dccConnected()) return;                  // -->
    Comms.dccPrintf("<=%c %s>\n", track, enable ? "PROG" : "MAIN");
  }

  //  Speed requests are cached and sent periodically by tick()
  //  Avoid saturating the Wifi stream
  //
  int percentToSpeed(int percent) {
    if (percent <= 0) return 0;       // Stop
    if (percent >= 100) return 126;   // Max safe speed step (avoid 127)
    
    return (percent * 125) / 99;      // Map 1-99% to speed steps 1-125
  }

  void sendSpeed(uint16_t locoId, int percentSpeed, bool isForward) {
    setLoco = locoId;
    setSpeed = percentToSpeed(percentSpeed);
    setForward = (isForward ? 1 : 0);
  }

  void eStop(uint16_t locoId) {                         // Single loco emergency stop
    setLoco  = locoId;
    setSpeed = -1;                                      // Native DCC-EX E-Stop
  }

  void eStop() {                                        // All locos emergency stop
    if (!Comms.dccConnected()) return;                  // -->
    Comms.dccPrintf("<!>");
  }

  //  Prompt for status
  //
  void requestRoster() {
    if (!Comms.dccConnected()) return;                  // -->

    Comms.dccPrintf("<JR>");
  }

  void requestRoster(int locoId) {
    if (!Comms.dccConnected()) return;                  // -->
    Comms.dccPrintf("<JR %d>\n", locoId);
  }

  void requestLocoStatus(uint16_t locoId) {
    if (!Comms.dccConnected()) return;                  // -->
    Comms.dccPrintf("<t %d>\n", locoId);
  }

  bool isPowerOn() const { return mainPowerState; }


  //  Continuous updates
  //  Non-blocking stream reader. 
  //
  void tick() {
    if (!Comms.dccConnected()) return;                          // -->

    //  Non-blocking stream reader. 
    //
    while (Comms.dccAvailable()) {
      char c = Comms.dccRead();
      if (c == '<') {                                           // start <
        rxIdx = 0;
        rxBuf[rxIdx++] = c;

      } else if (c == '>') {                                    // end >
        if (rxIdx < sizeof(rxBuf) - 1) {
          rxBuf[rxIdx++] = c;
          rxBuf[rxIdx]   = '\0';

          Terminal.log(rxBuf, false);                           // Log inbound on terminal
          parseFrame(rxBuf);                                    // Parse it
        }
        rxIdx = 0;

      } else if (rxIdx > 0 && rxIdx < sizeof(rxBuf) - 1) {      // store char
        rxBuf[rxIdx++] = c;

      }
    }

    //  Periodic speed control sender to avoid flooding the connection
    //
    static unsigned long lastSpeed = 0;
    if (millis() - lastSpeed < 100) return;              // --> Control call rate
    lastSpeed = millis();

    if(setLoco != sentLoco
      || setSpeed != sentSpeed
      || setForward != sentForward) {                    // different from last sent?

        // DCC-EX speed command format: <t cab speed direction>
        Comms.dccPrintf("<t %d %d %d>\n", setLoco, setSpeed, setForward);
        sentLoco = setLoco;
        sentSpeed = setSpeed;
        sentForward = setForward;
        LOG("Sent speed %d %d", sentLoco, sentSpeed);
    }

  }
} inline DCC;
