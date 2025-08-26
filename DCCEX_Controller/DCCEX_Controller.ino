//
//  DCC_EX Controller
//  Board:  esp8266 -> LOLIN(WEMOS) D1 mini (clone)
//
//  Minimal DCC controller:
//
//  Analogue centre detent pot input for speed
//
//  Digital buttons
//    1. SPDT momentary - Next/Previous Loco selection from the roster
//    2. Push momentary - Stop button
//
//  Wifi -or- Serial
//
//  After loco selection no speed outputs until the knob position matches the current speed
//
//  R.Lincoln     August 2025
//

//  Define connectivity - choose 1
//#define SERIAL_LOG                // for testing - turn off if talking to the CS on serial

//#define USE_SERIAL                // Comms over the USB serial line
#define USE_WIFI                  // Comms over Wifi


#include <DCCEXProtocol.h>
#include <ESP8266WiFi.h>
#include "config.h"
#include <Adafruit_Debounce.h>


//  User commands updating the CS with status
//
enum UCMD {
  U_CONNECTED,
  U_WAIT_ROSTER,
  U_LOCO,
  U_WAIT_POT_STOP,
  U_WAIT_POT_SPEED
};

//  Pot position constants
//  May need a calibration routine?
//
int potMin = 10;        
int potMax = 1024;
int detent = 572;                                          // detent & its dead zone
int detentLow = detent -20;    
int detentHigh = detent +20;


//  Speed knob wait states
//  Waiting for the user to sync position with internal system state
bool waitPotStop = true;
bool waitPotSpeed = false;
bool waitPotLeft = false;
bool waitPotRight = false;

//  Input controls last values
//
int speedSet;
unsigned long lastRead = 0;
unsigned long lastSpeed = 0;

int mappedSpeed = 0;
Direction mappedDirection = Direction::Forward;

int sentLocoAddress;
int sentSpeed;
Direction sentDirection;

bool csRosterReceived = false;
int csAddress = 0;
int csSpeed = 0;
Direction csDirection = Direction::Forward;


// define our loco object
Loco *loco = nullptr;


// Global objects
#ifdef USE_WIFI
WiFiClient client;
#endif

DCCEXProtocol dccex;

// Delegate class for callbacks
//
class MyDelegate : public DCCEXProtocolDelegate {
public:

  void receivedMessage(char *message) {
    Serial.printf("Message: %s\n", message);
  }
/*
  //  Response to any speed request changes
  void receivedLocoUpdate(Loco *l) {
    if(l->getAddress() == loco->getAddress()) {
      csAddress = l->getAddress();
      csSpeed = l->getSpeed();
      csDirection = l->getDirection();
    }
  }
*/
  //  Response to a specific loco speed request
  void receivedLocoBroadcast(int address, int speed, Direction direction, int functionMap) override {
    if(address == loco->getAddress()) {
      csAddress = address;
      csSpeed = speed;
      csDirection = direction;
    }
  }

};
MyDelegate myDelegate;

//  Buttons low when pressed
Adafruit_Debounce button1(D1, LOW);
Adafruit_Debounce button2(D2, LOW);
Adafruit_Debounce button3(D3, LOW);

//  Setup
//
void setup() {

  Serial.begin(115200);
  while (!Serial && millis() < 5000);                   // wait max 5s for Serial to start
  
  #ifdef SERIAL_LOG
  Serial.println("DCCEX Controller");
  Serial.println();
  #endif

  //  Setup hardware
  //
  pinMode(LED_BUILTIN, OUTPUT);                         // Note - low = on
  button1.begin();
  button2.begin();
  button3.begin();

  // Connect via WiFi network
  //
  #ifdef USE_WIFI
  Serial.println("Connecting to WiFi..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(1000);
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Connect to the DCC-Ex server
  //
  Serial.println("Connecting to the server...");
  if (!client.connect(serverAddress, serverPort)) {
    Serial.println("Connection failed");
    while (1) delay(1000);
  }
  Serial.println("Connected to the server");

  //  Connect DCC-Ex
  //
  dccex.setLogStream(&Serial);                  // Logging on Serial
  dccex.setDelegate(&myDelegate);               // Pass the delegate instance
  dccex.connect(&client);                       // Pass the Wifi comms link
  Serial.println("DCC-EX connected");
  #endif

  //  Connect via USB/serial cable
  //
  #ifdef USE_SERIAL
  dccex.setDelegate(&myDelegate);               // Pass the delegate instance
  dccex.connect(&Serial);                       // Pass native serial comms link
  #endif

  sendCmd(U_CONNECTED);

  //  Retreive the roster
  //
  #ifdef SERIAL_LOG
  Serial.println("Retreiving roster ");
  #endif

  dccex.getLists(true, false, false, false);                    // Roster, turnouts, routes, turntables
  for(int i=0; i<10; i++) {                                     // Wait max 10 seconds for the roster
    dccex.check();                                              // Single thread so allow DCCex to work
    if(dccex.getRosterCount()) break;
    sendCmd(U_WAIT_ROSTER, i);
    delay(1000);    
  }

  //  Identify the initial loco
  //
  if(dccex.getRosterCount()) {                                  // If there's anything in the list use the first one
    loco = dccex.roster->getFirst();
  } else {
    loco = new Loco(3, LocoSource::LocoSourceEntry);            // no roster - default to loco 3
  }
  sendCmd(U_LOCO, loco->getAddress());                          // Tell the CS what we are controlling
  dccex.powerOn();                                              // turn track power on

  lastRead = millis();
}


//  Loop
//
void loop() {
  //  Todo - catch network or server connection drop, loco drop etc

//  delay(10);
  dccex.check();                                              // Parse incoming messages
  button1.update();                                           // Button debounce library
  button2.update();
  button3.update();

  // Read inputs periodically 
  // Convert ~0-1024 to 0-126 and a direction
  // Too qiuckly on analogue impacts Wifi stability
  //
  if (millis() - lastRead >= 10) {                          
    speedSet = constrain(analogRead(A0), potMin, potMax);

    mappedSpeed = 0;                                          // posit stopped
    digitalWrite(LED_BUILTIN, HIGH);                          // LED off

    if(speedSet > detentHigh) {                               // forward
      mappedSpeed = map(speedSet, detentHigh, potMax, 0, 126);
      mappedDirection = Direction::Forward;
      digitalWrite(LED_BUILTIN, LOW);                         // LED on
    } 

    else if(speedSet < detentLow) {                           // reverse
      mappedSpeed = map(speedSet, potMin, detentLow, 126, 0);
      mappedDirection = Direction::Reverse;
      digitalWrite(LED_BUILTIN, LOW);                         // LED on
    }

    mappedSpeed = smoothAnalog(mappedSpeed);                  // Try and smooth out the jitter
    lastRead = millis();
  }

  //  Handle button presses
  //
  if(dccex.getRosterCount()) {                      // Only if there's a roster to iterate through

    //  Select previous loco on the roster
    //
    if(button2.justPressed()) {
      int locoA = loco->getAddress();               // Current loco address
      Loco *l = dccex.roster->getFirst();
      if(l->getAddress() == locoA) {                // if the current loco is the first one - set the loop to find the last
        locoA = -1;
      }
      for(; l != nullptr; l = l->getNext()) {       // Loop through the roster
        if(l->getAddress() == locoA) break;         // Stop at the current loco, leaving the previous loco in loco
        loco = l;                                   // Remember the previous loco
      }

      dccex.requestLocoUpdate(loco->getAddress());  // Request the new loco's info
      waitPotSpeed = true;                          // Wait for the control position to match the new loco's speed
      sendCmd(U_WAIT_POT_SPEED);                    // tell the CS
    }

    //  Select next loco on the roster
    //
    if(button1.justPressed()) {
      loco = loco->getNext();
      if(!loco) {
        loco = dccex.roster->getFirst();
      }
      dccex.requestLocoUpdate(loco->getAddress());  // Request the new loco's info
      waitPotSpeed = true;                          // Wait for the control position to match the new loco's speed
      sendCmd(U_WAIT_POT_SPEED);                    // tell the CS
    }
  }

  //  All stop
  //
  if(button3.justPressed()) { 
    dccex.emergencyStop();
    waitPotStop = true;                             // no more speed control until the pot is centered
    sendCmd(U_WAIT_POT_STOP);                       // tell the CS
  }


  //  Wait for the stop position
  //
  if(waitPotStop) {
    if(speedSet < detentLow or speedSet > detentHigh) {
      return;                                       // ---->
    }
    waitPotStop = false;
    sendCmd(U_LOCO, loco->getAddress());
  }

  //  Wait for the control to pass the loco speed
  //  First step - work out if we're waiting for a left or right turn
  //
  if(waitPotSpeed) {                                // waiting for position speed match
    if(loco->getAddress() != csAddress) {           // ? do we have the current locos info yet?
      return;                                       // ---->
    }

    //  There may be shorter ways to write this test - going for readable and let the compiler optimise it !
    //  csxxx     is the command station/loco current speed
    //  mappedxxx is the local pot mapped speed
    //
    waitPotLeft = false;
    waitPotRight = false;
    if(csSpeed == 0 and mappedSpeed == 0) {         // Special case - stationary forwards and backwards are the same thing
                                                    // just fall through no alignment required
    } else if (csDirection == Direction::Forward) {
      if(mappedDirection == Direction::Reverse)                             waitPotRight = true;
      if(mappedDirection == Direction::Forward and mappedSpeed < csSpeed)   waitPotRight = true;
      if(mappedDirection == Direction::Forward and mappedSpeed > csSpeed)   waitPotLeft = true;

    } else if (csDirection == Direction::Reverse) {
      if(mappedDirection == Direction::Forward)                             waitPotLeft = true;
      if(mappedDirection == Direction::Reverse and mappedSpeed < csSpeed)   waitPotLeft = true;
      if(mappedDirection == Direction::Reverse and mappedSpeed > csSpeed)   waitPotRight = true;
    }
    waitPotSpeed = false;
  }

  //  Waiting for the pot to turn left past the loco/cs speed
  //
  if(waitPotLeft) {
    sendCmd(U_WAIT_POT_SPEED, Direction::Reverse);

    if(mappedDirection  == csDirection and mappedSpeed <= csSpeed) {
      waitPotLeft = false;
    } else {
      return;                                       // ---->
    }
  }

  //  Waiting for the pot to turn right past the loco/cs speed
  //
  if(waitPotRight) {
    sendCmd(U_WAIT_POT_SPEED, Direction::Forward);

    if(mappedDirection  == csDirection and mappedSpeed >= csSpeed) {
      waitPotRight = false;
    } else {
      return;                                       // ---->
    }
  }
  
  sendCmd(U_LOCO, loco->getAddress());


  //  Fallthrough, all conditions met for control
  //
  if (millis() - lastSpeed < 50) {                  // Limit speed command frequency
    return;                                         // ---->
  }
  if(loco->getAddress() == sentLocoAddress 
  and mappedSpeed == sentSpeed
  and mappedDirection == sentDirection) {           // If the speed actually changed
    return;                                         // ---->
  }
  sentLocoAddress = loco->getAddress();
  sentSpeed = mappedSpeed;
  sentDirection = mappedDirection;

  dccex.setThrottle(loco, mappedSpeed, mappedDirection);
  lastSpeed = millis();

  // Note - flow only gets here if we are controlling the loco speed
}

//  Send user defined command with general status
//  <U [command] [parametr]>
//
void sendCmd(int cmd) {
  sendCmd(cmd, 0);
};


void sendCmd(int cmd, int p1) {
  static int lastCmd;
  static int lastP1;

  if(cmd == lastCmd and p1 == lastP1) {         // only send changes, dont SPAM the DCC server
    return;                                     // ---->
  }
  lastCmd = cmd;
  lastP1 = p1;

  char buffer[20];
  sprintf(buffer, "U %i %i", cmd, p1);
  dccex.sendCommand(buffer);

  #ifdef SERIAL_LOG
  Serial.printf("Send Cmd: %s\n", buffer);
  #endif
}

//  Try and smooth out the jitter on the potentiometer reading
//
int smoothAnalog(int reading) 
{
  const int numSamples = 50;
  static int samples[numSamples];
  static int sampleIndex = 0;
  static int sampleSum = 0;
  
  // Update sum
  sampleSum -= samples[sampleIndex];
  samples[sampleIndex] = reading;
  sampleSum += samples[sampleIndex++];
  sampleIndex = sampleIndex % numSamples;

  // Return average of last numSamples measurements
  return sampleSum/numSamples;
}
