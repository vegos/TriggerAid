// --- (New) CAMERA TRIGGER --------------------------------------------------------------------------------------------------------------------------
//
//     (c)2013, Antonis Maglaras
//     maglaras at gmail dot com
//     http://www.slr.gr 
//
//     Latest source code available at: https://github.com/vegos/NewCameraTrigger
//     Photos etc at: http://www.slr.gr/trigger
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
// Schematic
// ---------
// Απλά schematic δεν υπάρχει :)
//
// Το setup πάνω-κάτω είναι το εξής:
//
// 1 Arduino Leonardo 
// 1 ProtoShield
// 1 Κουτί (LinkSprite Clear Arduino Case)
// 1 IR LED για ενεργοποίηση φωτογραφικής μηχανής μέσω IR
// 2 Optocouplers (MOC3021 - 400 Volts Peak)
// 1 Photodiode για έλεγχο του φωτός
// 4 push-switches (για το πληκτρολόγιο, Αριστερά/Δεξιά/Enter/Back)
// 1 LCD οθόνη 16Χ2 με backlight
// 1 I2C driver για την LCD οθόνη
// Αντιστάσεις, connectors, κλπ :)
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------




// Libraries used in this sketch          -- Many thanks to the creators of the following libraries!
#include <Wire.h>                         // Arduino included Library
#include <LiquidCrystal_I2C.h>            // http://www.xs4all.nl/~hmario/arduino/LiquidCrystal_I2C/LiquidCrystal_I2C.zip
#include <DS1307RTC.h>                    // http://code.google.com/p/arduino-time/source/browse/#svn%2Ftrunk%2FDS1307RTC
#include <Time.h>                         // Arduino included Library
#include <MemoryFree.h>                   // http://playground.arduino.cc/Code/AvailableMemory
#include <EEPROM.h>                       // Arduino included Library
#include <multiCameraIrControl.h>         // http://sebastian.setz.name/arduino/my-libraries/multi-camera-ir-control/

LiquidCrystal_I2C lcd(0x20,16,2);

#define  Version         "1.0.2"          // Current Version


// Create two (2) custom characters (for visual identification of armed/disarmed mode)
byte ONChar[8] =  { B00000, B01110, B11111, B11111, B11111, B01110, B00000, B00000 };
byte OFFChar[8] = { B00000, B01110, B10001, B10001, B10001, B01110, B00000, B00000 };

// Translate button names to numbers for easier recognition
#define  LEFTKEY            1
#define  RIGHTKEY           2
#define  ENTERKEY           3
#define  BACKKEY            4
#define  NONEKEY            0

// Definition of pins used by Arduino
#define  IRLedPin          10      // For IR transmitting
#define  LightSensorPin    A1      // Built-in Light trigger

#define  Input1Pin         A0      // For future use / analog senors?
#define  ExternalPin        8      // For connecting external (digital) sensors
#define  BuzzerPin          9      // Buzzer -- I used a PWM pin. Maybe in next version will play some music :)

#define  RightButton        4      // Button Right Pin
#define  BackButton         5      // Buton Back Pin
#define  LeftButton         6      // Button Left Pin
#define  EnterButton        7      // Button Enter Pin

#define  Optocoupler1Pin   11      // Optocoupler 1 is used for focus triggering    \____ Olympus E-3 (the camera that I make the testing
#define  Optocoupler2Pin   12      // Optocoupler 2 is used for shutter triggering  /     needs focus & shutter to be triggered together (at least).
                                   //                                                     With that setup, it's possible to trigger two (2) external
                                   //                                                     flashes for example, or two cameras etc.

// Olympus IR remote trigger
Olympus Camera(IRLedPin);


// Main Menu Strings
char* MenuItems[8] = { "",
                       "-> Light Trigger",    // Mode 1: Built-in light trigger
                       "-> Ext. Trigger ",    // Mode 2: External Trigger (works with digital modules, like a sound or light module.
                       "-> Time Lapse   ",    // Mode 3: Time lapse. 
                       "-> Bulb Mode    ",    // Mode 4: Bulb mode (My camera can't keep the shutter for more than 60 secs, so that way I can go up to 8 minutes!
                       "-> Setup        ",    // Mode 5: Setup System (Delays, Backlight, Buzzer, Triggers, Date/Time)
                       "-> Information  ",    // Mode 6: Version information, memory free, etc.
                       "-> Factory Reset",    // Mode 7: Factory Reset.
                      };
// Definition of global variables

int MenuSelection = 1;
int CurrentHour = 99, CurrentMin = 99, CurrentSec = 99, CurrentDay = 99, CurrentMonth = 99, CurrentYear = 99;
int Mode = 1, ShortCut, OptocouplersStatus;
int LightThreshold = 0, Light = 0;
boolean StandBy = true;
boolean WhenHigh = false;
boolean Armed = false;
boolean BackLight, MakeSounds, Infrared, PreFocus, Optocoupler1Enabled, Optocoupler2Enabled;
boolean StayInside = false;
int PreDelay, ShutterDelay, AfterDelay;
long tmpDelay=60;
long StartMillis, tmpMillis, TriggerMillis;




// --- SETUP procedure -------------------------------------------------------------------------------------------------------------------------------
// Create custom characters, display a welcome message, setup pins, read and set system variables etc.

void setup()
{
  lcd.init();
  lcd.createChar(1, ONChar);
  lcd.createChar(2, OFFChar);
  lcd.backlight();
  setSyncProvider(RTC.get);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Camera Trigger ");
  lcd.setCursor(0,1);
  lcd.print("Antonis Maglaras");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("    WELCOME!    ");
  // Setup pins
  pinMode(RightButton, INPUT_PULLUP);  
  pinMode(BackButton, INPUT_PULLUP);
  pinMode(LeftButton, INPUT_PULLUP);
  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(ExternalPin, INPUT);
  pinMode(Optocoupler1Pin, OUTPUT);
  pinMode(Optocoupler2Pin, OUTPUT);
  // Read settings from EEPROM
  if (ReadFromMem(0)==1)
    MakeSounds=true;
  else
    MakeSounds=false;
  if (ReadFromMem(2)==1)
    BackLight=true;
  else
    BackLight=false;
  PreDelay=ReadFromMem(4);
  ShutterDelay=ReadFromMem(6);
  AfterDelay=ReadFromMem(8);
  if (ReadFromMem(10)==1)
    Infrared=true;
  else
    Infrared=false;
  if (ReadFromMem(12)==1)
    PreFocus=true;
  else
    PreFocus=false;
  ShortCut=ReadFromMem(14);
  OptocouplersStatus=ReadFromMem(16);
  switch (OptocouplersStatus)
  {
    case 0:
      Optocoupler1Enabled=true;
      Optocoupler2Enabled=true;
      break;
    case 1:
      Optocoupler1Enabled=true;
      Optocoupler2Enabled=false;
      break;
    case 2:
      Optocoupler1Enabled=false;
      Optocoupler2Enabled=true;
      break;
    case 3:
      Optocoupler1Enabled=false;
      Optocoupler2Enabled=false;
      break;
  }
  //
  delay(1000);
  if (BackLight)
    lcd.backlight();
  else
    lcd.noBacklight();
  StandBy=true;
  if (MakeSounds)
    Beep(1);
}




// --- MAIN procedure --------------------------------------------------------------------------------------------------------------------------------
// Check for system status (armed/disarmed), check for mode selection, display time.

void loop()
{
  if (StandBy)
  {
    // On RIGHT key press, turn LCD backlight on/off
    if (Keypress() == RIGHTKEY)
    {
      BackLight=!(BackLight);
      if (BackLight)
        lcd.backlight();
      else 
        lcd.noBacklight();
    }
    // On LEFT key press, start bulb shooting
    if (Keypress() == LEFTKEY)
    {
      lcd.setCursor(0,1);
      lcd.print("Bulb Shooting...");
      StartBulb();
      while (Keypress() == LEFTKEY);
      StopBulb();
      lcd.setCursor(0,1);
      lcd.print("                ");
      ResetTimeVars();
    }
    // On BACK key pressed for 3 seconds, go to shortcut
    if (Keypress() == BACKKEY)
    {
      long Temp=millis();
      {
        while (Backkey())
        {
          if (millis()-Temp>3000)
          {
            if (MakeSounds)
              Beep(3);
            if (Backkey())
            {
              lcd.setCursor(0,1);
              lcd.print("    Shortcut    ");
              while (Backkey());
            }
            Mode=ShortCut;
            StandBy=false;
            ResetTimeVars();
            return;
          }
        }
      }
    }
    // On ENTER key press, enter the main menu
    if (Keypress() == ENTERKEY)
      MainMenu();
    DisplayTime();  
  }
  else
  {
    switch (Mode)
    {
      case 1:
        lcd.clear();
        lcd.print("Light:          ");
        lcd.setCursor(0,1);
        lcd.print("Threshold:      ");
        Light = map(analogRead(LightSensorPin),0,1023,0,100);
        LightThreshold = Light+10;
        if (LightThreshold>100)
          LightThreshold = 100;
        Armed=false;
        lcd.setCursor(15,0);
        lcd.write(2);
        while (Keypress() != BACKKEY)
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.clear();
              lcd.print("Light Trigger   ");
              lcd.setCursor(0,1);
              lcd.print("Ready!          ");
              lcd.setCursor(15,0);
              lcd.write(1);
              if (PreFocus)
                PreFocusStart();
              while (!Backkey())
              {
                Light = map(analogRead(LightSensorPin),0,1023,0,100);
                if (Light>LightThreshold)
                {
                  TriggerMillis=millis();
                  Trigger();
                  if (PreFocus)
                    PreFocusStart();
                }
              }
              if (PreFocus)
                PreFocusStop();
              Armed=false;
              lcd.clear();
              lcd.print("Light:          ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:      ");
              lcd.setCursor(15,0);
              lcd.write(2);              
              delay(100);
            }
            else
            {
              lcd.clear();
              lcd.print("Light:          ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:      ");
              lcd.setCursor(15,0);
              lcd.write(2);               
            }
          }
          Light = map(analogRead(LightSensorPin),0,1023,0,100);
          lcd.setCursor(7,0);
          PrintDigits(Light,3);
          lcd.setCursor(11,1);
          PrintDigits(LightThreshold,3);
          if (Keypress() == RIGHTKEY)
          {
            LightThreshold+=1;
            if (LightThreshold>100)
              LightThreshold=100;
          }
          if (Keypress() == LEFTKEY)
          {
            LightThreshold-=1;
            if (LightThreshold<0)
              LightThreshold=0;
          }
        }
        StandBy=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 2:
        lcd.clear();
        lcd.print("Ext. Trigger    ");
        lcd.setCursor(0,1);
        lcd.print("Trigger on      ");
        Armed=false;
        lcd.setCursor(15,0);
        lcd.write(2);
        while (Keypress() != BACKKEY)
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(0,1);
              lcd.print("Ready!          ");
              lcd.setCursor(15,0);
              lcd.write(1);
              if (PreFocus)
                PreFocusStart();
              while (!Backkey())
              {
                if ((WhenHigh) && (digitalRead(ExternalPin) == HIGH))
                {
                  Trigger();
                  if (PreFocus)
                    PreFocusStart();
                }
                if ((!WhenHigh) && (digitalRead(ExternalPin) == LOW))
                {
                  Trigger();
                  if (PreFocus)
                    PreFocusStart();
                }
              }
              if (PreFocus)
                PreFocusStop();
              lcd.setCursor(0,1);
              lcd.print("Trigger on      ");
              Armed=false;
              lcd.setCursor(15,0);
              lcd.write(2); 
              delay(100);             
            }
            else
            {
              lcd.setCursor(0,1);
              lcd.print("Trigger on      ");
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          lcd.setCursor(11,1);
          if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
            WhenHigh=!(WhenHigh);
          if (WhenHigh)
            lcd.print("HIGH");
          else
            lcd.print("LOW ");
        }
        StandBy=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 3:
        lcd.clear();
        lcd.print("TimeLapse (   ) ");
        lcd.setCursor(0,1);
        lcd.print("Interval: 060\"   ");
        tmpDelay=60;
        StartMillis=millis();
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;
        while (Keypress() != BACKKEY)
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(15,0);
              lcd.write(1);
              StartMillis=millis();
            }
            else
            {
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          if (Armed)
          {
            lcd.setCursor(11,0);
            long remaining = (tmpDelay-((millis()-StartMillis)/1000));
            PrintDigits(remaining,3);            
            if ((millis()-StartMillis)>=(tmpDelay*1000))
            {
              Trigger();
              StartMillis=millis();
            }
          }
          else
          {
            lcd.setCursor(11,0);
            lcd.print("   ");
          }
          if (Keypress() == LEFTKEY)
          {
            tmpDelay-=1;
            if (tmpDelay<1)
              tmpDelay=1;
          }
          if (Keypress() == RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>360)
              tmpDelay=360;
          }
          lcd.setCursor(10,1);
          PrintDigits(tmpDelay,3);
        }
        StandBy=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 4:
        lcd.clear();
        lcd.print("Bulb Mode (   ) ");
        lcd.setCursor(0,1);
        lcd.print("Bulb: 030\"      ");
        tmpDelay=30;
        StartMillis=millis();
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;
        while (Keypress() != BACKKEY)
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(15,0);
              lcd.write(1);
              StartMillis=millis();
              StartBulb();
            }
            else
            {
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          if (Armed)
          {
            lcd.setCursor(11,0);
            long remaining = (tmpDelay-((millis()-StartMillis)/1000));
            PrintDigits(remaining,3);
            if ((millis()-StartMillis)>=(tmpDelay*1000))
            {
              StopBulb();
              Armed=false;
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          else
          {
            lcd.setCursor(11,0);
            lcd.print("   ");
          }
          if (Keypress() == LEFTKEY)
          {
            tmpDelay-=1;
            if (tmpDelay<1)
              tmpDelay=1;
          }
          if (Keypress() == RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>360)
              tmpDelay=360;
          }
          lcd.setCursor(6,1);
          PrintDigits(tmpDelay,3);
        }
        StandBy=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 5:
        SetupMenu();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 6:
        lcd.clear();
        lcd.print("Version: ");
        lcd.print(Version);
        lcd.setCursor(0,1);
        PrintDigits(freeMemory(),4);
        lcd.print(" bytes free");
        while (Keypress() != BACKKEY) {}; // Delay for Back key
        lcd.clear();
        lcd.print("    (c) 2013    ");
        lcd.setCursor(0,1);
        lcd.print("Antonis Maglaras");
        delay(100);
        while (Keypress() != BACKKEY) {}; // Delay for Back key     
        lcd.clear();
        StandBy=true;
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 7:
        FactoryReset();
        lcd.clear();
        StandBy=true;
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;       
    }
  }  
}




// --- MAIN MENU procedure ---------------------------------------------------------------------------------------------------------------------------
// Display and move at Main Menu.

void MainMenu()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Select Mode     ");
  lcd.setCursor(0,1);
  MenuSelection=1;
  StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(MenuItems[MenuSelection]);
    if (Keypress() == RIGHTKEY)
    {
      MenuSelection+=1;
      if (MenuSelection>7)
        MenuSelection=1;
    }
    if (Keypress() == LEFTKEY)
    {
      MenuSelection-=1;
      if (MenuSelection<1)
        MenuSelection=7;
    }
    if (Keypress() == ENTERKEY)
    {
      StandBy=false;
      StayInside=false;
      Mode=MenuSelection;  
    }
    if (Keypress() == BACKKEY)
    {
      Mode=MenuSelection;
      lcd.clear();
      lcd.print(" Camera Trigger ");
      StayInside=false;
      ResetTimeVars();
      return; 
    }
  }
  lcd.clear();
  lcd.print(" Camera Trigger ");
  ResetTimeVars();
  delay(10);
}




// --- RETURN KEYPRESS function ----------------------------------------------------------------------------------------------------------------------
// Check for keypress and return key.

int Keypress()
{
  if (digitalRead(LeftButton) == LOW)
  {
    delay(30);
    return LEFTKEY;
  }
  else
    if (digitalRead(RightButton) == LOW)
    {
      delay(30);
      return RIGHTKEY;
    }
    else
      if (digitalRead(EnterButton) == LOW)
      {
        delay(30);
        return ENTERKEY;
      }
      else
        if (digitalRead(BackButton) == LOW)
        {
          delay(30);
          return BACKKEY;
        }
        else
          return NONEKEY;
}




// --- PRINT DIGITS (1, 2, 3 or 4 digits) procedure --------------------------------------------------------------------------------------------------
// Print digits using leading zeros.

void PrintDigits(int x, int y)
{
  if (y>3)
    if (x<1000)
      lcd.print("0");
  if (y>2)
    if (x<100)
      lcd.print("0");
  if (y>1)
    if (x<10)
      lcd.print("0");
  lcd.print(x);
}




// --- BEEP procedure --------------------------------------------------------------------------------------------------------------------------------
// Make a beep!

void Beep(int x)
{
  for (int j=0; j<x; j++)
  {
    digitalWrite(BuzzerPin, HIGH);
    delay(50);
    digitalWrite(BuzzerPin, LOW);
    delay(50);
  }
}




// --- DISPLAYTIME procedure -------------------------------------------------------------------------------------------------------------------------
// Display current time/date

void DisplayTime()
{
  lcd.setCursor(0,1);
  lcd.print(" ");
  if (hour()!=CurrentHour)
  {
    PrintDigits(hour(),2);
    CurrentHour=hour();
  }
  lcd.setCursor(3,1);
  lcd.print(":");
  if (minute()!=CurrentMin)
  {
    PrintDigits(minute(),2);
    CurrentMin=minute();
  }
  lcd.setCursor(6,1);
  lcd.print(":");
  if (second()!=CurrentSec)
  {
    PrintDigits(second(),2);
    CurrentSec=second();
  }
  lcd.setCursor(9,1);
  lcd.print(" ");
  if (day()!=CurrentDay)
  {
    PrintDigits(day(),2);
    CurrentDay=day();
  }
  lcd.setCursor(12,1);
  lcd.print("/");
  if (month()!=CurrentMonth)
  {
    PrintDigits(month(),2);
    CurrentMonth=month();
  }
  lcd.setCursor(15,1);
  lcd.print(" ");
}




// --- RESETTIMEVARS procedure -----------------------------------------------------------------------------------------------------------------------
// Reset time vars. Needed to run before displaying time for first time.

void ResetTimeVars()
{
  CurrentHour=99;
  CurrentMin=99;
  CurrentSec=99;
  CurrentYear=99;
  CurrentMonth=99;
  CurrentDay=99;
}



// --- SETUP TIME procedure --------------------------------------------------------------------------------------------------------------------------
// Setup the time and save the changes to RealTimeClock.

void SetupTime()
{
  lcd.clear();
  lcd.print("Change Hour");
  lcd.setCursor(0,1);
  lcd.print("Hour: ");
  StayInside=true;
  CurrentHour=hour();
  while (StayInside)
  {
    lcd.setCursor(6,1);
    PrintDigits(CurrentHour,2);
    if (Keypress() == RIGHTKEY)
    {
      CurrentHour+=1;
      if (CurrentHour>23)
        CurrentHour=0;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentHour-=1;
      if (CurrentHour<0)
        CurrentHour=23;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(CurrentHour,minute(), second(), day(), month(), year());
      RTC.set(now());
      StayInside=false;
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress() == BACKKEY)
    {
      StayInside=false;
      if (MakeSounds)
        Beep(2);
    }
  }
  lcd.clear();
  lcd.print("Change Minutes");
  lcd.setCursor(0,1);
  lcd.print("Minutes: ");
  StayInside=true;
  CurrentMin=minute();
  while (StayInside)
  {
    lcd.setCursor(9,1);
    PrintDigits(CurrentMin,2);
    if (Keypress() == RIGHTKEY)
    {
      CurrentMin+=1;
      if (CurrentMin>59)
        CurrentMin=0;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentMin-=1;
      if (CurrentMin<0)
        CurrentMin=59;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(hour(),CurrentMin, second(), day(), month(), year());
      RTC.set(now());
      StayInside=false;
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress() == BACKKEY)
    {
      if (MakeSounds)
        Beep(2);
      StayInside=false;
    }
  }
  lcd.clear();
  lcd.print("Change Seconds");
  lcd.setCursor(0,1);
  lcd.print("Seconds: ");
  StayInside=true;
  CurrentSec=second();
  while (StayInside)
  {
    lcd.setCursor(9,1);
    PrintDigits(CurrentSec,2);
    if (Keypress() == RIGHTKEY)
    {
      CurrentSec+=1;
      if (CurrentSec>59)
        CurrentSec=0;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentSec-=1;
      if (CurrentSec<0)
        CurrentSec=59;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(hour(),minute(), CurrentSec, day(), month(), year());
      StayInside=false;
      if (MakeSounds)
        Beep(1);
      RTC.set(now());
    }
    if (Keypress() == BACKKEY)
    {
      if (MakeSounds)
        Beep(2);
      StayInside=false;
    }
  }
  lcd.clear();
  lcd.print("Change Day");
  lcd.setCursor(0,1);
  lcd.print("Day: ");
  StayInside=true;
  CurrentDay=day();
  while (StayInside)
  {
    lcd.setCursor(5,1);
    PrintDigits(CurrentDay,2);
    if (Keypress() == RIGHTKEY)
    {
      CurrentDay+=1;
      if (CurrentDay>31)
        CurrentDay=1;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentDay-=1;
      if (CurrentDay<1)
        CurrentDay=31;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(hour(),minute(), second(), CurrentDay, month(), year());
      StayInside=false;
      RTC.set(now());
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress() == BACKKEY)
    {
      StayInside=false;
      if (MakeSounds)
        Beep(2);
    }
  }
  lcd.clear();
  lcd.print("Change Month");
  lcd.setCursor(0,1);
  lcd.print("Month: ");
  StayInside=true;
  CurrentMonth=month();
  while (StayInside)
  {
    lcd.setCursor(7,1);
    PrintDigits(CurrentMonth,2);
    if (Keypress() == RIGHTKEY)
    {
      CurrentMonth+=1;
      if (CurrentMonth>12)
        CurrentMonth=1;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentMonth-=1;
      if (CurrentMonth<1)
        CurrentMonth=12;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(hour(),minute(), second(), day(), CurrentMonth, year());
      RTC.set(now());
      StayInside=false;
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress() == BACKKEY)
    {
      StayInside=false;
      if (MakeSounds)
        Beep(2);
    }
  }
  lcd.clear();
  lcd.print("Change Year");
  lcd.setCursor(0,1);
  lcd.print("Year: ");
  StayInside=true;
  CurrentYear=year();
  while (StayInside)
  {
    lcd.setCursor(6,1);
    PrintDigits(CurrentYear,4);
    if (Keypress() == RIGHTKEY)
    {
      CurrentYear+=1;
      if (CurrentYear>2030)
        CurrentYear=2013;
    }
    if (Keypress() == LEFTKEY)
    {
      CurrentYear-=1;
      if (CurrentYear<2013)
        CurrentYear=2020;
    }
    if (Keypress() == ENTERKEY)
    {
      setTime(hour(),minute(), second(), day(), month(), CurrentYear);
      RTC.set(now());
      StayInside=false;
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress() == BACKKEY)
    {
      setSyncProvider(RTC.get);
      StayInside=false;
      if (MakeSounds)
        Beep(2);
    }
  }
}




// --- SETUP INFRARED/PreFocus TRIGGER procedure --------------------------------------------------------------------------------------------------------
// Setup the trigger type: PreFocus (by using 2 optocouplers) or by Infrared command or both.

void SetupMenu()
{
  lcd.clear();
  lcd.print("Pre Focus       ");
  lcd.setCursor(0,1);
  StayInside=true;
  boolean tmpPreFocus=PreFocus;
  while (StayInside)
  {
    if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
      tmpPreFocus=!(tmpPreFocus);
    lcd.setCursor(0,1);
    if (tmpPreFocus)
      lcd.print("Enabled ");
    else
      lcd.print("Disabled");
    if (Keypress() == ENTERKEY)
    {
      PreFocus=tmpPreFocus;
      if (PreFocus)
        WriteToMem(12,1);
      else
        WriteToMem(12,0);     
     SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("Wired Triggers  ");
  lcd.setCursor(0,1);
  StayInside=true;
  int tmpOptocouplersStatus = OptocouplersStatus;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpOptocouplersStatus-=1;
      if (tmpOptocouplersStatus<0)
        tmpOptocouplersStatus=3;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpOptocouplersStatus+=1;
      if (tmpOptocouplersStatus>3)
        tmpOptocouplersStatus=0;
    }
    lcd.setCursor(0,1);
    switch (tmpOptocouplersStatus)
    {
      case 0:     
        lcd.print("Both Active     ");
        break;
      case 1:
        lcd.print("First Only      ");
        break;
      case 2:
        lcd.print("Second Only     ");
        break;
      case 3:
        lcd.print("Both NOT Active ");
        break;
    }
    if (Keypress() == ENTERKEY)
    {
      OptocouplersStatus = tmpOptocouplersStatus;
      switch (OptocouplersStatus)
      {
        case 0:
          Optocoupler1Enabled=true;
          Optocoupler2Enabled=true;
          break;
        case 1:
          Optocoupler1Enabled=true;
          Optocoupler2Enabled=false;
          break;
        case 2:
          Optocoupler1Enabled=false;
          Optocoupler2Enabled=true;
          break;
      }
      WriteToMem(16,OptocouplersStatus);    
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("Infrared Trigger");
  lcd.setCursor(0,1);
  StayInside=true;
  boolean tmpInfrared=Infrared;
  while (StayInside)
  {
    if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
      tmpInfrared = !(tmpInfrared);
    lcd.setCursor(0,1);
    if (tmpInfrared)
      lcd.print("Enabled ");
    else
      lcd.print("Disabled");
    if (Keypress() == ENTERKEY)
    {
      Infrared=tmpInfrared;
      if (Infrared)
        WriteToMem(10,1);
      else
        WriteToMem(10,0);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("Pre Shot Delay  ");
  lcd.setCursor(0,1);
  lcd.print("Millis:         ");
  StayInside=true;
  int tmpPreDelay=PreDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpPreDelay-=1;
      if (tmpPreDelay<0)
        tmpPreDelay=3000;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpPreDelay+=1;
      if (tmpPreDelay>3000)
        tmpPreDelay=0;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpPreDelay,4);
    if (Keypress() == ENTERKEY)
    {
      PreDelay=tmpPreDelay;
      WriteToMem(4,PreDelay);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  } 
  StayInside=true;
  lcd.clear();
  lcd.print("Shutter Delay");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  int tmpShutterDelay=ShutterDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpShutterDelay-=1;
      if (tmpShutterDelay<0)
        tmpShutterDelay=3000;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpShutterDelay+=1;
      if (tmpShutterDelay>3000)
        tmpShutterDelay=0;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpShutterDelay,4);
    if (Keypress() == ENTERKEY)
    {
      ShutterDelay=tmpShutterDelay;
      WriteToMem(6,ShutterDelay);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("After Shot Delay");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  int tmpAfterDelay=AfterDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpAfterDelay-=1;
      if (tmpAfterDelay<0)
        tmpAfterDelay=3000;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpAfterDelay+=1;
      if (tmpAfterDelay>3000)
        tmpAfterDelay=0;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpAfterDelay,4);
    if (Keypress() == ENTERKEY)
    {
      AfterDelay=tmpAfterDelay;
      WriteToMem(8,AfterDelay);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
    SettingsNotSaved();
  }
  StayInside=true;
  int ShortCutTemp=ShortCut;
  lcd.clear();
  lcd.print("Shortcut        ");
  lcd.setCursor(0,1);
  lcd.print(MenuItems[ShortCutTemp]);
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      ShortCutTemp-=1;
      if (ShortCutTemp<1)
        ShortCutTemp=4;
    }
    if (Keypress() == RIGHTKEY)
    {
      ShortCutTemp+=1;
      if (ShortCutTemp>4)
        ShortCutTemp=1;
    }
    lcd.setCursor(0,1);
    lcd.print(MenuItems[ShortCutTemp]);
    if (Keypress() == ENTERKEY)
    {
      ShortCut=ShortCutTemp;
      WriteToMem(14,ShortCut);         
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("LCD Backlight   ");    
  StayInside=true;  
  lcd.setCursor(0,1);
  boolean BacklightTemp=BackLight;
  while (StayInside)
  {
    if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
      BacklightTemp=!(BacklightTemp);
    lcd.setCursor(0,1);
    if (BacklightTemp)
      lcd.print("Enabled ");
    else
      lcd.print("Disabled");
    if (Keypress() == ENTERKEY)
    {
      BackLight=BacklightTemp;
      if (BackLight)
      {
        WriteToMem(2,1);
        lcd.backlight();
      }
      else
      {
        WriteToMem(2,0);
        lcd.noBacklight();
      }
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }  
  lcd.clear();
  lcd.print("Buzzer          ");
  lcd.setCursor(0,1);
  StayInside=true;
  boolean MakeSoundsTemp = MakeSounds;
  while (StayInside)
  {
    if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
      MakeSoundsTemp=!(MakeSoundsTemp);
    lcd.setCursor(0,1);
    if (MakeSoundsTemp)
      lcd.print("Enabled ");
    else
      lcd.print("Disabled");
    if (Keypress() == ENTERKEY)
    {
      MakeSounds=MakeSoundsTemp;
      if (MakeSounds)
        WriteToMem(0,1);
      else
        WriteToMem(0,0);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }  
  lcd.clear();
  lcd.print("Setup Time?");
  StayInside=true;
  while (StayInside)
  {
    if (Keypress() == ENTERKEY)
    {
      if (MakeSounds)
        Beep(1);
      SetupTime();
      StayInside=false;
    }
    if (Keypress() == BACKKEY)
    {
      StayInside=false;
      if (MakeSounds)
        Beep(2);
    }
  }
  Mode=1;
  StandBy=true;
}




// --- WRITE TO EEPROM procedure ---------------------------------------------------------------------------------------------------------------------
// Write numbers (0-65535) to EEPROM (using 2 bytes).

void WriteToMem(byte address, int number)
{
  int a = number/256;
  int b = number % 256;
  EEPROM.write(address,a);
  EEPROM.write(address+1,b);
}




// --- READ FRON EEPROM procedure --------------------------------------------------------------------------------------------------------------------
// Read numbers (0-65535) from EEPROM (using 2 bytes).

int ReadFromMem(byte address)
{
  int a=EEPROM.read(address);
  int b=EEPROM.read(address+1);

  return a*256+b;
}




// --- PREFOCUS START procedure ----------------------------------------------------------------------------------------------------------------------
// Start by keeping the focus HIGH.

void PreFocusStart()
{
  digitalWrite(Optocoupler1Pin, HIGH);
}




// --- PREFOCUS STOP procedure -----------------------------------------------------------------------------------------------------------------------
// Start by keeping the focus HIGH.

void PreFocusStop()
{
  digitalWrite(Optocoupler1Pin, LOW);
}




// --- TRIGGER procedure -----------------------------------------------------------------------------------------------------------------------------
// Trigger the camera.

void Trigger()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if (!PreFocus)
    if (Optocoupler1Enabled)
      digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  if (Infrared)
    Camera.shutterNow();
  delay(ShutterDelay);  
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  delay(AfterDelay);
  if (MakeSounds)
    Beep(3);
}




// --- BULB START procedure --------------------------------------------------------------------------------------------------------------------------
// Start bulb mode.

void StartBulb()
{
  digitalWrite(Optocoupler1Pin, HIGH);
  digitalWrite(Optocoupler2Pin, HIGH);
  if (MakeSounds)
    Beep(1);
}



// --- BULB STOP procedure ---------------------------------------------------------------------------------------------------------------------------
// Stop bulb mode.

void StopBulb()
{
  digitalWrite(Optocoupler1Pin, LOW);
  digitalWrite(Optocoupler2Pin, LOW);
  delay(AfterDelay);
  if (MakeSounds)
    Beep(2);
}




// --- BACKKEY function ------------------------------------------------------------------------------------------------------------------------------
// For faster checking for back keypress.

boolean Backkey()
{
  if (digitalRead(BackButton)==LOW)
    return true;
  else
    return false;
}




// --- FACTORY RESET procedure -----------------------------------------------------------------------------------------------------------------------
// Factory reset & reboot.                  

void FactoryReset()
{
  lcd.clear();
  lcd.print("FACTORY RESET!  ");
  lcd.setCursor(0,1);
  lcd.print("Are you sure?   ");
  Beep(1);
  boolean StayInside=true;
  while (StayInside)
  {
    if (Keypress() == ENTERKEY)
    {
      delay(100);
      lcd.setCursor(0,1);
      lcd.print("Confirm?        ");
      Beep(1);
      boolean StayInside2 = true;
      while (StayInside2)
      {
        if (Keypress() == ENTERKEY)
        {
          for (int i = 0; i < 512; i++)
            EEPROM.write(i, 0);      
          lcd.setCursor(0,1);
          lcd.print("Done!           ");
          WriteToMem(0,1);
          WriteToMem(2,1);
          WriteToMem(4,0);
          WriteToMem(6,250);
          WriteToMem(8,250);
          WriteToMem(10,1);
          WriteToMem(12,1);  
          WriteToMem(14,1);
          WriteToMem(16,1);
          Beep(3);
          delay(1000);
          SoftReset();
        }
        if (Keypress() == BACKKEY)
        {
          StayInside=false;
          StayInside2=false;
        }      
      }
    }
    if (Keypress() == BACKKEY)
    {
      StayInside=false;
      ResetTimeVars();
      return;
    }
  }
  SoftReset();
}




// --- SOFTWARE RESET ARDUINO procedure --------------------------------------------------------------------------------------------------------------
// Reset Arduino.

void SoftReset()
{
  asm volatile ("  jmp 0");
}




// --- SAVE SETTINGS MSG procedure -------------------------------------------------------------------------------------------------------------------
// Make sound and display confirm/save message.

void SettingsSaved()
{
  lcd.setCursor(0,1);
  lcd.print("Saved!          ");
  StayInside=false;
  if (MakeSounds)
    Beep(1);
  delay(500); 
}




// --- NOT SAVED SETTINGS MSG procedure --------------------------------------------------------------------------------------------------------------
// Make sound and display not saved message.

void SettingsNotSaved()
{
  lcd.setCursor(0,1);
  lcd.print("Not Saved!      ");
  StayInside=false;
  if (MakeSounds)
    Beep(2);
  delay(500);     
}
