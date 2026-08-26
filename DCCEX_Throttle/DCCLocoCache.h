/*
    DCCLocoCache.h

    R.A.Lincoln         2026

    Cache for incoming roster or speed information
    Keyed on loco, populated in any order depending what is broadcast by the CS

*/
#define MAX_LOCO_SLOTS 10
#define NAME_SIZE      32

struct LocoSlot {
  // --- Static Metadata (Roster) ---
  uint16_t address = 0;
  char name[NAME_SIZE] = {0};

  // --- Dynamic Telemetry (Cache) ---
  int rawSpeed = 0;            // 0-127 or -1
  int percentSpeed = 0;
  bool forward = true;
  uint32_t functions = 0;   // Bitmask for F0-F28

  // --- Slot Management ---
  bool active = false;      // Slot in use
  bool dirty = false;       // Screen redraw required
};


class LocoCacheClass {

private:
  LocoSlot slots[MAX_LOCO_SLOTS];

  // Single lookup helper for both metadata and telemetry
  //
  LocoSlot* find(uint16_t address) {
    for (auto& slot : slots) {
      if (slot.active && slot.address == address) return &slot;
    }
    return nullptr;
  }

  //  Find a free slot, if they're all full use the first one
  //
  LocoSlot* findFree() {
    for (auto& slot : slots) {
      if (!slot.active) return &slot;
    }
    return &slots[0];                          // All slots occupied, keep using the first one
  }


public:

  // Find existing slot or claim an unused one
  //
  LocoSlot* getSlot(uint16_t address) {
    LocoSlot* slot = find(address);
    
    LOG("getSlot existing %d > %d", address, slot);
    if (slot) return slot;                    // --> found it

    slot = findFree();                        // create a new entry in a free slot
    slot->address = address;
    snprintf(slot->name, sizeof(slot->name), "#%04u", address);   // Fallback label until a roster return message
    slot->active = true;
    slot->dirty  = true;

    LOG("getSlot new %d > %d", address, slot);
    return slot;
  }

  // Seeding path 1: Roster packet arrives (<i ...> or NVS load)
  //
  void updateRosterInfo(uint16_t address, const char* name) {
    LocoSlot* slot = getSlot(address);
    if (strcmp(slot->name, name) != 0) {                      // if the inbound name is different, record it
      snprintf(slot->name, sizeof(slot->name), "%s", name);
      slot->dirty = true;
    }
  }

  // Seeding path 2: Live telemetry arrives (<l ...>)
  // Speed is encoded:
  //  reverse - 2-127 = speed 1-126, 0 = stop, 1 = Emergency Stop
  //  forward - 130-255 = speed 1-126, 128 = stop, 129 = Emergency Stop
  //
  void updateTelemetry(uint16_t address, int rawSpeed) {
    LocoSlot* slot = getSlot(address);
    if (slot->rawSpeed != rawSpeed) {
      slot->rawSpeed = rawSpeed;
      slot->forward = (rawSpeed >= 128);                // >= 128 is Forward; < 128 is Reverse
      int step = rawSpeed & 0x7F;                       // Extract low 7 bits (step 0..127)
      int speed = (step <= 1) ? 0 : (step - 1);         // Steps 0 (Stop) and 1 (E-Stop) map to 0. Steps 2..127 map to 1..126.
      slot->percentSpeed = (speed *100) /126;           // Percentage speed (0-100%)
      slot->dirty   = true;
    }
  }


  // Read-only accessors for roster mode screen rendering
  //
  int getSlotCount() const {
    int count = 0;
    for (const auto& slot : slots) { if (slot.active) count++; }
    return count;
  }

  const LocoSlot* getSlotByIndex(int index) const {
    int current = 0;
    for (const auto& slot : slots) {
      if (slot.active) {
        if (current == index) return &slot;
        current++;
      }
    }
    return nullptr;
  }
} inline LocoCache;
