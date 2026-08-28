/*
  Roster.h
  R.A.Lincoln       2026

  All funciotnality concerning the loco roster & loco descriptions

*/

#pragma once

#define ROSTERENTRIES 20                          // Max roster entries
#define LINESIZE      13                          // Display width for formatting names
#define PERPAGE       5

#include "DCCLocoCache.h"


class rosterClass {

private:
  int topEntry = 0;                             // First entry on the page
  int selectedEntry = 0;

  
  //  Screen display
  //
  void drawUI() {
    scr.fb(c64::White, c64::Orange);
    scr.cls("Roster");                                 // Header/footer
    drawUIEntries();
  }

  //  One three line entry
  //
  void drawUIEntry(int y, LocoInfo* info) {
    bool hilight = (info->locoId == LocoCache.getSlotByIndex(selectedEntry)->locoId);

    //  Build the hilight box left/right extensions
    const char* left = hilight ? "\xf5" : "\xf0";                       // note - ends drawn in inverse when hilighted
    const char* right = hilight ? "\xfa" : "\xf0";

    //  Draw the 3x lines
    //
    scr.inverse();
    scr.at(1, y, "%.*s", 5, info->lines[0]);                            // #0000 LocoId in inverse

    scr.inverseIf(hilight);
    scr.at(0, y,  left);                                                // Line 0
    scr.at(6, y, "%.*s", LINESIZE -1, info->lines[0]+5);                // Rest of the line normal
    scr.at(14, y, right);

    y++;                                                                // Line 1
    scr.at(0, y,  left);
    scr.at(1, y,  info->lines[1]);
    scr.at(14, y, right);

    y++;                                                                // Line 2
    scr.at(0, y,  left);
    scr.at(1, y,  info->lines[2]);
    scr.at(14, y, right);

    scr.normal();
  }

  //  Empty entry for partial pages
  //
  void drawUIEmptyEntry(int y) {
    scr.at(0, y,    "               ");
    scr.at(0, y +1, "               ");
    scr.at(0, y +2, "               ");
  }

  //  Draw the entry list
  //  Hilight the selected entry
  //
  void drawUIEntries() {
    int y = 2;

    //  Loco information
    int entries = LocoCache.getSlotCount();                       // Total valid slots
    for (int i = 0; i < PERPAGE; i++) {                           // For each screen position
      int entry = topEntry + i;
      if (entry < entries) {
        drawUIEntry(y, LocoCache.getSlotByIndex(entry));
      } else {
        drawUIEmptyEntry(y);
      }

      y += 3;
    }

    // Scroll arrow prompts
    scr.at(14, 1, topEntry > 0 ? "\xd0" : " ");                   // Up
    scr.at(14, 18,topEntry + PERPAGE < entries ? "\xe0" : " ");   // Down
  }


public:


  // Switch in and select a loco
  //
  void switchTo(int id) {
    LocoCache.getSlotByLocoId(id);                                // Find/create a slot for the id

    int entries = LocoCache.getSlotCount();                       // Total valid slots
    for (int i = 0; i < entries; i++) {                           // Work out which page the selected loco is on
      if (LocoCache.getSlotByIndex(i)->locoId == id) {
        selectedEntry = i;
        topEntry = (selectedEntry / PERPAGE) * PERPAGE;
        break;
      }
    }

    switchTo();
  }

  //  Switch in
  //
  void switchTo() {
      drawUI();
  }

  //  Select next/previous entry
  //
  void handleEncoder(int step) {

    int entries = LocoCache.getSlotCount();
    selectedEntry += (step > 0 ? 1 : -1);                         // Selected entry within the roster
    if(selectedEntry > entries -1) selectedEntry = entries -1;    // No wrapping
    if(selectedEntry < 0) selectedEntry = 0;

    topEntry = selectedEntry /PERPAGE *PERPAGE;                   // Calc the page its on
    drawUIEntries();                                              // No flicker, only the hilighted line will change
  }

  //  Exit
  //
  bool handleEncoderButton() {
    return true;                                                  // Exit the roster screen
  }


  //  Return info on the selected item
  //
  int getSelectedLocoId() {
    return LocoCache.getSlotByIndex(selectedEntry)->locoId;
  }

  


} inline Roster;

