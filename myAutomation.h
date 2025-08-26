
AUTOSTART
   SET_TRACK(A, MAIN)
   SET_TRACK(B, PROG)
   SET_POWER(A, ON)
   SET_POWER(B, ON)
   FOLLOW(1)

SEQUENCE(1)						// Clear system messages from the LCD
   LCD(2, "")
   LCD(3, "")
   LCD(4, "")
   LCD(5, "")
   LCD(6, "")
   LCD(7, "")
   LCD(8, "")
   DELAY(1000)
   FOLLOW(1)


// Roster formatted for 16 char LCD
//
//	      Whyte. Name............
ROSTER(3,    "       Default decoder ", "F0/F1/F2/F3/F4/F5/F6/F7/F8/F9/F10/")
ROSTER(3986, "0-6-0  BR 08  3986     ", "")
ROSTER(84,   "4-6-2  BR A3  Trigo    ", "")
ROSTER(8965, "0-6-0T BR J50 68965    ", "")


// Process inbound U commands from the throttle
//
STEALTH_GLOBAL(

  enum UCMD {						// U commands sent by the controller
    U_CONNECTED,
    U_WAIT_ROSTER,
    U_LOCO,
    U_WAIT_POT_STOP,
    U_WAIT_POT_SPEED
  };

  void myFilter(Print *stream, byte &opcode, byte &paramCount, int16_t p[]) {
    (void)stream;					// Tell compiler not to warn this is unused

    if(opcode != 'U') {					// Just want U
      return;						// ---> <X>
    }

    const FSH* name;

    switch(p[0]) {					// p[0] is the UCMD

    case U_CONNECTED:					// <U U_CONNECTED [Controller #]>
      StringFormatter::lcd(0, F("Controller      "));
      StringFormatter::lcd(1, F("       Connected"));
      break;

    case U_WAIT_ROSTER:					// <U U_WAIT_ROSTER [Try]>
      StringFormatter::lcd(0, F("Waiting for     "));
      StringFormatter::lcd(1, F("          Roster"));
      break;

    case U_LOCO:					// <U U_LOCO [LocoID]>
      name = RMFT2::getRosterName(p[1]);
      if (!name) return; 				// ---> <X>

      char line[20];
      strncpy(line, " ", 16);

      sprintf(line, F("%04d            "), p[1]);	// 0000		ID & \0
      strncpy(line +10, name, 6);			// 0-6-0T	Whyte notation
      StringFormatter::lcd(0, line);

//      LCD(0, F("%04d            "), p[1], name);	?

      StringFormatter::lcd(1, name +7);			// Description
      break;

    case U_WAIT_POT_STOP:				// <U U_WAIT_POT_STOP>
      StringFormatter::lcd(0, F("Rotate to Top   "));
      StringFormatter::lcd(1, F("   >>>> <<<<    "));
      break;
	
    case U_WAIT_POT_SPEED:				// <U U_WAIT_POT_SPEED [Direction]>
      StringFormatter::lcd(0, F("Rotate to Speed "));
      if(p[1] == 1) {
      	StringFormatter::lcd(1, F("   ---- >>>>    "));
      }
      if(p[1] == 0) {
      	StringFormatter::lcd(1, F("   <<<< ----    "));
      }
      break;

    default:
      return;						// ---> <X>
    }

    opcode = 0;						// Caller will ignore this
  }

)
