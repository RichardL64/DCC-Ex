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
  int sentSpeed = -1;                                           // Force very first send
  int sentForward = 0;


  //  DCC message cache
  //
  LocoCacheClass locoCache;


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
    LOG("Parse frame <%s>", frame);

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

    LOG("Token count %d", count);

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
      LOG("<l...");
      locoCache.updateTelemetry(atoi(tokens[1]), atoi(tokens[3]));
      break;

    // Roster <jR
    //
    case 'j':
      // Roster info  <jR id "des c" "funct1/funct2/funct3/...">
      // Inside the quotes could be spaces so split across tokens
      //
      if(count > 2 && tokens[2][0] == '"') {
        LOG("<jR detail");
        uint16_t address = (uint16_t)atoi(tokens[1]);

        char* startQuote = frame + (tokens[2] - buffer);  // Directly points to first '"' in frame
        char* endQuote   = strchr(startQuote + 1, '"');   // Search forward for second '"'

        if (endQuote) {
          *endQuote = '\0';                               // Overwrite closing quote in frame
          char* name = startQuote + 1;                    // Direct pointer to "BR Class A3 60084 Trigo"

          LocoCache.updateRosterInfo(address, name);
        }

      // Roster index <jR id1 id2 id3...>
      //
      } else {
        LOG("<jR index");
        for(int i = 1; i < count; i++) {
          uint16_t address = (uint16_t)atoi(tokens[i]);
          Comms.dcc().printf("<JR %d>\n", address);         // Send requests for loco info
        }
        
      }

    default:
      return;                                               // --> dont know what it is
    }
  }

public:


  //  Continuous updates
  //  Non-blocking stream reader. 
  //
  void tick() {
    if (!Comms.dcc().connected()) return;               // -->

    //  Non-blocking stream reader. 
    //
    while (Comms.dcc().available()) {
      char c = Comms.dcc().read();
      if (c == '<') {                                           // start <
        rxIdx = 0;
        rxBuf[rxIdx++] = c;

      } else if (c == '>') {                                    // end >
        if (rxIdx < sizeof(rxBuf) - 1) {
          rxBuf[rxIdx++] = c;
          rxBuf[rxIdx]   = '\0';
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
        Comms.dcc().printf("<t %d %d %d>\n", setLoco, setSpeed, setForward);
        sentLoco = setLoco;
        sentSpeed = setSpeed;
        sentForward = setForward;
        LOG("Sent speed %d %d", sentLoco, sentSpeed);
    }

  }


  //  Outbound Commands
  //
  void setTrackPower(char track, bool enable) {         // Track A/B enable
    if (!Comms.dcc().connected()) return;               // -->
    if (track) {
      Comms.dcc().printf("<%c %c>\n", enable ? '1' : '0', track);
    } else {
      Comms.dcc().printf("<%c>\n", enable ? '1' : '0');
    }
  }

  void setProgTrack(char track, bool enable) {          // Track A/B programming track
    if (!Comms.dcc().connected()) return;               // -->
    Comms.dcc().printf("<=%c %s>\n", track, enable ? "PROG" : "MAIN");
  }

  //  Speed requests are cached and sent periodically by tick()
  //  Avoid saturating the Wifi stream
  //
  void sendSpeed(uint16_t locoId, int percentSpeed, bool isForward) {
    setLoco = locoId;
    setSpeed = map(constrain(percentSpeed, 0, 100), 0, 100, 0, 127);
    setForward = (isForward ? 1 : 0);
  }

  void eStop(uint16_t locoId) {                         // Single loco emergency stop
    setLoco  = locoId;
    setSpeed = -1;                                      // Native DCC-EX E-Stop
  }

  void eStop() {                                        // All locos emergency stop
    if (!Comms.dcc().connected()) return;               // -->
    Comms.dcc().println("<!>");
  }

  //  Prompt for status
  //
  void requestLocoStatus(uint16_t locoId) {
    if (!Comms.dcc().connected()) return;               // -->
    Comms.dcc().printf("<t %d>\n", locoId);
  }

  void requestRoster() {
    if (!Comms.dcc().connected()) return;               // -->
    Comms.dcc().println("<JR>");
  }

  void requestRoster(int locoId) {
    if (!Comms.dcc().connected()) return;               // -->
    Comms.dcc().printf("<JR %d>\n", locoId);
  }

  bool isPowerOn() const { return mainPowerState; }

} inline DCC;





/*
#pragma once
#include <Arduino.h>

//
//    DCC Loco info
//

struct LocoInfo {
  uint16_t address   = 0;
  int      speed     = 0;       // 0-126 or 0-100
  bool     isForward = true;
  uint32_t functions = 0;       // Bitmask for F0-F28
  uint32_t lastUpdate = 0;       // millis() timestamp
  bool     valid     = false;
};

class LocoCacheClass {
private:
  static constexpr size_t MAX_CACHED_LOCOS = 8;
  LocoInfo cache[MAX_CACHED_LOCOS];

  // Helper to find existing slot or return a free slot
  LocoInfo* getSlot(uint16_t address) {
    for (size_t i = 0; i < MAX_CACHED_LOCOS; i++) {
      if (cache[i].valid && cache[i].address == address) return &cache[i];
    }
    for (size_t i = 0; i < MAX_CACHED_LOCOS; i++) {
      if (!cache[i].valid) return &cache[i];
    }
    return &cache[0]; // Fallback LRU / overwrite index 0
  }

public:
  // Returns pointer to cached info if present, or nullptr
  LocoInfo* get(uint16_t address) {
    for (size_t i = 0; i < MAX_CACHED_LOCOS; i++) {
      if (cache[i].valid && cache[i].address == address) return &cache[i];
    }
    return nullptr;
  }

  //  parse <l loco information messages  
  //
  void parseFrame(char* frame) {
    if (frame[0] != '<' || frame[1] != 'l') return;         // -->

    char buffer[64];                                        // Local copy for strtok to modify
    strncpy(buffer, frame + 1, sizeof(buffer) - 1);
    char* end = strchr(buffer, '>');
    if (end) *end = '\0';

    char* tokens[8] = {nullptr};
    int count = 0;
    char* ptr = strtok(buffer, " ");                        // Space delimited data
    while (ptr && count < 8) {
      tokens[count++] = ptr;
      ptr = strtok(nullptr, " ");
    }

    if (count < 4) return;                                  // -->

    uint16_t address = atoi(tokens[1]);
    LocoInfo* loco   = getSlot(address);

    int rawSpeed     = atoi(tokens[3]);
    bool isFwd       = (count > 4) ? (atoi(tokens[4]) != 0) : true;

    // Map DCC step (0-127) to 0-100%, handling emergency stop step (1)
    int magnitude    = (rawSpeed <= 1) ? 0 : ((rawSpeed * 100) / 126);
    if (magnitude > 100) magnitude = 100;

    loco->address    = address;
    loco->speed      = rawSpeed;
    loco->isForward  = isFwd;
    loco->speedPerc  = isFwd ? magnitude : -magnitude; // Signed percentage
    loco->lastUpdate = millis();
    loco->valid      = true;
  }

  // Call this continuously in loop() or during wait to process incoming serial/TCP data
  //
  void processDCCInput() {
    static char rxBuf[64];
    static size_t rxIdx = 0;

    while (dccClient.available()) {
      char c = dccClient.read();
      if (c == '<') {
        rxIdx = 0;
        rxBuf[rxIdx++] = c;
      } else if (c == '>') {
        if (rxIdx < sizeof(rxBuf) - 1) {
          rxBuf[rxIdx++] = c;
          rxBuf[rxIdx]   = '\0';
          LocoCache.parseFrame(rxBuf);                        // Feed to cache
        }
        rxIdx = 0;
      } else if (rxIdx > 0 && rxIdx < sizeof(rxBuf) - 1) {
        rxBuf[rxIdx++] = c;
      }
    }
  }

  // Triggers <t cab> and blocks until response is cached or timeout expires
  //
  bool requestLocoStatus(uint16_t locoId, uint32_t timeoutMs = 500) {
    if (!dccClient.connected()) return false;

    // Clear existing cache validity so we wait for a fresh response
    LocoInfo* info = LocoCache.get(locoId);
    if (info) info->valid = false;

    // Send request command: <t locoId>
    dccClient.printf("<t %d>\n", locoId);

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
      processDCCInput();                                      // Keep reading stream

      info = LocoCache.get(locoId);
      if (info && info->valid) {
        return true;                                          // Received fresh broadcast!
      }
      yield();                                                // Prevent watchdog trigger on ESP32
    }

    return false;                                             // Timed out
  }

} inline LocoCache;






//  for drive.h
/*
void switchTo(int locoId) {
  activeLocoId = locoId;
  saveLastLocoID(activeLocoId);

  if (requestLocoStatus(activeLocoId)) {
    LocoInfo* info = LocoCache.get(activeLocoId);
    
    isForward      = info->speedPerc >= 0;
    targetThrottle = abs(info->speedPerc);
  } else {
    targetThrottle = 0;
    isForward      = true;
  }

  drawUI();
}



//
//    DCC ROSTER
//

class RosterCacheClass {
private:

// Send roster request command to DCC-EX Command Station
  //
  void requestRoster() {
    if (!dccClient.connected()) return;
    dccClient.println("<JR>");
  }

  // Parse incoming DCC-EX frame (e.g., `<jR 60084 "60084/4-6-2/A3/Trigo">`)
  //
  bool parseFrame(char* frame) {
    if (frame[0] != '<' || (frame[1] != 'j' && frame[1] != 'J') || toupper(frame[2]) != 'R') {
      return false;
    }

    // Extract payload between '<' and '>'
    char buffer[128];
    strncpy(buffer, frame + 1, sizeof(buffer) - 1);
    char* end = strchr(buffer, '>');
    if (end) *end = '\0';

    // Skip "jR " prefix
    char* ptr = buffer + 2;
    while (*ptr == ' ') ptr++;

    // Parse locoId
    uint16_t locoId = atoi(ptr);
    if (locoId == 0) return false;

    // Move past ID to the description payload
    while (*ptr && *ptr != ' ') ptr++;
    while (*ptr == ' ') ptr++;

    // Strip quotes if payload is enclosed in "..."
    if (*ptr == '"') ptr++;
    char* quoteEnd = strrchr(ptr, '"');
    if (quoteEnd) *quoteEnd = '\0';

    // Store and format
    int index = findOrAddSlot(locoId);
    entries[index].locoId = locoId;
    entries[index].valid  = true;
    parseName(index, ptr);

    return true;
  }
} inline RosterCache;
*/
