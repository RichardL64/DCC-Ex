/*
    Comms.h
    R.A.Lincoln       2026

    Handle Wi-Fi provisioning and DCC-EX TCP socket connectivity
    Updates the wifi status top right corner directly
*/
#pragma once
#include <WiFi.h>
#include <Preferences.h>

#include "Screen.h"

#define SSID_SIZE   65
#define PASS_SIZE   65
#define IP_SIZE     20


class commsClass {

private:
  Preferences prefs;

  // Stored Network configuration
  char savedSSID[SSID_SIZE];
  char savedPassword[PASS_SIZE];
  char savedIP[IP_SIZE];
  uint16_t savedPort;

  // Active DCC TCP Socket Client
  WiFiClient dccClient;

  int foundNetworksCount = 0;


  // Read stored values with defaults
  //
  void loadSettings() {
    savedSSID[0]     = '\0';
    savedPassword[0] = '\0';
    strcpy(savedIP, "192.168.4.1");
    savedPort        = 2560;

    prefs.begin("dccex_cfg", true);
    prefs.getString("ssid", savedSSID, sizeof(savedSSID));
    prefs.getString("pass", savedPassword, sizeof(savedPassword));
    prefs.getString("ip",   savedIP,   sizeof(savedIP));
    savedPort = prefs.getUShort("port", savedPort);
    prefs.end();
  }


  // Save settings only if values have changed
  //
  void saveSettings(const char* ssid, const char* password, const char* ip, uint16_t port) {
    if (strcmp(savedSSID, ssid) == 0 &&
        strcmp(savedPassword, password) == 0 &&
        strcmp(savedIP, ip) == 0 &&
        savedPort == port) {
      return;                                           // --> Nothing changed
    }

    prefs.begin("dccex_cfg", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.putString("ip",   ip);
    prefs.putUShort("port", port);
    prefs.end();

    snprintf(savedSSID,     sizeof(savedSSID),     "%s", ssid);
    snprintf(savedPassword, sizeof(savedPassword), "%s", password);
    snprintf(savedIP,       sizeof(savedIP),       "%s", ip);
    savedPort = port;
  }


  // Derive default DCC-EX AP password if SSID follows "DCCEX_xxxxxx"
  //
  void derivePassword(const char* ssid, char* password, size_t maxLen) {
    if (strncmp(ssid, "DCCEX_", 6) != 0) return;        // --> Not on DCC AP

    snprintf(password, maxLen, "PASS_%s", ssid + 6);
  }


  // Derive DCC IP from Wi-Fi Gateway if connected directly to a DCC-EX AP
  //
  void deriveIP(const char* ssid, char* ip, size_t maxLen) {
    if (strncmp(ssid, "DCCEX_", 6) != 0) return;        // --> Not on DCC AP

    IPAddress gw = WiFi.gatewayIP();
    if (gw != IPAddress(0, 0, 0, 0)) {
      snprintf(ip, maxLen, "%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
    }
  }


  //  Raw Wi-Fi connection handler
  //  Blocks while waiting for a connection
  //
  bool connectWiFi(const char* ssid, const char* password) {

    scr.status("Wifi Start");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();                                  // make sure the old connection is fully down
    delay(100);

    WiFi.begin(ssid, password);

    for (int attempts = 0; attempts < 20; attempts++) {
      if (WiFi.status() == WL_CONNECTED) {
        scr.status("WiFi Up");                          // Status line update
        return true;                                    // --> Success
      }
      scr.status("Wifi Waiting..");
      delay(300);
    }

    scr.status("Wifi failed");
    return false;                                       // --> Timed out
  }


  //  TCP connection to Command Station
  //
  bool connectDCC(const char* ip, uint16_t port) {
    dccClient.stop();                                   // Tear down any stale socket
    if(dccClient.connect(ip, port)) {
      scr.status("DCC Connected");
      return true;                                      // -->
    }

    scr.status("DCC Failed");
    return false;                                       // -->
  }


public:

  //  To allow external managment of messaging (mostly by dcc.h)
  //
  WiFiClient dcc() {
    return dccClient;
  }

  // Bootstrap from stored NVS settings
  //
  void init() {
    loadSettings();

    if (!savedSSID[0]) return;                          // --> No stored SSID
    if (!connectWiFi(savedSSID, savedPassword)) return; // --> Failed Wi-Fi
    
    connectDCC(savedIP, savedPort);
  }


  // Wi-Fi Scanning
  //
  int startWifiScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    foundNetworksCount = WiFi.scanNetworks();
    return foundNetworksCount;
  }

  int getNetworkCount() const { return foundNetworksCount; }


  // Zero-heap SSID lookup from scan results
  //
  void getScannedSSID(int index, char* dest, size_t maxLen) {
    if (index >= 0 && index < foundNetworksCount) {
      snprintf(dest, maxLen, "%s", WiFi.SSID(index).c_str());
      return;
    }
    dest[0] = '\0';
  }


  // Explicit connection request with auto-derivation & settings save
  //
  bool connectTo(const char* ssid, const char* password, const char* ip, uint16_t port) {
    if (!ssid || !ssid[0]) return false;                // --> Empty SSID

    char activePass[PASS_SIZE];
    char activeIP[IP_SIZE];

    snprintf(activePass, sizeof(activePass), "%s", password);
    snprintf(activeIP,   sizeof(activeIP),   "%s", ip);

    derivePassword(ssid, activePass, sizeof(activePass));
    if (!connectWiFi(ssid, activePass)) return false;   // -->

    deriveIP(ssid, activeIP, sizeof(activeIP));
    if (!connectDCC(activeIP, port)) return false;      // -->

    saveSettings(ssid, activePass, activeIP, port);     // Save known good setup
    return true;
  }


  

  // Periodic poll from main the loop
  //
  void tick() {
    static unsigned long lastTick = 0;
    if (millis() - lastTick < 250) return;              // --> Control call rate
    lastTick = millis();

    // 1. UI / Icon Update
    //
    c64 colour = c64::Red;                              // Default: Disconnected
    const char* glyph = "\x04";
    if (WiFi.status() == WL_CONNECTED) {                // Wi-Fi active
      colour = c64::Yellow;
      glyph = "\x05";
    }
    if (dccClient.connected()) {                        // DCC TCP socket active
      colour = c64::Green;
      glyph = "\x06";
    }
    scr.at(14, 0, glyph, c64::White, colour);           // Draw it


    // 2. Network & Reconnection State Machine
    //
    static bool reconnectPending = false;

    if (WiFi.status() == WL_CONNECTED) {                // WiFi UP
      reconnectPending = false;                         // Reset flag here when link is restored
      if (!dccClient.connected()) {                     // WiFi up -> ensure socket connection
        connectDCC(savedIP, savedPort);
      }

    } else {                                            // WiFi down
      dccClient.stop();

      if (!reconnectPending) {                          // Fire reconnect trigger once
        WiFi.disconnect();
        delay(100);
        WiFi.reconnect();
        reconnectPending = true;
      }
    }
  }

} inline Comms;
