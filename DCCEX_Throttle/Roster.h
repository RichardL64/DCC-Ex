/*
  Roster.h
  R.A.Lincoln       2026

  All funciotnality concerning the loco roster & loco descriptions

*/

#pragma once

#define ROSTERENTRIES 20                          // Max roster entries
#define LINESIZE      13                          // Display width for formatting names
#define PERPAGE       5


class rosterClass {

private:
  struct rosterEntry {
    int  locoId;
    char lines[3][LINESIZE +1];                  // 13 chars + null, Pre formatted for display
    bool valid = false;
  };

  rosterEntry roster[ROSTERENTRIES];

  int entries;                                  // Number of entries
  int selectedEntry;                            // Selected roster entry
  int topEntry;                                 // First entry on the page


  /*
    Example source data from DCC-Ex

    addEntry(84, "BR A3,TRIGO,4-6-2,60084");

    Assume comma delimited list - change formatting depending on the number of fields:
          Line 0      Line 1    Line 2
    1     id          f0
    2     id          f1        f2
    3     id  f1      f2        f3
    4     id  f1      f2  f3    f4
    5     id  f1      f2  f3    f4  f5

  */



  //  Overlay the strings on the left and right
  //
  void formatJustified(char* dest, const char* left, const char* right) {
    int spaces = LINESIZE - strlen(left) - strlen(right);
    snprintf(dest, LINESIZE +1, "%s%*s%s", left, spaces > 0 ? spaces : 1, "", right);
  }

  // Overlay string on the left end and pad out with spaces
  //
  void formatLeft(char* dest, const char* left) {
    snprintf(dest, LINESIZE +1, "%-13.13s", left);
  }

  // Overlay string on the right end - assumes pre-filled with spaces
  //
  void formatRight(char* dest, const char* right) {
    if (!right) return;
    int len = strlen(right);
    if (len <= LINESIZE) {
      strcpy(dest + LINESIZE - len, right);
    }
  }

  // Format raw roster entry into lines[0..3] (requires lines[4][14] array size)
  //
  // Format raw roster entry into 3 lines (locoId + up to 5 CSV fields from rawEntry)
  //
  void parseName(int entry, char* rawEntry) {
    auto lines = roster[entry].lines;

    char rawData[64] = {0};                                           // local scratch to modify
    if (rawEntry) strncpy(rawData, rawEntry, sizeof(rawData) - 1);

    char* fields[4] = {nullptr};                                      // Up to 4 fields from rawEntry
    int count = 0;

    char* ptr = strtok(rawData, "/\n\r,");                            // find the commas or slashes, note modifies the string
    while (ptr != nullptr && count < 4) {
      fields[count++] = ptr;
      ptr = strtok(nullptr, "/\n\r,");
    }

    snprintf(lines[0], LINESIZE, "#%-12.4d", roster[entry].locoId);   // Id top left
    formatLeft(lines[1], "");                                         // everything else padded empty
    formatLeft(lines[2], "");
    formatLeft(lines[3], "");

    switch (count) {
        case 1:                                                       // Line 0 right: f0
          formatRight(lines[0], fields[0]);
          break;

        case 2:                                                       // Line 0 right: f0 | Line 1: f1
          formatRight(lines[0], fields[0]);
          formatLeft(lines[1], fields[1]);
          break;

        case 3:                                                       // Line 0 right: f0 | Line 1: f1 | Line 2: f2
          formatRight(lines[0], fields[0]);
          formatJustified(lines[1], fields[1], fields[2]);
          break;

        case 4:                                                       // Line 0 right: f0 | Line 1: f1 & f2 | Line 2: f3
          formatRight(lines[0], fields[0]);
          formatJustified(lines[1], fields[1], fields[2]);
          formatRight(lines[2], fields[3]);
          break;

        default: // 0 CSV fields: ID only
          break;
      }
  }

  //  Screen display
  //
  void drawUI() {
    scr.fb(c64::White, c64::Orange);
    scr.cls("Roster");                                 // Header/footer
    drawUIEntries();
  }

  //  One three line entry
  //
  void drawUIEntry(int y, int entry) {
    bool hilight = (entry == selectedEntry);
    char* left = "\xf0";
    char* right = "\xf0";

    if(hilight) {                                                       // hilight and box left/right
      left = "\xf5";
      right = "\xfa";
      scr.reverse();                                                    // flip fg/bg
    };

    scr.at(0, y,  left);                                                // Line 0
    // tbc - print first 5 chars in reverse
    scr.at(1, y,  roster[entry].lines[0]);
    scr.at(14, y, right);

    y++;                                                                // Line 1
    scr.at(0, y,  left);
    scr.at(1, y,  roster[entry].lines[1]);
    scr.at(14, y, right);

    y++;                                                                // Line 2
    scr.at(0, y,  left);
    scr.at(1, y,  roster[entry].lines[2]);
    scr.at(14, y, right);

    if(hilight) scr.reverse();                                          // flip back for the next line
  }

  //  Empty entry for partial pages
  //
  void drawUIEmptyEntry(int row) {
    scr.at(0, row,     "               ");
    scr.at(0, row + 1, "               ");
    scr.at(0, row + 2, "               ");
  }

  //  Draw the entry list
  //  Hilight the selected entry
  //
  void drawUIEntries() {
    int y = 2;

    for (int i = 0; i < PERPAGE; i++) {                   // Draw the entries
      int entry = topEntry + i;
      if (entry < entries) {
        drawUIEntry(y, entry);
      } else {
        drawUIEmptyEntry(y);
      }

      y += 3;
    }

    if(topEntry > 0) {                                  // Can scroll up
      scr.at(14, 1, "\x5e");                            // Up arrow
    } else {
      scr.at(14, 1, " ");
    }
    if(topEntry + PERPAGE < entries) {                  // Can scroll down
      scr.at(14, 17, "\xbe");                           // Down arrow
    } else {
      scr.at(14, 17, " ");
    }
  }

  //  Add an entry into the internal table directly
  //
  void addEntry(int id, char* name) {
    if(entries == ROSTERENTRIES) entries--;                 // Prevent overflow, keep re-using the last one

    roster[entries].locoId = id;
    parseName(entries, name);
    entries++;
  }

public:

  //  Called during setup, populate the local roster information
  /*
        Line 1: [ #DCC# ] (REVERSED)      [ Loco# ]
        Line 2: [ Whyte ]                [ Class ]
        Line 3: < Name > (or blank if unnamed)
  */
  //
  void init() {
    entries = 0;
    addEntry(3,  "0003/Default");
    addEntry(86, "3986/BR 08/0-6-0");
    addEntry(84, "60084/BR A3/4-6-2/TRIGO");
    addEntry(65, "68965/BR J50/0-6-0T");
    addEntry(67, "D6700/BR 37/Co-Co");
    addEntry(87, "43187/BR 43/Bo-Bo/HST");
    addEntry(99, "000/Test 01");
    addEntry(98, "000/Test 02");
    addEntry(97, "000/Test 03");
    addEntry(96, "000/Test 04");

    //  TBC read the cs roster

    selectedEntry = 0;
    topEntry = 0;
  }

  void switchTo() {
      drawUI();
  }

  //  Select next/previous entry
  //
  void handleEncoder(int step) {

    selectedEntry += (step > 0 ? 1 : -1);                         // Selected entry within the roster
    if(selectedEntry > entries -1) selectedEntry = entries -1;    // No wrapping
    if(selectedEntry < 0) selectedEntry = 0;

    topEntry = selectedEntry /PERPAGE *PERPAGE;                   // Calc the page its on
    drawUIEntries();                                              // No flicker, only the hilighted line will change
  }

  //  Return the selected entry
  //
  bool handleEncoderButton() {
    return true;                                                  // Selected
  }

  //  Return info on the selected item
  //
  int selectedId() {
    return roster[selectedEntry].locoId;
  }

  char* line(int num) {
    return roster[selectedEntry].lines[num];    
  }

  // Select a loco from the list by id
  // If it's not there - add it for this session
  //
  void selectId(int id) {
    for (int i = 0; i < entries; i++) {                           // Search the roster
      if (roster[i].locoId == id) {
        selectedEntry = i;
        topEntry = (selectedEntry / PERPAGE) * PERPAGE;
        return;                                                   // -->
      }
    }


    char tempName[32];                                            // Not found, Create a temporary entry
    snprintf(tempName, sizeof(tempName), "%04d,Session temp", id);
    addEntry(id, tempName);

    selectedEntry = entries - 1;
    topEntry = (selectedEntry / PERPAGE) * PERPAGE;
  }


} inline Roster;

