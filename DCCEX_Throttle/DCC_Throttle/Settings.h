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


  //  Draw the UI
  //
  void drawUI() {
      scr.fb(c64::White, c64::Grey);
      scr.cls("Settings");

      // TBC - special char alignment in the charset  underscores

      scr.at(0, 2, "Network", scr.bg(), scr.fg());
      scr.at(0, 3, Comms.getSSID());

      scr.at(0, 11, "Password", scr.bg(), scr.fg());
      scr.at(0, 12, Comms.getPassword());

      scr.at(0, 14, "Server", scr.bg(), scr.fg());
      scr.at(0, 15, Comms.getIP());
      scr.at(0, 16, "%d", Comms.getPort());
  }

public:

  void init() {

  }

  void switchTo() {
    drawUI();
  }

  void handleEncoder(int step) {

  }

  bool handleEncoderButton() {
    LOG("Settings - connect");
    Comms.connectTo("DCCEX_a60e20", "PASS_a60e20", "192.168.4.1", 2560);

    LOG("Settings - request roster");
    DCC.requestRoster();

    LOG("Done");
    return false;                                       // --> stay on this screen
  }


} inline Settings;

/*
enum SettingsUXState {
  UX_SETTINGS_MENU,     // Main settings menu (Wi-Fi, Info, etc.)
  UX_SCANNING_WIFI,     // "Scanning..." screen
  UX_SELECT_WIFI,       // Scanned AP list menu
  UX_CONNECTING_WIFI,   // Connection in progress spinner
  UX_CONNECTED_SUCCESS  // Confirmation screen
};

SettingsUXState currentSettingsUX = UX_SELECT_WIFI;
int selectedNetworkIndex = 0;

// Render Settings Page UI
void drawSettingsUI() {
  tft.fillScreen(CONTRAST_BG);
  tft.setTextDatum(textdatum_t::top_left);

  // Header Bar
  tft.setTextColor(TFT_WHITE, CONTRAST_BG);
  tft.setFont(&fonts::FreeSansBold9pt7b);
  tft.drawString("SETTINGS", 15, 5);
  tft.drawFastHLine(15, 30, 210, TRACK_ACCENT);

  if (currentSettingsUX == UX_SCANNING_WIFI) {
    tft.setTextColor(TFT_YELLOW, CONTRAST_BG);
    tft.drawString("Scanning Wi-Fi APs...", 15, 60);
  } 
  else if (currentSettingsUX == UX_SELECT_WIFI) {
    tft.setFont(&fonts::FreeSans9pt7b);
    tft.setTextColor(TFT_WHITE, CONTRAST_BG);
    tft.drawString("Select DCC-EX AP:", 15, 42);

    int maxDisplay = min(foundNetworksCount, 5);
    for (int i = 0; i < maxDisplay; i++) {
      int yPos = 68 + (i * 26);
      String ssid = getScannedSSID(i);

      if (i == selectedNetworkIndex) {
        // Highlighted item
        tft.fillRect(15, yPos - 2, 210, 22, TRACK_ACCENT);
        tft.setTextColor(CONTRAST_BG, TRACK_ACCENT);
      } else {
        tft.setTextColor(TFT_WHITE, CONTRAST_BG);
      }

      tft.drawString(ssid, 20, yPos);
    }
  }
  else if (currentSettingsUX == UX_CONNECTING_WIFI) {
    tft.setTextColor(TFT_CYAN, CONTRAST_BG);
    tft.drawString("Connecting & Locking...", 15, 70);
  }
  else if (currentSettingsUX == UX_CONNECTED_SUCCESS) {
    tft.setTextColor(SIGNAL_GREEN, CONTRAST_BG);
    tft.drawString("Network Saved!", 15, 60);
    tft.setTextColor(TFT_WHITE, CONTRAST_BG);
    tft.drawString("SSID: " + savedSSID, 15, 90);
    tft.drawString("IP:   " + savedIP,   15, 110);
  }
}

// Handle Rotary Encoder Navigation for Settings Mode
void handleSettingsEncoder(int delta, bool buttonPressed) {
  if (currentSettingsUX == UX_SELECT_WIFI) {
    // Scroll list
    if (delta != 0 && foundNetworksCount > 0) {
      selectedNetworkIndex += delta;
      if (selectedNetworkIndex < 0) selectedNetworkIndex = 0;
      if (selectedNetworkIndex >= foundNetworksCount) selectedNetworkIndex = foundNetworksCount - 1;
      drawSettingsUI();
    }

    // Select AP
    if (buttonPressed && foundNetworksCount > 0) {
      String chosen = getScannedSSID(selectedNetworkIndex);

      currentSettingsUX = UX_CONNECTING_WIFI;
      drawSettingsUI();

      // Delegate hardware action to dcc_comms
      bool ok = connectAndSaveNetwork(chosen);

      if (ok) {
        currentSettingsUX = UX_CONNECTED_SUCCESS;
      } else {
        // Failed -> Rescan
        currentSettingsUX = UX_SCANNING_WIFI;
        drawSettingsUI();
        startWifiScan();
        currentSettingsUX = UX_SELECT_WIFI;
      }
      drawSettingsUI();
    }
  }
  else if (currentSettingsUX == UX_CONNECTED_SUCCESS && buttonPressed) {
    // Exit settings back to Drive Mode
    currentMode = MODE_DRIVE;
    drawDriveBackground();
  }
}
*/
