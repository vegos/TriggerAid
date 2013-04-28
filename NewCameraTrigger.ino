#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <MemoryFree.h>
#include <EEPROM.h>
#include <multiCameraIrControl.h>


LiquidCrystal_I2C lcd(0x20,16,2);

#define  Version         "3.20b"


byte onchar[8] = { B00000, B01110, B11111, B11111, B11111, B01110, B00000, B00000 };
byte offchar[8] = { B00000, B01110, B10001, B10001, B10001, B01110, B00000, B00000 };

#define  LEFTKEY         1
#define  RIGHTKEY        2
#define  ENTERKEY        3
#define  BACKKEY         4
#define  NONEKEY         0


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

Olympus Camera(IRLed);


//                     0123456789012345
char* MenuItems[10] = { "",
                        "-> Light Trigger",    // 1
                        "-> Ext. Trigger ",    // 2
                        "-> Time Lapse   ",    // 3
                        "-> Bulb Mode    ",    // 4
                        "-> Set Date/Time",    // 5
                        "-> Sound/Backlit",    // 6
                        "-> Set Delays   ",    // 7
                        "-> Infrared/Wire",    // 8
                        "-> Information  ",    // 8
                    };
int MenuSelection = 1;

int CurrentHour = 99, CurrentMin = 99, CurrentSec = 99, CurrentDay = 99, CurrentMonth = 99, CurrentYear = 99;

int Mode = 1;

int LightThreshold = 0, Light = 0;

boolean NotArmed = true;
boolean WhenHigh = true;
boolean Armed = false;
boolean BackLight, MakeSounds, Infrared, Wired;
int PreDelay, ShutterDelay, AfterDelay;
long tmpDelay=60;
long StartMillis, tmpMillis;

#define    Debug    true


void setup()
{
  if (Debug)  Serial.begin(9600);
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
    Wired=true;
  else
    Wired=false;
  //
  if (BackLight)
    lcd.backlight();
  else
    lcd.noBacklight();
  NotArmed=true;
}


void loop()
{
  if (NotArmed)
  {
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
          if ((Light>LightThreshold) && (Armed))
            Trigger();
          if (Keypress()==ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              lcd.setCursor(15,0);
              lcd.write(1);
            }
            else
            {
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
              lcd.setCursor(15,0);
              lcd.write(1);
            }
            else
            {
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          if (Armed)
          {
            if ((WhenHigh) && (digitalRead(ExternalPin)==HIGH))
              Trigger();
            if ((!WhenHigh) && (digitalRead(ExternalPin)==LOW))
              Trigger();              
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
        lcd.print("Delay: 060 secs ");
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
          lcd.setCursor(7,1);
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
          lcd.setCursor(7,1);
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
/*
      switch (MenuSelection)
      {
        case 1:
          NotArmed=false;
          StayInside=false;
          Mode=1;
          break;
        case 2:
          Mode=2;
          NotArmed=false;
          StayInside=false;
          break;
        case 3:
          Mode=3;
          NotArmed=false;
          StayInside=false;
          break;
        case 4:
          Mode=4;
          NotArmed=false;
          StayInside=false;
          break;
        case 5:
          Mode=5;
          NotArmed=false;
          StayInside=false;
          break;
        case 6:
          Mode=6;
          NotArmed=false;
          StayInside=false;
          break;
        case 7:
          Mode=7;
          NotArmed=false;
          StayInside=false;
          break;
        case 8:
          Mode=8;
          NotArmed=false;
          StayInside=false;
          break;
        case 9:
          Mode=9;
          NotArmed=false;
          StayInside=false;
          break;
      }
*/      
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

void Beep(int x)
{
  for (int j=0; j<x; j++)
  {
    digitalWrite(BuzzerPin, HIGH);
    delay(100);
    digitalWrite(BuzzerPin, LOW);
    delay(100);
  }
}


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

void ResetTimeVars()
{
  CurrentHour=99;
  CurrentMin=99;
  CurrentSec=99;
  CurrentYear=99;
  CurrentMonth=99;
  CurrentDay=99;
}

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

void SetupInfrared()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Wired Trigger   ");
  lcd.setCursor(0,1);
  lcd.print("Enabled:     ");
  boolean StayInside=true;
  boolean tmpWired=Wired;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      tmpWired=!(tmpWired);
    lcd.setCursor(9,1);
    if (tmpWired)
      lcd.print("ON ");
    else
      lcd.print("OFF");
    if (Keypress()==ENTERKEY)
    {
      Wired=tmpWired;
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
      lcd.print("ON ");
    else
      lcd.print("OFF");
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
      if (Wired)
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



void WriteToMem(byte address, int number)
{
int a = number/256;
int b = number % 256;

EEPROM.write(address,a);
EEPROM.write(address+1,b);
}

int ReadFromMem(byte address)
{
  int a=EEPROM.read(address);
  int b=EEPROM.read(address+1);

  return a*256+b;
}


void Trigger()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if (Wired)
  {
    digitalWrite(Optocoupler1, HIGH);
    digitalWrite(Optocoupler2, HIGH);
  }
  if (Infrared)
    Camera.shutterNow();
  if (MakeSounds)
    Beep(1);
  if (Wired)
  {
    delay(ShutterDelay);  
    digitalWrite(Optocoupler1, LOW);
    digitalWrite(Optocoupler2, LOW);
  }
  delay(AfterDelay);
  if (Debug)  Serial.println("Triggered!");
  if (MakeSounds)
    Beep(2);
}


void StartBulb()
{
  digitalWrite(Optocoupler1, HIGH);
  digitalWrite(Optocoupler2, HIGH);
  if (MakeSounds)
    Beep(1);
}

void StopBulb()
{
  digitalWrite(Optocoupler1, LOW);
  digitalWrite(Optocoupler2, LOW);
  delay(AfterDelay);
}
