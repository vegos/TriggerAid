                                    TriggerAid
                                    ----------
                             The Camera/Flash Trigger!
			     -------------------------
                         http://vegos.github.io/TriggerAid
			 


Hardware Required:

Arduino (I used a Leonardo), ProtoShield, 2 Optocouplers, IR LED, 
Phototransistor, Buzzer, Tacticle Switches, Resistors, Wires, Jacks, and
some more things that I'm sure I forgot. 
Oh, and -last but not least- a nice enclosure to put the (finished) project inside :)



Libraries required:

Wire - Arduino included Library
LiquidCrystal_I2C - http://www.xs4all.nl/~hmario/arduino/LiquidCrystal_I2C/LiquidCrystal_I2C.zip
DS1307RTC - http://code.google.com/p/arduino-time/source/browse/#svn%2Ftrunk%2FDS1307RTC
Time - Arduino included Library
MemoryFree - http://playground.arduino.cc/Code/AvailableMemory
EEPROM - Arduino included Library
multiCameraIrControl - http://sebastian.setz.name/arduino/my-libraries/multi-camera-ir-control/



Features:

Can trigger a camera (with separate or no) focus & shutter, or two cameras
(with combined focus & shutter), or two flashes via WIRED jack. It uses two
(2) optocouplers (MOC3021 for up to 400V Peak).
It can also trigger a camera (or something else?) via IR. 
Supported cameras: Olympus, Pentax, Canon, Nikon, Sony.



Modes:

-> Light Trigger. It can be programmed to trigger on HIGH or LOW.
-> Time Lapse (0-360 secs).
-> External Trigger (for connecting (digital-output) sound/light/etc modules).
-> Bulb mode (1-999 seconds).
-> High Speed Flash Triggering (1-100ms, for 1-50 times).
-> Fully customizable / Menu driven.




To be continued... 
