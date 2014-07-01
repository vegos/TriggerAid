// --- TriggerAid (former (new) CAMERA TRIGGER) ------------------------------------------------------------------------------------------------------
//
//     (c)2013, Antonis Maglaras
//     maglaras at gmail dot com
//     http://www.slr.gr 
//
//     More information: http://vegos.github.io/TriggerAid & http://github.com/vegos/TriggerAid/wiki
//     Latest source code available at: https://github.com/vegos/TriggerAid
//     Photos etc at: http://www.slr.gr/trigger
//     
// ---------------------------------------------------------------------------------------------------------------------------------------------------




// Libraries used in this sketch          -- Many thanks to the creators of the following libraries!
#include <LiquidCrystal.h>                // Arduino included Library
#include <MemoryFree.h>                   // http://playground.arduino.cc/Code/AvailableMemory
#include <EEPROM.h>                       // Arduino included Library
#include <multiCameraIrControl.h>         // http://sebastian.setz.name/arduino/my-libraries/multi-camera-ir-control/

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define  Version         "2.0.0"          // Current Version


// Create two (2) custom characters (for visual identification of armed/disarmed mode)
byte ONChar[8] =  { B00000, B01110, B11111, B11111, B11111, B01110, B00000, B00000 };
byte OFFChar[8] = { B00000, B01110, B10001, B10001, B10001, B01110, B00000, B00000 };



// Translate button names to numbers for easier recognition
#define  LEFTKEY            1
#define  RIGHTKEY           2
#define  ENTERKEY           3
#define  BACKKEY            4
#define  SHOOTKEY           5
#define  NONEKEY            0

// Definition of pins used by Arduino
#define  IRLedPin          10      // For IR transmitting
#define  LightSensorPin    A1      // Built-in Light trigger

#define  LEDPin            A5      // LED Pin
#define  ExternalPin        8      // For connecting external (digital) sensors
#define  BuzzerPin         A0      // Buzzer -- I used a PWM pin. Maybe in next version will play some music :)

#define  RightButton       A3      // Button Right Pin
#define  BackButton         6      // Buton Back Pin
#define  LeftButton        A4      // Button Left Pin
#define  EnterButton       A2      // Button Enter Pin
#define  ShootButton        7      // Button Shoot Pin

#define  Optocoupler1Pin   13      // Optocoupler 1 is used for focus triggering    \____ Olympus E-3 (the camera that I make the testing
#define  Optocoupler2Pin    9      // Optocoupler 2 is used for shutter triggering  /     needs focus & shutter to be triggered together (at least).
                                   //                                                     With that setup, it's possible to trigger two (2) external
                                   //                                                     flashes for example, or two cameras etc.

//Olympus IR remote trigger
Olympus OlympusCamera(IRLedPin);
Pentax PentaxCamera(IRLedPin);
Nikon NikonCamera(IRLedPin);
Sony SonyCamera(IRLedPin);
Canon CanonCamera(IRLedPin);


// Main Menu Strings
char* MenuItems[9] = { "",
                       "Light Trigger   ",    // Mode 1: Built-in light trigger
                       "External Trigger",    // Mode 2: External Trigger (works with digital modules, like a sound or light module.
                       "Time Lapse      ",    // Mode 3: Time lapse. Exposure is available on setup menu (for use with Bulb mode, only on wired triggers).
                       "Bulb Mode       ",    // Mode 4: Bulb mode (My camera can't keep the shutter for more than 60 secs, so that way I can go up to 8 minutes!
                       "High Speed Burst",    // Mode 5: High Speed Burst mode (For flash units, as I guess that there are no cameras so fast).
                       "Setup Parameters",    // Mode 6: Setup System (Delays, Buzzer, Triggers)
                       "Information     ",    // Mode 7: Version information, memory free, etc.
                       "Factory Reset   ",    // Mode 8: Factory Reset.
                      };


// Definition of global variables
int CameraBrand = 1;
int MenuSelection = 1;
int Mode = 1, ShortCut, OptocouplersStatus;
int LightThreshold = 0, Light = 0;
boolean StandBy = true;
boolean WhenHigh = false;
boolean Armed = false;
boolean MakeSounds, PreFocus, Optocoupler1Enabled, Optocoupler2Enabled, BuiltInLightTrigger;
boolean StayInside = false;
int PreDelay, ShutterDelay, AfterDelay, HighSpeedDelay, Limit, LimitTimes, TimeLapseExposure;
long tmpDelay=60;
long StartMillis, tmpMillis, TriggerMillis;




// --- SETUP procedure -------------------------------------------------------------------------------------------------------------------------------
// Create custom characters, display a welcome message, setup pins, read and set system variables etc.

void setup()
{
  lcd.begin(16,2);
  lcd.createChar(1, ONChar);
  lcd.createChar(2, OFFChar);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   TriggerAid   ");
  lcd.setCursor(0,1);
  lcd.print("Antonis Maglaras");
  delay(1000);
  // Setup pins
  pinMode(RightButton, INPUT_PULLUP);  
  pinMode(BackButton, INPUT_PULLUP);
  pinMode(LeftButton, INPUT_PULLUP);
  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(ShootButton, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(ExternalPin, INPUT);
  pinMode(Optocoupler1Pin, OUTPUT);
  pinMode(Optocoupler2Pin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  // Read settings from EEPROM
  if (ReadFromMem(0)==1)
    MakeSounds=true;
  else
    MakeSounds=false;
  PreDelay=ReadFromMem(4);
  ShutterDelay=ReadFromMem(6);
  AfterDelay=ReadFromMem(8);
  CameraBrand = ReadFromMem(10);  
  if (ReadFromMem(12)==1)
    PreFocus=true;
  else
    PreFocus=false;
  ShortCut=ReadFromMem(14);
  OptocouplersStatus=ReadFromMem(16);
  HighSpeedDelay=ReadFromMem(18);
  LimitTimes=ReadFromMem(20);
  if (ReadFromMem(22)==1)
    BuiltInLightTrigger=true;
  else
    BuiltInLightTrigger=false;
  TimeLapseExposure=ReadFromMem(24);
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
  StandBy=true;
  if (MakeSounds)
    Beep(1);
  lcd.setCursor(0,1);
  lcd.print("Ready!          ");    
}




// --- MAIN procedure --------------------------------------------------------------------------------------------------------------------------------
// Check for system status (armed/disarmed), check for mode selection, display time.

void loop()
{
  if (StandBy)
  {
    // On SHOOT key press, start bulb shooting
    if (Keypress() == SHOOTKEY)
    {
      lcd.setCursor(0,1);
      lcd.print("Bulb Shooting...");
      StartBulb();
      while (Keypress() == SHOOTKEY);
      StopBulb();
      lcd.setCursor(0,1);
      lcd.print("Ready!          ");    
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
            return;
          }
        }
      }
    }
    // On ENTER key press, enter the main menu
    if (Keypress() == ENTERKEY)
      MainMenu();
  }
  else
  {
    switch (Mode)
    {
      case 1:
        lcd.clear();
        lcd.print("Light:         ");
        lcd.setCursor(0,1);
        lcd.print("Threshold:     ");
        Light = map(analogRead(LightSensorPin),0,1023,0,100);
        lcd.setCursor(15,1);
        if (BuiltInLightTrigger)
        {
          lcd.print("H");
          LightThreshold = Light+10;
          if (LightThreshold>100)
            LightThreshold=100;
        }
        else
        {
          lcd.print("L");
          LightThreshold = Light-10;
          if (LightThreshold<1)
            LightThreshold=1;
        }
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
              lcd.setCursor(0,0);
              lcd.print("Light Trigger  ");
              lcd.setCursor(0,1);
              lcd.print("Enabled!        ");
              lcd.setCursor(15,0);
              lcd.write(1);
              if (PreFocus)
                PreFocusStart();
              while (!Backkey())
              {
                Light = map(analogRead(LightSensorPin),0,1023,0,100);
                if ((Light>LightThreshold) && (BuiltInLightTrigger))
                {
                  TriggerMillis=millis();
                  Trigger();
                  if (PreFocus)
                    PreFocusStart();
                }
                if ((Light<LightThreshold) && !(BuiltInLightTrigger))
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
              lcd.setCursor(0,0);
              lcd.print("Light:         ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:     ");
              lcd.setCursor(15,1);
              if (BuiltInLightTrigger)
                lcd.print("H");
              else
                lcd.print("L");              
              lcd.setCursor(15,0);
              lcd.write(2);              
              delay(100);
            }
            else
            {
              lcd.setCursor(0,0);
              lcd.print("Light:         ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:     ");
              lcd.setCursor(15,1);
              if (BuiltInLightTrigger)
                lcd.print("H");
              else
                lcd.print("L");                            
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
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");    
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
              lcd.print("Enabled!        ");
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
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");        
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
            if (PreFocus)
              PreFocusStart();
            if ((millis()-StartMillis)>=(tmpDelay*1000))
            {
              TriggerTimeLapse();
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
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");        
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
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");    
        break;
      case 5:
        lcd.clear();
        lcd.print("HighSpeed Burst ");
        lcd.setCursor(0,1);
        lcd.print("Interval:     ms");
        tmpDelay=50;
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;  
        Limit = 1;
        while (Keypress() != BACKKEY)
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(15,0);
              lcd.write(1);
              lcd.setCursor(0,1);
              lcd.print("Shooting!       ");
              StartMillis=millis();
              StayInside=true;
              Limit=1;
              while (StayInside)
              {
                if (millis()-StartMillis>tmpDelay)
                {
                  HighSpeedTrigger();
                  Limit+=1;
                  if (Limit>LimitTimes)
                  {
                    lcd.setCursor(15,0);
                    lcd.write(2);
                    StayInside=false;
                    Armed=false;
                    lcd.setCursor(0,1);
                    lcd.print("Interval:     ms");
                  }
                  StartMillis=millis();
                }
                if (digitalRead(BackButton)==LOW)
                {
                  lcd.setCursor(15,0);
                  lcd.write(2);
                  StayInside=false;
                  Armed=false;
                  lcd.setCursor(0,1);
                  lcd.print("Interval:     ms");
                }
              }
            }
            else
            {
              lcd.setCursor(15,0);
              lcd.write(2);
              lcd.setCursor(0,1);
              lcd.print("Interval:     ms");
            }
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
            if (tmpDelay>500)
              tmpDelay=500;
          }
          lcd.setCursor(10,1);
          PrintDigits(tmpDelay,3);         
        }
        StandBy=true;
        lcd.clear();
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");    
        break;
      case 6:
        SetupMenu();
        lcd.clear();
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");    
        break;
      case 7:
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
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");        
        break;
      case 8:
        FactoryReset();
        lcd.clear();
        StandBy=true;
        lcd.print("   TriggerAid   ");
        lcd.setCursor(0,1);
        lcd.print("Ready!          ");        
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
      if (MenuSelection>8)
        MenuSelection=1;
    }
    if (Keypress() == LEFTKEY)
    {
      MenuSelection-=1;
      if (MenuSelection<1)
        MenuSelection=8;
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
      lcd.print("   TriggerAid   ");
      lcd.setCursor(0,1);
      lcd.print("Ready!          ");    
      StayInside=false;
      return; 
    }
  }
  lcd.clear();
  lcd.print("   TriggerAid   ");
  lcd.setCursor(0,1);
  lcd.print("Ready!          ");        
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
          if (digitalRead(ShootButton) == LOW)
          {
            return SHOOTKEY;
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
  int tmpCameraBrand=CameraBrand;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpCameraBrand-=1;
      if (tmpCameraBrand<0)
        tmpCameraBrand=5;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpCameraBrand+=1;
      if (tmpCameraBrand>5)
        tmpCameraBrand=0;
    }
    lcd.setCursor(0,1);
    switch (tmpCameraBrand)
    {
      case 0: // None
        lcd.print("Disabled");
        break;
      case 1: // Olympus
        lcd.print("Olympus ");
        break;
      case 2: // Pentax
        lcd.print("Pentax  ");
        break;
      case 3:
        lcd.print("Canon   ");
        break;
      case 4: // Nikon
        lcd.print("Nikon   ");
        break;
      case 5: // Sony
        lcd.print("Sony    ");
        break;
    }
    if (Keypress() == ENTERKEY)
    {
      CameraBrand=tmpCameraBrand;
      WriteToMem(10,CameraBrand);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("Light Trigger  ");
  lcd.setCursor(0,1);
  lcd.print("Trigger on     ");
  boolean tmpBuiltInLightTrigger=BuiltInLightTrigger;
  while (StayInside)
  {
    lcd.setCursor(11,1);
    if (tmpBuiltInLightTrigger)
      lcd.print("HIGH");
    else
      lcd.print("LOW ");
    if ((Keypress() == LEFTKEY) != (Keypress() == RIGHTKEY))
      tmpBuiltInLightTrigger = !(tmpBuiltInLightTrigger);
    if (Keypress() == ENTERKEY)
    {
      BuiltInLightTrigger=tmpBuiltInLightTrigger;
      if (BuiltInLightTrigger)
        WriteToMem(22,1);
      else
        WriteToMem(22,0);
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
  lcd.clear();
  lcd.print("Timelapse Exposr");
  lcd.setCursor(0,1);
  lcd.print("Seconds: ");
  int tmpTimeLapseExposure=TimeLapseExposure;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpTimeLapseExposure-=1;
      if (tmpTimeLapseExposure<0)
        tmpTimeLapseExposure=300;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpTimeLapseExposure+=1;
      if (tmpTimeLapseExposure>300)
        tmpTimeLapseExposure=0;
    }
    lcd.setCursor(9,1);
    PrintDigits(tmpTimeLapseExposure,3);
    if (Keypress() == ENTERKEY)
    {
      TimeLapseExposure=tmpTimeLapseExposure;
      WriteToMem(24,TimeLapseExposure);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
    SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("HighSpeed Delay ");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  int tmpHighSpeedDelay=HighSpeedDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpHighSpeedDelay-=1;
      if (tmpHighSpeedDelay<0)
        tmpHighSpeedDelay=100;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpHighSpeedDelay+=1;
      if (tmpHighSpeedDelay>100)
        tmpHighSpeedDelay=1;
    }
    lcd.setCursor(7,1);
    PrintDigits(tmpHighSpeedDelay,3);
    if (Keypress() == ENTERKEY)
    {
      HighSpeedDelay=tmpHighSpeedDelay;
      WriteToMem(18,HighSpeedDelay);
      SettingsSaved();
    }
    if (Keypress() == BACKKEY)
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("HighSpeed Limit");
  lcd.setCursor(0,1);
  lcd.print("Times: ");
  int tmpLimitTimes=LimitTimes;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpLimitTimes-=1;
      if (tmpLimitTimes<0)
        tmpLimitTimes=0;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpLimitTimes+=1;
      if (tmpLimitTimes>50)
        tmpLimitTimes=50;
    }
    lcd.setCursor(7,1);
    PrintDigits(tmpLimitTimes,2);
    if (Keypress() == ENTERKEY)
    {
      LimitTimes=tmpLimitTimes;
      WriteToMem(20,LimitTimes);
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
        ShortCutTemp=5;
    }
    if (Keypress() == RIGHTKEY)
    {
      ShortCutTemp+=1;
      if (ShortCutTemp>5)
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
  digitalWrite(LEDPin, HIGH);
  if (CameraBrand != 0)
  {
    switch (CameraBrand)
    {
      case 1: // Olympus
        OlympusCamera.shutterNow();
        break;
      case 2: // Pentax
        PentaxCamera.shutterNow();
        break;
      case 3: // Canon
        CanonCamera.shutterNow();
        break;
      case 4: // Nikon
        NikonCamera.shutterNow();
        break;
      case 5: // Sony
        SonyCamera.shutterNow();
        break;
    }
  }
  delay(ShutterDelay);  
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  delay(AfterDelay);
  if (MakeSounds)
    Beep(3);
  digitalWrite(LEDPin, LOW);    
}



// --- TRIGGER TimeLapse -----------------------------------------------------------------------------------------------------------------------------
// Trigger the camera.

void TriggerTimeLapse()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if (!PreFocus)
    if (Optocoupler1Enabled)
      digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  digitalWrite(LEDPin, HIGH);    
  if (CameraBrand != 0)
  {
    switch (CameraBrand)
    {
      case 1: // Olympus
        OlympusCamera.shutterNow();
        break;
      case 2: // Pentax
        PentaxCamera.shutterNow();
        break;
      case 3: // Canon
        CanonCamera.shutterNow();
        break;
      case 4: // Nikon
        NikonCamera.shutterNow();
        break;
      case 5: // Sony
        SonyCamera.shutterNow();
        break;
    }
  }
  for (int x=0; x<TimeLapseExposure; x++)
    delay(1000);  
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  if (MakeSounds)
    Beep(3);
  digitalWrite(LEDPin, LOW);    
}



// --- HIGH SPEED TRIGGER procedure ------------------------------------------------------------------------------------------------------------------
// High-Speed Trigger (for flashes). Can trigger up to 50 times, on every millisecond. Works best with high speed flash units (due to recharging).


void HighSpeedTrigger()
{
  digitalWrite(LEDPin, HIGH);
  digitalWrite(Optocoupler1Pin, HIGH);
  digitalWrite(Optocoupler2Pin, HIGH);
  delay(HighSpeedDelay);
  digitalWrite(Optocoupler1Pin, LOW);
  digitalWrite(Optocoupler2Pin, LOW);
  digitalWrite(LEDPin, LOW);  
}



// --- BULB START procedure --------------------------------------------------------------------------------------------------------------------------
// Start bulb mode.

void StartBulb()
{
//  OlympusCamera.shutterNow();
  digitalWrite(Optocoupler1Pin, HIGH);
  digitalWrite(Optocoupler2Pin, HIGH);
  digitalWrite(LEDPin, HIGH);
  if (CameraBrand != 0)
  {
    switch (CameraBrand)
    {
      case 1: // Olympus
        OlympusCamera.shutterNow();
        break;
      case 2: // Pentax
        PentaxCamera.shutterNow();
        break;
      case 3: // Canon
        CanonCamera.shutterNow();
        break;
      case 4: // Nikon
        NikonCamera.shutterNow();
        break;
      case 5: // Sony
        SonyCamera.shutterNow();
        break;
    }
  }  
  if (MakeSounds)
    Beep(1);
}



// --- BULB STOP procedure ---------------------------------------------------------------------------------------------------------------------------
// Stop bulb mode.

void StopBulb()
{
  digitalWrite(Optocoupler1Pin, LOW);
  digitalWrite(Optocoupler2Pin, LOW);
  digitalWrite(LEDPin, LOW);
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
          WriteToMem(18,5);
          WriteToMem(20,10);
          WriteToMem(22,1);
          WriteToMem(24,60);
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