                                    TriggerAid
                                    ----------
                          HighSpeed Camera/Flash Trigger!
			  -------------------------------
                       http://www.facebook.com/TheTriggerAid
                         http://vegos.github.io/TriggerAid

		

Hardware Required:

Custom PCB (ATmega328-based, programmed in Arduino IDE) with 2x16 LCD (HD44780).
Parts are specified at "Parts.txt".



Libraries required:

LiquidCrystal - Arduino included library
MemoryFree - http://playground.arduino.cc/Code/AvailableMemory
EEPROM - Arduino included Library
multiCameraIrControl - http://sebastian.setz.name/arduino/my-libraries/multi-camera-ir-control/



Features:

Can trigger a camera' focus & shutter, or two cameras (with combined focus & shutter), 
or two flashes via WIRED jack.
kIt uses two (2) optocouplers (MOC3061 for up to 400V Peak).
It can also trigger a camera (or something else?) via IR. 
Supported cameras: Olympus, Pentax, Canon, Nikon, Sony.

It has 6 buttons (Reset, Previous, Next, Enter, Back, Shoot) and a LCD display for 
interfacing.



Modes:

-> Light Trigger. It can be programmed to trigger on HIGH or LOW, using built-in 
   light detector.
-> Time Lapse (0-360 secs).
-> External Trigger (for connecting (digital-output) sound/light/etc modules).
-> Bulb mode.
-> High Speed Burst Flash Triggering (1-100ms, for 1-50 times).
-> Manual Shoot (bulb) mode.
-> Setup Menu for storing default parameters (on EEPROM).



To be continued... 
