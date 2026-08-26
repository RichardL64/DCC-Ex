/*
    Address.h
    R.A.Lincoln       2026

    Allow direct entry of a DCC address

*/

#pragma once


class addressClass {

private:
  int digits[4];
  int activeDigit = 0;


  //  The user interface
  //
  void drawUI() {
    scr.fb(c64::Light_green, c64::Green);
    scr.cls("Address");                                 // Header/footer

    updateUIDigits();

    scr.at(2, 10, "Turn: 0-9");
    scr.at(2, 11, "Click: Next");
  }

  //  Just the digits
  // 
  void updateUIDigits() {
    int x = 1;
    for (int i = 0; i < 4; i++) {                       // current 4 digit #
      if(i == activeDigit) {                            // active digit reverse colours
        scr.reverse();
        scr.at(x, 2, {0xf3, 0xf3, 0xf3});
        scr.at3x4(x, 3, digits[i]);      
        scr.reverse();

      } else {                                          // else the current colours
        scr.at(x, 2, "   ");
        scr.at3x4(x, 3, digits[i]);
      }
      x += 3;
    }
  }

public:
  void init() {
  }

  void switchTo() {
    drawUI();
  }


  //  Select next/previous
  //
  void handleEncoder(int step) {
    digits[activeDigit] += (step > 0 ? 1 : -1);

    if (digits[activeDigit] > 9) digits[activeDigit] = 9;     // no wrapping
    if (digits[activeDigit] < 0) digits[activeDigit] = 0;

    updateUIDigits();                                         // updates
  }


  //  Encoder button, return true to exit
  //
  bool handleEncoderButton() {
    activeDigit++;
    if (activeDigit >= 4) {
      activeDigit = 0;                                        // Reset for next use
      return true;                                            // --> 4 digits finished
    }

    updateUIDigits();
    return false;                                             // --> stay on this screen
  }


  //  Return the selected dcc address
  //
  int selectedAddress() {
    return (digits[0] * 1000) + (digits[1] * 100) + (digits[2] * 10) + digits[3];
  }

} inline Address;
