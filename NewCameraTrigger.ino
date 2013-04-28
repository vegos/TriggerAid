// --- (New) CAMERA TRIGGER --------------------------------------------------------------------------------------------------------------------------
//     (c)2013, Antonis Maglaras
//     maglaras at gmail dot com
//     http://www.slr.gr 
// ---------------------------------------------------------------------------------------------------------------------------------------------------


#include <Wire.h>                         // Arduino included Library
#include <LiquidCrystal_I2C.h>
#include <DS1307RTC.h>
#include <Time.h>                         // Arduino included Library
#include <MemoryFree.h>
#include <EEPROM.h>                       // Arduino included Library
#include <multiCameraIrControl.h>

LiquidCrystal_I2C lcd(0x20,16,2);

#define  Version         "0.5.1b"          // Current Version


// Create two (2) custom characters (for visual identification of armed/disarmed mode)
byte onchar[8] = { B00000, B01110, B11111, B11111, B11111, B01110, B00000, B00000 };
byte offchar[8] = { B00000, B01110, B10001, B10001, B10001, B01110, B00000, B00000 };

// Translate button names to numbers 
#define  LEFTKEY         1
#define  RIGHTKEY        2
#define  ENTERKEY        3
#define  BACKKEY         4
#define  NONEKEY         0

// Definition of pins used by Arduino
#define  IRLed          10      // For IR transmitting
#define  LightSensor    A1      // Built-in Light trigger

#define  Input1Pin      A0      // For future use / analog senors?
#define  ExternalPin     8      // For connecting external (digital) sensors
#define  BuzzerPin       9      // Buzzer

#define  RightButton     4      // Button Right Pin
#define  BackButton      5      // Buton Back Pin
#define  LeftButton      6      // Button Left Pin
#define  EnterButton     7      // Button Enter Pin

#define  Optocoupler1   11
#define  Optocoupler2   12

// Olympus IR remote trigger
Olympus Camera(IRLed);


// Main Menu Strings
//                     0123456789012345
char* MenuItems[10] = { "",
                        "-> Light Trigger",    // 1
                        "-> Ext. Trigger ",    // 2
                        "-> Time Lapse   ",    // 3
                        "-> Bulb Mode    ",    // 4
                        "-> Set Date/Time",    // 5
                        "-> Sound/Backlit",    // 6
                        "-> Set Delays   ",    // 7
                        "-> PreFocus/IR  ",    // 8
                        "-> Information  ",    // 8
                    };
                    
// Definition of global variables

int MenuSelection = 1;
int CurrentHour = 99, CurrentMin = 99, CurrentSec = 99, CurrentDay = 99, CurrentMonth = 99, CurrentYear = 99;
int Mode = 1;
int LightThreshold = 0, Light = 0;
boolean NotArmed = true;
boolean WhenHigh = true;
boolean Armed = false;
boolean BackLight, MakeSounds, Infrared, PreFocus;
int PreDelay, ShutterDelay, AfterDelay;
long tmpDelay=60;
long StartMillis, tmpMillis, TriggerMillis;

// Debug mode. When true, it outputs information data to serial port
#define    Debug    true



// --- SETUP procedure -------------------------------------------------------------------------------------------------------------------------------
// Create custom characters, display a welcome message, setup pins, read and set system variables etc.

void setup()
{
  if (Debug) 
  { 
    Serial.begin(9600);
    Serial.println("Camera Trigger -- Debug information");
  }
  lcd.init();
  lcd.createChar(1, onchar);
  lcd.createChar(2, offchar);
  lcd.backlight();
  setSyncProvider(RTC.get);
  lcd.clear();
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print(" Camera Trigger ");
  lcd.setCursor(0,1);
  lcd.print("Antonis Maglaras");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("    WELCOME!    ");
  delay(1000);
  // Setup pins
  pinMode(RightButton, INPUT_PULLUP);  
  pinMode(BackButton, INPUT_PULLUP);
  pinMode(LeftButton, INPUT_PULLUP);
  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(ExternalPin, INPUT);
  pinMode(Optocoupler1, OUTPUT);
  pinMode(Optocoupler2, OUTPUT);
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
  //
  if (BackLight)
    lcd.backlight();
  else
    lcd.noBacklight();
  NotArmed=true;
}



// --- MAIN procedure --------------------------------------------------------------------------------------------------------------------------------
// Check for system status (armed/disarmed), check for mode selection, display time.

void loop()
{
  if (NotArmed)
  {
    if (Keypress()==RIGHTKEY)
    {
      BackLight=!(BackLight);
      if (BackLight)
        lcd.backlight();
      else 
        lcd.noBacklight();
    }
    if (Keypress()==LEFTKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Bulb Shooting...");
      StartBulb();
      while (Keypress()==LEFTKEY);
      StopBulb();
      lcd.setCursor(0,1);
      lcd.print("                ");
      ResetTimeVars();
    }
    if (Keypress()==ENTERKEY)
    {
      if (Debug)  Serial.println("MAIN MENU");
      MainMenu();
    }
    DisplayTime();  
  }
  else
  {
    switch (Mode)
    {
      case 1:
        if (Debug)  Serial.println("Case 1: Light Trigger");
        lcd.clear();
        //         0123456789012345
        lcd.print("Light:          ");
        lcd.setCursor(0,1);
        lcd.print("Threshold:      ");
        Light = map(analogRead(LightSensor),0,1023,0,100);
        if (Debug)  Serial.print("Light Reading: ");
        if (Debug)  Serial.println(analogRead(LightSensor));
        LightThreshold = Light+10;
        Armed=false;
        lcd.setCursor(15,0);
        lcd.write(2);
        while (Keypress() != BACKKEY)
        {
          if (Keypress()==ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.clear();
              //         0123456789012345
              lcd.print("Light Trigger   ");
              lcd.setCursor(0,1);
              lcd.print("Ready!          ");
              lcd.setCursor(15,0);
              lcd.write(1);
              if (PreFocus)
                PreFocusStart();
              while (!Backkey())
              {
                Light = map(analogRead(LightSensor),0,1023,0,100);
                if ((Light>LightThreshold) && (Armed))
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
              //         0123456789012345
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
              //         0123456789012345
              lcd.print("Light:          ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:      ");
              lcd.setCursor(15,0);
              lcd.write(2);               
            }
          }
          if (Debug)  Serial.print("Light Reading: ");
          if (Debug)  Serial.println(analogRead(LightSensor));
          Light = map(analogRead(LightSensor),0,1023,0,100);
          lcd.setCursor(7,0);
          PrintDigits(Light,3);
          lcd.setCursor(11,1);
          PrintDigits(LightThreshold,3);
          if (Keypress()==RIGHTKEY)
          {
            LightThreshold+=1;
            if (LightThreshold>100)
              LightThreshold=100;
          }
          if (Keypress()==LEFTKEY)
          {
            LightThreshold-=1;
            if (LightThreshold<0)
              LightThreshold=0;
          }
        }
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 2:
        if (Debug)  Serial.println("Case 2: External Trigger");
        lcd.clear();
        //         0123456789012345
        lcd.print("Ext. Trigger    ");
        lcd.setCursor(0,1);
        lcd.print("Trigger on      ");
        Armed=false;
        lcd.setCursor(15,0);
        lcd.write(2);
        while (Keypress()!=BACKKEY)
        {
          if (Keypress()==ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(0,1);
              //         0123456789012345
              lcd.print("Ready!          ");
              lcd.setCursor(15,0);
              lcd.write(1);
              if (PreFocus)
                PreFocusStart();
              while (!Backkey())
              {
                if ((WhenHigh) && (digitalRead(ExternalPin)==HIGH))
                {
                  TriggerMillis=millis();
                  Trigger();
                  if (PreFocus)
                    PreFocusStart();
                }
                if ((!WhenHigh) && (digitalRead(ExternalPin)==LOW))
                {
                  TriggerMillis=millis();
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
          if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
            WhenHigh=!(WhenHigh);
          if (WhenHigh)
            lcd.print("HIGH");
          else
            lcd.print("LOW ");
        }
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 3:
        if (Debug)  Serial.println("Case 3: Time Lapse");
        lcd.clear();
        lcd.print("TimeLapse [   ] ");
        lcd.setCursor(0,1);
        //         0123456789012345
        lcd.print("Interval: 060   ");
        tmpDelay=60;
        StartMillis=millis();
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;
        while (Keypress()!=BACKKEY)
        {
          if (Keypress()==ENTERKEY)
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
            if ((millis()-StartMillis)>(tmpDelay*1000))
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
          if (Keypress()==LEFTKEY)
          {
            tmpDelay-=1;
            if (tmpDelay<1)
              tmpDelay=1;
          }
          if (Keypress()==RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>360)
              tmpDelay=360;
          }
          lcd.setCursor(10,1);
          PrintDigits(tmpDelay,3);
        }
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 4:
        if (Debug)  Serial.println("Case 4: Bulb Mode");
        lcd.clear();
        lcd.print("Bulb Mode [   ] ");
        lcd.setCursor(0,1);
        //         0123456789012345
        lcd.print("Bulb: 030 secs  ");
        tmpDelay=30;
        StartMillis=millis();
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;
        while (Keypress()!=BACKKEY)
        {
          if (Keypress()==ENTERKEY)
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
            if ((millis()-StartMillis)>(tmpDelay*1000))
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
          if (Keypress()==LEFTKEY)
          {
            tmpDelay-=1;
            if (tmpDelay<1)
              tmpDelay=1;
          }
          if (Keypress()==RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>360)
              tmpDelay=360;
          }
          lcd.setCursor(6,1);
          PrintDigits(tmpDelay,3);
        }
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 5:
        if (Debug)  Serial.println("Case 5: Setup Time");
        SetupTime();
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 6:
        if (Debug)  Serial.println("Case 6: Setup Interface");
        SetupInterface();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 7:
        if (Debug)  Serial.println("Case 7: Setup Delays");
        SetupDelays();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 8:
        if (Debug)  Serial.println("Case 8: Infrared");
        SetupInfrared();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();       
        break;
      case 9:
        if (Debug)  Serial.println("Case 9: Information");
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
        NotArmed=true;
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
  lcd.print("Change Mode");
  lcd.setCursor(0,1);
  //         0123456789012345
  MenuSelection=1;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(MenuItems[MenuSelection]);
    if (Keypress() == RIGHTKEY)
    {
      MenuSelection+=1;
      if (MenuSelection>9)
        MenuSelection=1;
    }
    if (Keypress() == LEFTKEY)
    {
      MenuSelection-=1;
      if (MenuSelection<1)
        MenuSelection=9;
    }
    if (Keypress() == ENTERKEY)
    {
      NotArmed=false;
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
      return; // epistrofh sto loop
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
  if (digitalRead(LeftButton)==LOW)
  {
    if (Debug)  Serial.println("LEFT");
    delay(50);
    return LEFTKEY;
  }
  if (digitalRead(RightButton)==LOW)
  {
    if (Debug)  Serial.println("RIGHT");
    delay(50);
    return RIGHTKEY;
  }
  if (digitalRead(EnterButton)==LOW)
  {
    if (Debug)  Serial.println("ENTER");
    delay(50);
    return ENTERKEY;
  }
  if (digitalRead(BackButton)==LOW)
  {
    if (Debug)  Serial.println("BACK");
    delay(50);
    return BACKKEY;
  }
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
  lcd.print("Hour: ");
  lcd.setCursor(0,1);
  lcd.print("Change hour");
  boolean StayInside=true;
  CurrentHour=hour();
  while (StayInside)
  {
    lcd.setCursor(6,0);
    PrintDigits(CurrentHour,2);
    if (Keypress()==RIGHTKEY)
    {
      CurrentHour+=1;
      if (CurrentHour>23)
        CurrentHour=0;
    }
    if (Keypress()==LEFTKEY)
    {
      CurrentHour-=1;
      if (CurrentHour<0)
        CurrentHour=23;
    }
    if (Keypress()==ENTERKEY)
    {
      setTime(CurrentHour,minute(), second(), day(), month(), year());
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  lcd.print("Minutes: ");
  lcd.setCursor(0,1);
  lcd.print("Change minutes");
  StayInside=true;
  CurrentMin=minute();
  while (StayInside)
  {
    lcd.setCursor(9,0);
    PrintDigits(CurrentMin,2);
    if (Keypress()==RIGHTKEY)
    {
      CurrentMin+=1;
      if (CurrentMin>59)
        CurrentMin=0;
    }
    if (Keypress()==LEFTKEY)
    {
      CurrentMin-=1;
      if (CurrentMin<0)
        CurrentMin=59;
    }
    if (Keypress()==ENTERKEY)
    {
      setTime(hour(),CurrentMin, second(), day(), month(), year());
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  lcd.print("Seconds: ");
  lcd.setCursor(0,1);
  lcd.print("Reset seconds");
  StayInside=true;
  CurrentSec=second();
  while (StayInside)
  {
    lcd.setCursor(9,0);
    PrintDigits(CurrentSec,2);
    if (Keypress()==RIGHTKEY)
      CurrentSec=0;
    if (Keypress()==LEFTKEY)
      CurrentSec=0;
    if (Keypress()==ENTERKEY)
    {
      setTime(hour(),minute(), 0, day(), month(), year());
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  lcd.print("Day: ");
  lcd.setCursor(0,1);
  lcd.print("Change day");
  StayInside=true;
  CurrentDay=day();
  while (StayInside)
  {
    lcd.setCursor(5,0);
    PrintDigits(CurrentDay,2);
    if (Keypress()==RIGHTKEY)
    {
      CurrentDay+=1;
      if (CurrentDay>31)
        CurrentDay=1;
    }
    if (Keypress()==LEFTKEY)
    {
      CurrentDay-=1;
      if (CurrentDay<1)
        CurrentDay=31;
    }
    if (Keypress()==ENTERKEY)
    {
      setTime(hour(),minute(), second(), CurrentDay, month(), year());
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  lcd.print("Month: ");
  lcd.setCursor(0,1);
  lcd.print("Change month");
  StayInside=true;
  CurrentMonth=month();
  while (StayInside)
  {
    lcd.setCursor(7,0);
    PrintDigits(CurrentMonth,2);
    if (Keypress()==RIGHTKEY)
    {
      CurrentMonth+=1;
      if (CurrentMonth>12)
        CurrentMonth=1;
    }
    if (Keypress()==LEFTKEY)
    {
      CurrentMonth-=1;
      if (CurrentMonth<1)
        CurrentMonth=12;
    }
    if (Keypress()==ENTERKEY)
    {
      setTime(hour(),minute(), second(), day(), CurrentMonth, year());
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  lcd.print("Year: ");
  lcd.setCursor(0,1);
  lcd.print("Change year");
  StayInside=true;
  CurrentYear=year();
  while (StayInside)
  {
    lcd.setCursor(7,0);
    PrintDigits(CurrentYear,4);
    if (Keypress()==RIGHTKEY)
    {
      CurrentYear+=1;
      if (CurrentYear>2030)
        CurrentYear=2013;
    }
    if (Keypress()==LEFTKEY)
    {
      CurrentYear-=1;
      if (CurrentYear<2013)
        CurrentYear=2020;
    }
    if (Keypress()==ENTERKEY)
    {
      lcd.clear();
      setTime(hour(),minute(), second(), day(), month(), CurrentYear);
      RTC.set(now());
      StayInside=false;
      //         0123456789012345
      lcd.print("Datetime saved!");
      if (MakeSounds)
        Beep(1);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.clear();
      setSyncProvider(RTC.get);
      StayInside=false;
      lcd.print("Not saved!");
      if (MakeSounds)
        Beep(2);
    }
  }
  delay(2000);
  Mode=1;
  NotArmed=true;
}



// --- SETUP INFRARED/PreFocus TRIGGER procedure --------------------------------------------------------------------------------------------------------
// Setup the trigger type: PreFocus (by using 2 optocouplers) or by Infrared command or both.

void SetupInfrared()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Pre Focus       ");
  lcd.setCursor(0,1);
  lcd.print("Enabled:        ");
  boolean StayInside=true;
  boolean tmpPreFocus=PreFocus;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      tmpPreFocus=!(tmpPreFocus);
    lcd.setCursor(9,1);
    if (tmpPreFocus)
      lcd.print("YES");
    else
      lcd.print("NO ");
    if (Keypress()==ENTERKEY)
    {
      PreFocus=tmpPreFocus;
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  lcd.clear();
  //         0123456789012345
  lcd.print("Infrared Trigger");
  lcd.setCursor(0,1);
  lcd.print("Enabled:     ");
  StayInside=true;
  boolean tmpInfrared=Infrared;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      tmpInfrared=!(tmpInfrared);
    lcd.setCursor(9,1);
    if (tmpInfrared)
      lcd.print("YES");
    else
      lcd.print("NO ");
    if (Keypress()==ENTERKEY)
    {
      Infrared=tmpInfrared;
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Settings Saved! ");
      if (MakeSounds)
        Beep(1);
      if (Infrared)
        WriteToMem(10,1);
      else
        WriteToMem(10,0);
      if (PreFocus)
        WriteToMem(12,1);
      else
        WriteToMem(12,0);
      StayInside=false;
      delay(1000);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Not Saved!      ");
      StayInside=false;
      if (MakeSounds)
        Beep(2);
      delay(1000);
    }
  }
  Mode=1;
  NotArmed=true;
}



// --- MAIN SETUP procedure --------------------------------------------------------------------------------------------------------------------------
// Main setup procedure. Setting the buzzer and the backlight of the LCD.

void SetupInterface()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Setup Interface ");
  lcd.setCursor(0,1);
  lcd.print("Buzzer:      ");
  boolean StayInside=true;
  boolean MakeSoundsTemp=MakeSounds;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      MakeSoundsTemp=!(MakeSoundsTemp);
    lcd.setCursor(8,1);
    if (MakeSoundsTemp)
      lcd.print("ON ");
    else
      lcd.print("OFF");
    if (Keypress()==ENTERKEY)
    {
      MakeSounds=MakeSoundsTemp;
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  }
  StayInside=true;
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("Backlight:      ");
  boolean BacklightTemp=BackLight;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      BacklightTemp=!(BacklightTemp);
    lcd.setCursor(11,1);
    if (BacklightTemp)
      lcd.print("ON ");
    else
      lcd.print("OFF");
    if (Keypress()==ENTERKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Settings Saved! ");
      BackLight=BacklightTemp;
      StayInside=false;
      if (MakeSounds)
        Beep(1);
      if (MakeSounds)
        WriteToMem(0,1);
      else
        WriteToMem(0,0);
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
      delay(1000);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Not saved!      ");
      if (MakeSounds)
        Beep(2);
      StayInside=false;    
      delay(1000);
    }
  }  
  Mode=1;
  NotArmed=true;
}



// --- SETUP DELAYS procedure ------------------------------------------------------------------------------------------------------------------------
// Setup delays (Pre Shot, Shutter (how much time the shutter will stay open) and After Shot).

void SetupDelays()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Setup Delays    ");
  lcd.setCursor(0,1);
  lcd.print("Pre Delay:      ");
  boolean StayInside=true;
  int tmpPreDelay=PreDelay;
  while (StayInside)
  {
    if (Keypress()==LEFTKEY)
    {
      tmpPreDelay-=1;
      if (tmpPreDelay<0)
        tmpPreDelay=3000;
    }
    if (Keypress()==RIGHTKEY)
    {
      tmpPreDelay+=1;
      if (tmpPreDelay>3000)
        tmpPreDelay=0;
    }
    lcd.setCursor(11,1);
    PrintDigits(tmpPreDelay,4);
    if (Keypress()==ENTERKEY)
    {
      PreDelay=tmpPreDelay;
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;
  } 
  StayInside=true;
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("Shutter:        ");
  int tmpShutterDelay=ShutterDelay;
  while (StayInside)
  {
    if (Keypress()==LEFTKEY)
    {
      tmpShutterDelay-=1;
      if (tmpShutterDelay<0)
        tmpShutterDelay=3000;
    }
    if (Keypress()==RIGHTKEY)
    {
      tmpShutterDelay+=1;
      if (tmpShutterDelay>3000)
        tmpShutterDelay=0;
    }
    lcd.setCursor(9,1);
    PrintDigits(tmpShutterDelay,4);
    if (Keypress()==ENTERKEY)
    {
      ShutterDelay=tmpShutterDelay;
      StayInside=false;
    }
    if (Keypress()==BACKKEY)
      StayInside=false;    
  }
  StayInside=true;
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("After Shot:     ");
  int tmpAfterDelay=AfterDelay;
  while (StayInside)
  {
    if (Keypress()==LEFTKEY)
    {
      tmpAfterDelay-=1;
      if (tmpAfterDelay<0)
        tmpAfterDelay=3000;
    }
    if (Keypress()==RIGHTKEY)
    {
      tmpAfterDelay+=1;
      if (tmpAfterDelay>3000)
        tmpAfterDelay=0;
    }
    lcd.setCursor(12,1);
    PrintDigits(tmpAfterDelay,4);
    if (Keypress()==ENTERKEY)
    {
      AfterDelay=tmpAfterDelay;
      StayInside=false;
      lcd.setCursor(0,1);
      lcd.print("Delays Saved!   ");
      StayInside=false;
      if (MakeSounds)
        Beep(1);
      WriteToMem(4,PreDelay);
      WriteToMem(6,ShutterDelay);
      WriteToMem(8,AfterDelay);
      delay(1000);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Not saved!      ");
      if (MakeSounds)
        Beep(2);
      StayInside=false;    
      delay(1000);
    }
  }
  Mode=1;
  NotArmed=true;
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
  if (Debug)  Serial.println("Start focus!");
  digitalWrite(Optocoupler1, HIGH);
}



// --- PREFOCUS STOP procedure -----------------------------------------------------------------------------------------------------------------------
// Start by keeping the focus HIGH.

void PreFocusStop()
{
  if (Debug)  Serial.println("Stop focus.");
  digitalWrite(Optocoupler1, LOW);
}



// --- TRIGGER procedure -----------------------------------------------------------------------------------------------------------------------------
// Trigger the camera.

void Trigger()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if (!PreFocus)
    digitalWrite(Optocoupler1, HIGH);
  digitalWrite(Optocoupler2, HIGH);
  if (Debug)
  {
    Serial.print("Millis from triggering: ");
    Serial.println(millis()-TriggerMillis);
  }
  if (Infrared)
    Camera.shutterNow();
  delay(ShutterDelay);  
  digitalWrite(Optocoupler1, LOW);
  digitalWrite(Optocoupler2, LOW);
  delay(AfterDelay);
  if (Debug)  Serial.println("Triggered!");
  if (MakeSounds)
    Beep(3);
}



// --- BULB START procedure --------------------------------------------------------------------------------------------------------------------------
// Start bulb mode.

void StartBulb()
{
  digitalWrite(Optocoupler1, HIGH);
  digitalWrite(Optocoupler2, HIGH);
  if (Debug)  Serial.println("Starting Bulb mode!");
  if (MakeSounds)
    Beep(3);
}



// --- BULB STOP procedure ---------------------------------------------------------------------------------------------------------------------------
// Stop bulb mode.

void StopBulb()
{
  digitalWrite(Optocoupler1, LOW);
  digitalWrite(Optocoupler2, LOW);
  if (Debug)  Serial.println("Bulb mode stopped!");
  delay(AfterDelay);
  if (MakeSounds)
    Beep(3);
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
