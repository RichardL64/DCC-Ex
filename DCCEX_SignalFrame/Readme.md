The signal frame uses 3x 23017s for input output.
Each board has 
  - 8 ground switches outputs to resistor protected LEDS
  - 8 ground switches SPST lever toggle switches

The EXRAIL logic enables interlocking between levers, an appoximation of interlocking on a real railway signal frame.

The principle is based on 'permission to pull' which gives a direct indication of which lever movements are legal at all times.

LED Lit = Unlocked, lever maybe moved
LED off = Locked, lever is locked and should not be moved
LED Flashing = lever was moved while locked, the input was ignored and the lever should be moved back

At startup the lever frame checks for moved levers and resets, with user help, the lever positions and all point/signal positions.

The example code is not yet portable, its my main layout station so contains specifics, alias naming etc to that station.


