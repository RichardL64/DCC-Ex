/*
    DCCLocoCache.h

    R.A.Lincoln         2026

    Cache for incoming roster or speed information
    Keyed on loco, populated in any order depending what is broadcast by the CS

    Data is re-formatted inbound into useful values - % speed, 3 line loco name etc

*/
#pragma once

#define MAX_LOCO_SLOTS  10
#define NAME_SIZE       64
#define LINE_SIZE       13


struct LocoInfo {
  // --- Static Metadata (Roster) ---
  uint16_t locoId = 0;
  char name[NAME_SIZE] = {0};   // Raw name in DCC-ex
  char lines[3][LINE_SIZE+1];   // Formatted onto 3 lines to display

  // --- Dynamic Telemetry (Cache) ---
  int rawSpeed = 0;             // 0-127 or -1 from DCC-Ex
  int percentSpeed = 0;         // Calculated on load from rawSpeed
  bool forward = true;          //    "

  // --- Slot Management ---
  bool active = false;        // Slot in use
};


class LocoCacheClass {

private:
  LocoInfo slots[MAX_LOCO_SLOTS];
  unsigned long lastRosterMillis;

  // Single lookup helper for both metadata and telemetry
  //
  LocoInfo* find(uint16_t locoId) {
    for (auto& slot : slots) {
      if (slot.active && slot.locoId == locoId) return &slot;
    }
    return nullptr;
  }


  //  Find a free slot, if they're all full use the first one
  //
  LocoInfo* findFree() {
    for (auto& slot : slots) {
      if (!slot.active) return &slot;
    }
    return &slots[0];                          // All slots occupied, keep using the first one
  }


  //  Loco name formatting
  //

  /*
    , or / delimited list - change formatting depending on the number of fields found:

    Fields    Line 0      Line 1    Line 2
    1         id          f0
    2         id          f1        f2
    3         id  f1      f2        f3
    4         id  f1      f2  f3    f4
  */

  //  Overlay the strings on the left and right
  //
  void formatJustified(char* dest, const char* left, const char* right) {
    int spaces = LINE_SIZE - strlen(left) - strlen(right);
    snprintf(dest, LINE_SIZE +1, "%s%*s%s", left, spaces > 0 ? spaces : 1, "", right);
  }

  // Overlay string on the left end and pad out with spaces
  //
  void formatLeft(char* dest, const char* left) {
    snprintf(dest, LINE_SIZE +1, "%-*.*s", LINE_SIZE, LINE_SIZE, left);
  }

  // Overlay string on the right end - assumes pre-filled with spaces
  //
  void formatRight(char* dest, const char* right) {
    if (!right) return;
    int len = strlen(right);
    if (len <= LINE_SIZE) {
      strcpy(dest + LINE_SIZE - len, right);
    }
  }

  // Format raw roster entry into 3 lines
  // Comma or / delimited
  //
  void parseName(LocoInfo* info) {
    auto lines = info->lines;

    char rawData[64] = {0};                                           // local scratch to modify
    strncpy(rawData, info->name, sizeof(rawData) -1);

    char* fields[4] = {nullptr};                                      // Up to 4 fields
    int count = 0;

    char* ptr = strtok(rawData, "/\n\r,");                            // find the commas or slashes, note modifies the string
    while (ptr != nullptr && count < 4) {
      fields[count++] = ptr;
      ptr = strtok(nullptr, "/\n\r,");
    }

    snprintf(lines[0], LINE_SIZE +1, "#%-*.4d", LINE_SIZE -1, info->locoId);   // Id always top left
    formatLeft(lines[1], "");                                                  // everything else padded empty
    formatLeft(lines[2], "");

    switch (count) {
        case 1:
                                                                      // ID
          formatLeft(lines[1], fields[0]);                            // f0
          break;

        case 2:
          formatRight(lines[0], fields[0]);                           // ID ... f0
          formatLeft(lines[1], fields[1]);                            // f1
          break;

        case 3:
          formatRight(lines[0], fields[0]);                           // ID ... f0
          formatLeft(lines[1], fields[1]);                            // f1
          formatLeft(lines[2], fields[2]);                            // f2
          break;

        case 4:                                                       // 4 or more
        default:                                                      
          formatRight(lines[0], fields[0]);                           // ID ... f0
          formatJustified(lines[1], fields[1], fields[2]);            // f1 ... f2
          formatRight(lines[2], fields[3]);                           // f3
          break;
      }
  }


public:


  // Seeding path 1: Roster packet arrives (<i ...> or NVS load)
  // Format the name ready for display over 3 lines
  //
  void updateInfo(uint16_t locoId, const char* name) {
    LocoInfo* slot = getSlotByLocoId(locoId);
    snprintf(slot->name, sizeof(slot->name), "%s", name);
    parseName(slot);
    lastRosterMillis = millis();                      // Used by DCC, to wait for roster responses
  }

  //  Used by DCCWaitForRoster to work out if we stopped receiving roster entries
  //
  unsigned long getLastRosterMillis() {
    return lastRosterMillis;
  }

  //  Seeding path 2: Live telemetry arrives (<l ...>)
  //  Decode speed to something more useful on the fly
  //
  void updateTelemetry(uint16_t locoId, int rawSpeed) {
    LocoInfo* slot = getSlotByLocoId(locoId);
    if (slot->rawSpeed != rawSpeed) {
      slot->rawSpeed = rawSpeed;

      // Speed coding:
      //  reverse - 2-127 = speed 1-126, 0 = stop, 1 = Emergency Stop
      //  forward - 130-255 = speed 1-126, 128 = stop, 129 = Emergency Stop
      slot->forward = (rawSpeed >= 128);                // >= 128 is Forward; < 128 is Reverse
      int step = rawSpeed & 0x7F;                       // Extract low 7 bits (step 0..127)
      int speed = (step <= 1) ? 0 : (step - 1);         // Steps 0 (Stop) and 1 (E-Stop) map to 0. Steps 2..127 map to 1..126.
      slot->percentSpeed = (speed *100) /126;           // Percentage speed (0-100%)
    }
  }


  // Find existing slot or claim & initialise an unused one
  //
  LocoInfo* getSlotByLocoId(uint16_t locoId) {
    LocoInfo* slot = find(locoId);
    if (slot) return slot;                    // --> found it

    slot = findFree();                        // create a new entry in a free slot
    slot->locoId = locoId;
    snprintf(slot->name, sizeof(slot->name), "#%04u/Temporary", locoId);  // Fallback label until a roster return message
    parseName(slot);
    slot->active = true;

    return slot;
  }


  // Read-only accessors for roster mode screen rendering
  // Cache implemented as a potentially sparse list so need to count active entries
  //
  int getSlotCount() {
    int count = 0;
    for (auto& slot : slots) { if (slot.active) count++; }
    return count;
  }

  LocoInfo* getSlotByIndex(int index) {
    int current = 0;
    for (auto& slot : slots) {
      if (slot.active) {                                  // Only active slots
        if (current == index) return &slot;
        current++;
      }
    }
    return nullptr;
  }

} inline LocoCache;
