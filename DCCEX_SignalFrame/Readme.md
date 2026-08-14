# DCCEX Controlled locking lever frame

I wanted something reminiscent of a lever frame for my model railway, not a mimic board - but more a row of numbered levers.
I also wanted some sort of interlock like the real thing to try and prevent invalid pulls and add some realism/complexity to managing the points.

This is built and working, however not yet full controlling the model railway. It is entirely possible it will not be fun! If that turns out to be the case its just EXRAIL logic and can be amended.

I have a contingency option of a switch elsewhere to relax the locking logic. If the bitmap pin PANEL_BYPASS is set then the levers will just operate their actions.

I believe sequences are limited to 255 in EXRAIL, this approach uses 3 no matter how many switches are controlled.


# Hardware
The signal frame uses 3x 23017s for input output.
Each board has 
  - 8 ground switches outputs to resistor protected LEDS
  - 8 ground switches SPST lever toggle switches

The EXRAIL logic enables interlocking between levers, an appoximation of interlocking on a real railway signal frame.

# Operation
The principle is based on 'permission to pull' which gives a direct indication of which lever movements are legal at all times.

LED Lit = Unlocked, lever maybe moved

LED off = Locked, lever is locked and should not be moved

LED Flashing = lever was moved while locked, the input was ignored and the lever should be moved back

At startup the lever frame checks for moved levers and resets, with user help, the lever positions and all point/signal positions.

The example code is not yet portable, its my main layout station so contains specifics, alias naming etc to that station.

# Principle - encoding locking logic
This allows the basic principle of locking to be encoded, e.g.

ALIAS(troyInterlock)
SEQUENCE(troyInterlock)

... Example of signals protecting points and points locking out signals

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

... Example of a mutualy exclusive set of levers, representing routes, only one can be selected


	IF_ANY(LEVER(20), LEVER(21), LEVER(22), LEVER(23), LEVER(24))
    IFLEVER(20) UNLOCK(20) ELSE LOCK(20) ENDIF
    IFLEVER(21) UNLOCK(21) ELSE LOCK(21) ENDIF
    IFLEVER(22) UNLOCK(22) ELSE LOCK(22) ENDIF
    IFLEVER(23) UNLOCK(23) ELSE LOCK(23) ENDIF
    IFLEVER(24) UNLOCK(24) ELSE LOCK(24) ENDIF
  ELSE
    UNLOCK(20) UNLOCK(21) UNLOCK(22) UNLOCK(23) UNLOCK(24)
  ENDIF

# Principle - encoding lever actions
Note - while each lever is deteced with an onsensor() event, the actions are all run from a single sequence.
This is required by the autostart routine so it can reset signals and points to a known good starting position,
Currently you cannot directly call an event from a sequence.

I have not tested, but understand DCC-EX will suppress the same message to the same vpin, so even though this is called for every valid lever pull it should not make point/signal motors/servos chatter.


ALIAS(troyActions)
SEQUENCE(troyActions)
	PRINT("Actions")

	IFLEVER(1)
	ENDIF

	IFLEVER(2)
		THROW(TROY_P_MayhillLoop)						// Point
		THROW(TROY_F_MayhillLoop)						// Frog
	ELSE
		CLOSE(TROY_P_MayhillLoop)
		CLOSE(TROY_F_MayhillLoop)
	ENDIF

...


![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_SignalFrame/IMG_1532.jpeg)

In this example - signals 1 & 3 protect point 2
If signal 3 is pulled, point 2's led will go out - locked
If the point 2 is pulled first, signal 3 will go out - locked, and signal 1 will light - unlocked

![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_SignalFrame/IMG_1538.jpeg)

This isn't as bad as it looks.. I hope.
A daisy chained spade connector cable is avaialble online - which hooks on to one side of the switches for common ground, and cut up for the other side feeding into the 23017s
Switches are wired:   Ground -> Switch -> 23017 pin

LED are all pre-wired off the shelf, run at 3.3v, bought as good for 6-9v (I may cut their brightness further with some diodes)
Note leds are grounded to turn on - so SET(led_vpin) = off,  RESET(led_vpin) = on

LEDS are wired: 3.3v -> LED/Inc resistor -> 23017 pin

![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_SignalFrame/IMG_1548.jpeg)
