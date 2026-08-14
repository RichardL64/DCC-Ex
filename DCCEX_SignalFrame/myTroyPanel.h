/*
	myTroyPanel.h

	R.A.Lincoln	August 2026
	Support from Gemini AI for debugging

	UP 	-->
	Down	<--
															To release
	1	UP MAYHILL OUTER HOME			2
	2	MAYHILL LOOP POINTS				1,3
	3	DOWN MAYHILL START'G			2
	4	UP MAYHILL HOME						10
	5	PLAT. 1 JUNC. POINTS			9,11

	6	UP TINTERN OUTER HOME			7
	7	TINTERN LOOP POINTS				6,8
	8	DOWN TINTERN START'G			7
	9	UP TINTERN HOME						5
	10	PLAT. 2 JUNC. POINTS		4,12

	11	DOWN PLAT. 1 START'G		5
	12	DOWN PLAT. 2 START'G		10
	13	PLAT. X-OVER						14,15,17

	14	UP PLAT. 1 START'G			16
	15	UP PLAT. 2 START'G			16
	16	TUNNEL JUNC. POINTS			14,15,17
	17	DOWN MAIN HOME					16

	18	LOW REV'S LOOP

	19	GOODS YARD POINTS
	20	BAY SIDING							21,22,23,24

	21	NO. 1 GOODS SIDING			20,22,23,24
	22	NO. 2 GOODS SIDING			20,21,23,24
	23	GOODS SHED SIDING				20,21,22,24
	24	NO. 3 GOODS SIDING			20,21,22,23


	Lever #			 1  2  3  4  5  6  7  8   9 10 11 12 13 14 15 16  17 18 19 20 21 22 23 24

	Fault   900 01 02 03 04 05 06 07 08  09 10 11 12 13 14 15 16  17 18 19 20 21 22 23 24
	Lock    925 01 02 03 04 05 06 07 08  09 10 11 12 13 14 15 16  17 18 19 20 21 22 23 24

  Leds       208 09 10 11 12 13 14 15 224 25 26 27 28 29 30 31 240 41 42 43 44 45 46 47
	Switches   200 01 02 03 04 05 06 07 216 17 18 19 20 21 22 23 232 33 34 35 36 37 38 39
	
	
	Vpin numbering:
		32768									System limit
		 MDPP
		 M		Mux			7x	0-9	Mux #
		  D		CS			0				Command station pins
		  		23017		2				0x2n addresses
		 			9685		4				0x4n addresses
					Signal	7				Virtual signals
					Turnout	8				Virtual turnouts
		 			Bitmap	9				Bitmap flags
		   PP	Pin			0-99		Device pin
*/

// ============================================================================
// PANEL CONFIGURATION
// ============================================================================
#define PANEL_NAME          Troy
#define PANEL_23017					200    	// MCP23017 boards at 200, 216, 232
#define PANEL_FAULTS				900    	// VPINs 900..924
#define PANEL_LOCKS					925    	// VPINs 925..949
#define PANEL_BYPASS				950			// If set all interlocking ignored
// ============================================================================


HAL(MCP23017, PANEL_23017, 16, 0x21)					// 200-247
HAL(MCP23017, PANEL_23017 +16, 16, 0x22)
HAL(MCP23017, PANEL_23017 +32, 16, 0x23)
HAL(Bitmap, 	PANEL_FAULTS,	25)								// 0=master, 1-24 lever faults
HAL(Bitmap,		PANEL_LOCKS,	25)								// 25 = master, 1-24 lever locks
HAL(Bitmap, 	PANEL_BYPASS,	1)								// Bypass interlock


//	Map lever numbers to vpins
//
#define VPIN_BASE(lever)          (PANEL_23017 + ((((lever) - 1) / 8) * 16))		// Board 	200, 216, or 232
#define LEVER(lever)         			(VPIN_BASE(lever) + (((lever) - 1) % 8))			// Switch	(200..207, 216..223, 232..239)
#define LED(lever)           			(VPIN_BASE(lever) + (((lever) - 1) % 8) + 8)	// LED		(208..215, 224..231, 240..247)


//	LEDS grounded to turn on, everything is oposite
//
#define LED_OFF(lever)            SET(LED(lever))
#define LED_ON(lever)             RESET(LED(lever))
#define LED_BLINK(lever, on, off) LED_ON(lever) \
                                  DELAY(10) \
                                  BLINK((LED(lever)), (off), (on))							// make sure the initial phase is on


#define IFLEVER(lever)            IF(LEVER(lever))
#define IFNOTLEVER(lever)					IFNOT(LEVER(lever))


#define EXPAND24(x) \
	x(1)  x(2)  x(3)  x(4)  x(5)  x(6)  x(7)  x(8)  \
	x(9)  x(10) x(11) x(12) x(13) x(14) x(15) x(16) \
	x(17) x(18) x(19) x(20) x(21) x(22) x(23) x(24)


//	Lever fault flags
//
#define FAULT0								PANEL_FAULTS

//	Note - led blink redundant here - the interlock sequence also does it
#define FAULT(lever)					SET(PANEL_FAULTS +lever)		LED_BLINK(lever, 100, 250)
#define IFFAULT(lever)				IF(PANEL_FAULTS +lever)
#define IFNOTFAULT(lever)			IFNOT(PANEL_FAULTS +lever)

#define CLEAR_FAULT(lever)		RESET(PANEL_FAULTS +lever)	LED_OFF(lever)

//	Lever locked flags
//
#define LOCK(lever)						SET(PANEL_LOCKS +lever)			LED_OFF(lever)
#define IFLOCKED(lever)				IF(PANEL_LOCKS +lever)

#define UNLOCK(lever)					RESET(PANEL_LOCKS +lever)		LED_ON(lever)
#define IFUNLOCKED(lever)			IFNOT(PANEL_LOCKS +lever)

//	Autostart
//
//	Phase
//		1 - fault any pulled lever and loop until they are all reset, note only call blink once per led
//		2 - call lever routines to sync the layout to the lever, reset, position
//		3 - initialise interlocking for correct led illumination
//
#define FAULT_LEVER(lever) \
	IFLEVER(lever) 				\
		IFNOTFAULT(lever)		\
			FAULT(lever) 			\
		ENDIF								\
	ELSE									\
		CLEAR_FAULT(lever)	\
	ENDIF

ALIAS(troyResetLevers)
SEQUENCE(troyResetLevers)
	EXPAND24(FAULT_LEVER)
	CALL(troyInterlock)
	IF(FAULT0)
		DELAY(200)
		FOLLOW(troyResetLevers)			// Loop until no faults
	ENDIF
RETURN

AUTOSTART
  FAULT(1)											// Shield: Suppress incoming sensor events during boot
  DELAY(200)                		// Allow MCP23017 bus reads / sensor sweeps to settle

  CALL(troyResetLevers)     		// Loop until physical levers are returned to NORMAL
  CALL(troyActions)         		// Sync layout points/signals to initial lever positions
  CALL(troyInterlock)       		// Evaluate interlocking rules and illuminate panel LEDs
  PRINT("Autostart end")
DONE

//  Lever fault check sequence
//
#define FAULT_LED_CHECK(lever) \
  IFFAULT(lever)               \
    LED_BLINK(lever, 100, 250) \
  ELSE                         \
    LED_OFF(lever)             \
  ENDIF

//
//	Manage locking logic
//
ALIAS(troyInterlock)
SEQUENCE(troyInterlock)

	//	Lever faults set FAULT0 for reference and block any actions
	//	IF_ANY Max 10 parameters
	//
	RESET(FAULT0)
	IFNOT(PANEL_BYPASS)							// Dont look for faults in bypass mode
		IF_ANY(PANEL_FAULTS+1,  PANEL_FAULTS+2,  PANEL_FAULTS+3,  PANEL_FAULTS+4,  PANEL_FAULTS+5,  PANEL_FAULTS+6,  PANEL_FAULTS+7,  PANEL_FAULTS+8)  SET(FAULT0) ENDIF
		IF_ANY(PANEL_FAULTS+9,  PANEL_FAULTS+10, PANEL_FAULTS+11, PANEL_FAULTS+12, PANEL_FAULTS+13, PANEL_FAULTS+14, PANEL_FAULTS+15, PANEL_FAULTS+16) SET(FAULT0) ENDIF
		IF_ANY(PANEL_FAULTS+17, PANEL_FAULTS+18, PANEL_FAULTS+19, PANEL_FAULTS+20, PANEL_FAULTS+21, PANEL_FAULTS+22, PANEL_FAULTS+23, PANEL_FAULTS+24) SET(FAULT0) ENDIF
		IF(FAULT0)
			EXPAND24(FAULT_LED_CHECK)		// Only leave faulted leds lit/blinking
			RETURN											// -->
		ENDIF
	ENDIF

	//	Mayhill approach
	//
	IF_ANY(LEVER(1), LEVER(3))		// Up Mayhill outer, Down Mayhill starting
		LOCK(2)											// Mayhill loop points
	ELSE
		UNLOCK(2)
	ENDIF

	IFLEVER(2)										// Mayhill loop points
		UNLOCK(1)
		LOCK(3)											// Dont allow down if points against it
	ELSE
		LOCK(1)											// Dont allow up to down line
		UNLOCK(3)
	ENDIF

	//	Tintern approach
	//
	IF_ANY(LEVER(6), LEVER(8))		// Signals lock points
		LOCK(7)
	ELSE
		UNLOCK(7)
	ENDIF

	IFLEVER(7)										// Points lock signals
		UNLOCK(6)
		LOCK(8)
	ELSE
		LOCK(6)
		UNLOCK(8)
	ENDIF

	//	Platforms
	//
	IF_ANY(LEVER(9), LEVER(11))		// Signals Platform 1
		LOCK(5)
	ELSE
		UNLOCK(5)
	ENDIF

	IFLEVER(5)										// Points
		UNLOCK(9)
		LOCK(11)
	ELSE
		LOCK(9)
		UNLOCK(11)
	ENDIF

	IF_ANY(LEVER(4), LEVER(12))		// Signals Platform 2
		LOCK(10)
	ELSE
		UNLOCK(10)
	ENDIF

	IFLEVER(10)										// Points
		UNLOCK(4)
		LOCK(12)
	ELSE
		LOCK(4)
		UNLOCK(12)
	ENDIF


	//	Tunnel
	//
  // Signals 14, 15, or 17 lock Junction Points 16
  IF_ANY(LEVER(14), LEVER(15), LEVER(17))
    LOCK(13)
    LOCK(16)
  ELSE
    UNLOCK(13)
    UNLOCK(16)
  ENDIF

  // Junction Points 16 & 13 lock/unlock Signal Levers (14, 15, and 17)
  IF_ANY(LEVER(13), LEVER(16))
    // Points REVERSE: Allow Up Platform Starting signals, lock Down Main Home
    UNLOCK(14)
    UNLOCK(15)
    LOCK(17)
  ELSE
    // Points NORMAL: Allow Down Main Home, lock Up Platform Starting signals
    LOCK(14)
    LOCK(15)
    UNLOCK(17)
  ENDIF

	//	Reversing loop vs. run around avilable unless the up starter is green
	IF_ANY(LEVER(14), LEVER(15), LEVER(17))
		LOCK(18)
	ELSE
		UNLOCK(18)
	ENDIF

	//	Goods yard

	//	Mayhill approach up/down and down starters must all be normal
	IF_ALL(LEVER(1), LEVER(2), LEVER(3), LEVER(5), LEVER(11))
		UNLOCK(19)
	ELSE
		LOCK(19)
	ENDIF

	//	Mutually exclusive goods yard routes
	//
	// If ANY goods lever is pulled, lock the others
	IF_ANY(LEVER(20), LEVER(21), LEVER(22), LEVER(23), LEVER(24))
    IFLEVER(20) UNLOCK(20) ELSE LOCK(20) ENDIF
    IFLEVER(21) UNLOCK(21) ELSE LOCK(21) ENDIF
    IFLEVER(22) UNLOCK(22) ELSE LOCK(22) ENDIF
    IFLEVER(23) UNLOCK(23) ELSE LOCK(23) ENDIF
    IFLEVER(24) UNLOCK(24) ELSE LOCK(24) ENDIF
  ELSE
    // No goods levers pulled -> all unlocked
    UNLOCK(20) UNLOCK(21) UNLOCK(22) UNLOCK(23) UNLOCK(24)
  ENDIF

	PRINT("Interlocking end")
RETURN


//	Lever action event handler definitions
//
#define LEVER_EVENT(lever)                          		\
  ONSENSOR(LEVER(lever))                            		\
                                                    		\
    /* 1. FAULT CLEAR PATH: Return faulted lever to Normal */ \
    IFFAULT(lever)                                  		\
      PRINT("Clear fault")                          		\
      CLEAR_FAULT(lever)                            		\
      CALL(troyInterlock)  /* Refresh panel LEDs */  		\
      DONE                /* STOP HERE! Do not run troyActions or fault checks */ \
    ENDIF                                           		\
                                                    		\
		/* 2. FAULT PATH: Locked or other faults active */	\
		IFNOT(PANEL_BYPASS)																	\
			IF_ANY(PANEL_LOCKS +lever, PANEL_FAULTS)					\
				PRINT("Fault")                   								\
				FAULT(lever)                                  	\
				CALL(troyInterlock)                            	\
				DONE                                          	\
			ENDIF          																		\
		ENDIF          																			\
                                                    		\
    /* 3. VALID MOVE PATH: Panel is clean & move is legal */ 	\
    CALL(troyActions)                               		\
    CALL(troyInterlock)                              		\
  DONE

EXPAND24(LEVER_EVENT)															// One event handler for each lever


//	Perform all actions, turnouts signals etc
//	Only called from the event handlers if there are no faults
//	DCC-Ex should ignore any duplicates
//
ALIAS(troyActions)
SEQUENCE(troyActions)
	PRINT("Actions")

	//	Mayhill approach
	//
	IFLEVER(1)
	ENDIF

	IFLEVER(2)
		THROW(TROY_P_MayhillLoop)						// Point
		THROW(TROY_F_MayhillLoop)						// Frog
	ELSE
		CLOSE(TROY_P_MayhillLoop)
		CLOSE(TROY_F_MayhillLoop)
	ENDIF

	IFLEVER(3)
	ENDIF
	IFLEVER(4)
	ENDIF

	IFLEVER(5)
		THROW(TROY_P_Plat1Junc)
		THROW(TROY_F_Plat1Junc)
	ELSE
		CLOSE(TROY_P_Plat1Junc)
		CLOSE(TROY_F_Plat1Junc)
	ENDIF

	//	Tintern approach
	//
	IFLEVER(6)
	ENDIF
	IFLEVER(7)
	ENDIF
	IFLEVER(8)
	ENDIF
	IFLEVER(9)
	ENDIF
	IFLEVER(10)
	ENDIF

	//	Platforms
	//
	IFLEVER(11)
	ENDIF
	IFLEVER(12)
	ENDIF
	IFLEVER(13)
	ENDIF

	//	Tunnel
	//
	IFLEVER(14)
	ENDIF
	IFLEVER(15)
	ENDIF
	IFLEVER(16)
	ENDIF
	IFLEVER(17)
	ENDIF
	IFLEVER(18)
	ENDIF

	//	Goods yard
	//
	IFLEVER(19)
	ENDIF
	IFLEVER(20)
	ENDIF
	IFLEVER(21)
	ENDIF
	IFLEVER(22)
	ENDIF
	IFLEVER(23)
	ENDIF
	IFLEVER(24)
	ENDIF

	PRINT("Actions end")
RETURN


