/*
    Settings.h
    R.A.Lincoln     2026

    All settings/parameters
    Wifi & connectivity settings
    Brightness?
    Terminal

*/

#pragma once
#include "Comms.h"
#include "DCC.h"

class settingsClass {

private:

  //  Legal characters
  static constexpr const char* numberList12 = "012";
  static constexpr const char* numberList   = "0123456789";

  static constexpr const char* alphaList    = " !#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~" "\x0a";
  static constexpr const char* alphaListS   = "!#$%&\'()*+,-./:;<=>?@[\]^_`{|}~"      "\xd0\x0a";
  static constexpr const char* alphaListU   = "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" "\xd0\x0a";
  static constexpr const char* alphaListL   = "0123456789 abcdefghijklmnopqrstuvwxyz" "\xd0\x0a";

  enum Focus {SELECT, EDIT } currentFocus;
  enum Fields {NETWORK_LIST, NETWORK_SCAN, PASSWORD, PASSWORD_AUTO, SERVER, SERVER_AUTO, COUNT } currentField;
  int currentChar;

  int selectedSSID;                       // 0-7
  char activePassword[65];
  char activeServer[16];                  // 123.456.789.ABC
  char activePort[4];                     // 1234


  //  Draw the UI
  //
  void drawUI() {
      scr.fb(c64::White, c64::Grey);
      scr.cls("Settings");

      updateUI();
  }

  //  The parts that change
  //
  void updateUI() {
      scr.inverseIf(currentField == NETWORK_LIST);
      scr.at(0, 2, "Network");
      scr.inverseIf(currentField == NETWORK_SCAN);
      scr.at(11, 2, "Scan");

      scr.normal();
      char ssid[65];                                      // SSID list
      for(int i = 0; i<Comms.getSSIDCount(); i++) {
        Comms.getSSID(i, ssid, sizeof(ssid));
        scr.at(0, 3 +i, ssid);
      }

      scr.inverseIf(currentField == PASSWORD);
      scr.at(0, 12, "Password");
      scr.inverseIf(currentField == PASSWORD_AUTO);
      scr.at(11, 12, "Auto");

      scr.normal();
      char pass[65];
      snprintf(pass, sizeof(pass), "%-15.15s", activePassword);
      scr.atEdit(0, 13, pass, currentPasswordChar);

      scr.inverseIf(currentField == SERVER);
      scr.at(0, 15, "Server");
      scr.inverseIf(currentField == SERVER_AUTO);
      scr.at(11, 15, "Auto");

      scr.normal();
      scr.at(0, 16, activeServer);
      scr.at(0, 17, "%d", activePort);
  }


  //  Edit the last char in the buffer 
  //  Chars selected from the passed list
  //
  void editBuffer(char* buffer, int maxLen, const char* charSet, int step) {
      int charSetLen = strlen(charSet);

      // If the buffer is empty and we are starting fresh, initialize with the first char
      int currentLen = strlen(buffer);
      if (currentLen == 0 && maxLen > 0) {
          buffer[0] = charSet[0];
          buffer[1] = '\0';
          charIndex = 0;
          return;
      }

      // Find current character in the charset and shift by step with wrap-around
      int charIndex = strlen(buffer) -1;
      const char* found = strchr(charSet, buffer[charIndex]);         // address of the char in the charset
      int currentIndex = found ? (found - charSet) : 0;               // address to index
      currentIndex = (currentIndex + step + charSetLen) % charSetLen;

      buffer[charIndex] = charSet[currentIndex];
  }


public:


  void init() {
    Comms.getPassword(activePassword, sizeof(activePassword));
  }

  void switchTo() {
    currentFocus = SELECT;
    currentField = NETWORK_LIST;
    currentChar = -1;                           // Stop chars being hilighted
    drawUI();
  }

  //  Step through fields, 
  //  When selected step through options
  //
  void handleEncoder(int step) {

    // Axis 1: Select fields
    if (currentFocus == SELECT) {
        int newField = static_cast<int>(currentField) + step;
        if (newField >= COUNT) newField = COUNT - 1;
        if (newField < 0) newField = 0;
        currentField = static_cast<Fields>(newField);
    } 

    // Axis 2: We are locked inside a field. Select field values/chars 
    else {
        switch(currentField) {
        case NETWORK_LIST:
          // Scroll through available Wi-Fi networks instead of changing menu fields
//                  scrollNetworkList(step); 
          break;

        case PASSWORD:
          editBuffer(activePassword, sizeof(activePassword), alphaList, step);
          break;

        default:
          break;
        }
    }

    updateUI();
  }


  //  Select a field to edit,
  //  Select a char in an edit field - until enter or field complete
  //
  bool handleEncoderButton() {

    //  Select a field to edit
    //
    if(currentFocus == SELECT) {
      currentFocus = EDIT;
      currentChar = 0;
      return false;                                       // -->
    }

    //  Edit a field
    //
    switch(currentField) {
      //NETWORK_LIST, NETWORK_SCAN, PASSWORD, PASSWORD_AUTO, SERVER, SERVER_AUTO,

      case NETWORK_SCAN:
        scr.status("\xdf" " Scanning");
        Comms.startSSIDScan();
        updateUI();
        scr.status();
        break;

      case PASSWORD:
        if(activePassword[currentChar] == '\x0a') {       // Enter => edit complete
          currentChar = -1;
          currentFocus = SELECT;
          return false;                                   // -->
        }
        currentChar++;                                    // Any other char, move to the next one
        //   check for max field length

      case PASSWORD_AUTO:
//        Comms.derivePassword(Comms.getSSID(), activePassword, sizeof(activePassword));
        updateUI();
        break;

      case SERVER_AUTO:
//        Comms.deriveIP(activeSSID, activeServer, sizeof(activeServer));      
        break;

      default:
        break;
    }
    return false;                                       // --> stay on this screen
  }


} inline Settings;

