# DCC-EX
DCC-Ex components/customisations and all things Model Railway

# Why: 
I was keen to keep the best of old-school 70s era (when I was using it as a kid) control with modern DCC multi loco functionality for my layout.


The Controller is an old DC H&M Centre-off based remote. The ergonomics of the box may look bad - but the ergonomics of a rotary knob for shunting are awesome (In my opinion of course)

The throttle has no local display, instead passes messages to the control station to centrally display appropriate status - i.e. which loco is in control.
I didn't go near batteries for the remote - just a simple USB C for programming and power - potentially Comms too if I connect via Serial to the CS.

The loco select switch chooses the next/previous loco and the central display shows its DCC address and description.


# Challenges
  The controller knob has positional state - so when changing to another loco in the roster it does not generate speed control until the user syncs the knob position with the loco speed. The central CS display currently shows 'Rotate <<<' 'Rotate >>>' 'Rotate to off' type messages to aid this process. 

Its not that hard to get used to - you just rotate the controller in the direction on screen - when it gets to the loco's actual speed it will start to have an effect again.

Its a compromise for keeping the physical centre off detent - lots of potential here for more hints for the operator, perhaps even a haptic detent when passing the off position vs. a physical one?


# Controller: 
   Wemos D1 - 1x 20k POT input for speed and direction, 1x double pole momentary switch to select locos from the roster, 1x momentary button for emergency stop.
   The controller can connect to the CS either over its Wifi, or serial.
   
# DCC-Ex:
  Automation.h implements a command filter which watches for specific user ' <U n1 n2 n3> ' commands from the throttle to tell it which loco the throttle is controlling. (Many thanks to the DCC-Ex Discord support on how to do this!)


This is proof of concept - next steps are a separate display on the DCC-Ex command station specifically for loco information.

![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/IMG_0818.jpeg)

Default DCC decoder 0003 from the roster
![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/IMG_0824.jpeg)

Example loco roster display
![alt_text](https://github.com/RichardL64/DCC-Ex/blob/main/IMG_0828.jpeg)
