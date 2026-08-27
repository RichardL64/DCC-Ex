# DCC Throttle

Work in progress
ESP32 C3 Screen/Rotary/Button module
DCC-Ex throttle controller - talking to DCC-Ex with native <...> commands

# Control scheme
Rotary - choose/speed

Rotary button - enter

Button - mode/screen change

# Display
Screen.h      Pet/C64 Style matrix graphics engine

              256 chars, independent fg/bg colours, C64 reminiscent palette
              
              Scaled chars using blocky Petscii quarter char blocks

# Various modes:
  Drive         Drive locos
  
  Roster        Select loco from the DCC-ex roster
  
  Address       Enter a Dcc address directly
  
  Settings      TBC 
  
  Terminal      Serial terminal monitor

# Ux
15 column x 20 row

The font is doubled up to 16x16 to be a viewable size (but remains at 8x8 pixels if that makes sense)

Top row reserved for the page name, top right hand corner track and wifi status

Bottom row reserved for status messages

References:

Hellorld - Usagi, Pi - The Net

![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_Throttle/IMG_1570.jpeg)

# Font
Based on Petscii alpha shifted to ASCII locations

Screen.h supports fg/bg colours at a character level, so no need for reverse in the font itself

The bottom row $F0-$FF used for block/large character output
![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_Throttle/Screenshot%202026-08-18%20at%2020.17.36.jpg)

# Wiring
![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_Throttle/IMG_1595.jpeg)

![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/DCCEX_Throttle/IMG_1596.jpeg)
