#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <MemoryFree.h>
#include <EEPROM.h>
#include <multiCameraIrControl.h>


LiquidCrystal_I2C lcd(0x20,16,2);

#define  Version         "3.03b"

#define  LEFTKEY         1
#define  RIGHTKEY        2
#define  ENTERKEY        3
#define  BACKKEY         4
#define  NONEKEY         0


#define  IRLed          10      // For IR transmitting
#define  LightSensor    A1      // Built-in Light trigger

#define  Input1Pin      A0      // For future use
#define  ExternalPin     2      // For connecting external sensors
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
                        "-> Standby      ",    // 1
                        "-> Light Trigger",    // 2
                        "-> Ext. Trigger ",    // 3
                        "-> Time Lapse   ",    // 4
                        "-> Time Trigger ",    // 5
                        "-> Set Date/Time",    // 6
                        "-> Sound/Backlit",    // 7
                        "-> Set Delays   ",    // 8
                        "-> Information  ",    // 9
                    };
int MenuSelection = 1;

int CurrentHour = 99, CurrentMin = 99, CurrentSec = 99, CurrentDay = 99, CurrentMonth = 99, CurrentYear = 99;

int Mode = 1;

int LightThreshold = 0, Light = 0;

boolean NotArmed = true;
boolean AtMenu = false;
boolean WhenHigh = true;
boolean BackLight;
boolean Keybeep;
int PreDelay, PostDelay;
int tmpDelay=60;
long StartMillis;




void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  setSyncProvider(RTC.get);
  lcd.clear();
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print(" Camera Trigger ");
  lcd.setCursor(0,1);
  lcd.print("Antonis Maglaras");
  delay(1000);
  pinMode(RightButton, INPUT_PULLUP);  
  pinMode(BackButton, INPUT_PULLUP);
  pinMode(LeftButton, INPUT_PULLUP);
  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(Optocoupler1, OUTPUT);
  pinMode(Optocoupler2, OUTPUT);
  if (ReadFromMem(0)==0)
    Keybeep=true;
  else
    Keybeep=false;
  if (ReadFromMem(2)==1)
    BackLight=true;
  else
    BackLight=false;
  PreDelay=ReadFromMem(4);
  PostDelay=ReadFromMem(6);
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
      Serial.println("MAIN MENU");
      MainMenu();
    }
    DisplayTime();  
  }
  else
  {
    switch (Mode)
    {
      case 2:
        lcd.clear();
        //         0123456789012345
        lcd.print("Light:          ");
        lcd.setCursor(0,1);
        lcd.print("Threshold:      ");
        Light = map(analogRead(LightSensor),0,1023,0,100);
        Serial.print("Light Reading: ");
        Serial.println(analogRead(LightSensor));
        LightThreshold = Light+10;
        while (Keypress() != BACKKEY)
        {
          Serial.print("Light Reading: ");
          Serial.println(analogRead(LightSensor));
          Light = map(analogRead(LightSensor),0,1023,0,100);
          if (Light>LightThreshold)
          {
            Beep(3);
          }
          lcd.setCursor(7,0);
          PrintDigits(Light,4);
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
      case 3:
        lcd.clear();
        //         0123456789012345
        lcd.print("External Trigger");
        lcd.setCursor(0,1);
        lcd.print("Trigger on      ");
        while (Keypress()!=BACKKEY)
        {
          if ((WhenHigh) && (digitalRead(ExternalPin)==HIGH))
            Trigger();
          if ((!WhenHigh) && (digitalRead(ExternalPin)==LOW))
            Trigger();
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
      case 4:
        lcd.clear();
        lcd.print("Time Lapse [   ]");
        lcd.setCursor(0,1);
        //         0123456789012345
        lcd.print("Delay:     secs ");
        tmpDelay=60;
        StartMillis=millis();
        while (Keypress()!=BACKKEY)
        {
          lcd.setCursor(12,0);
          PrintDigits(((tmpDelay*1000+StartMillis)-millis()),3);
          if (millis()-StartMillis>(tmpDelay*1000))
          {
            Trigger();
            StartMillis=millis();
          }
          if (Keypress()==LEFTKEY)
          {
            tmpDelay-=1;
            if (tmpDelay<0)
              tmpDelay=0;
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
        // Specify time and interval to shoot
        break;
      case 6:
        SetupTime();
        NotArmed=true;
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 7:
        SetupInterface();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 8:
        SetupDelays();
        lcd.clear();
        lcd.print(" Camera Trigger ");
        ResetTimeVars();
        break;
      case 9:
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
  lcd.print("Mode");
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
      switch (MenuSelection)
      {
        case 1:
          NotArmed=true;
          StayInside=false;
          Mode=1;         
          break;
        case 2:
          NotArmed=false;
          StayInside=false;
          Mode=2;
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
    Serial.println("LEFT");
    if (Keybeep)
      KeyBeepFunc();
    else
      delay(50);
    return LEFTKEY;
  }
  if (digitalRead(RightButton)==LOW)
  {
    Serial.println("RIGHT");
    if (Keybeep)
      KeyBeepFunc();
    else
      delay(50);
    return RIGHTKEY;
  }
  if (digitalRead(EnterButton)==LOW)
  {
    Serial.println("ENTER");
    if (Keybeep)
      KeyBeepFunc();
    else
      delay(50);
    return ENTERKEY;
  }
  if (digitalRead(BackButton)==LOW)
  {
    Serial.println("BACK");
    if (Keybeep)
      KeyBeepFunc();
    else
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

void KeyBeepFunc()
{
  digitalWrite(BuzzerPin, HIGH);
  delay(50);
  digitalWrite(BuzzerPin, LOW);
  delay(50);
}

void DisplayTime()
{
  lcd.setCursor(8,1);
  if (hour()!=CurrentHour)
  {
    PrintDigits(hour(),2);
    CurrentHour=hour();
  }
  lcd.setCursor(10,1);
  lcd.print(":");
  if (minute()!=CurrentMin)
  {
    PrintDigits(minute(),2);
    CurrentMin=minute();
  }
  lcd.setCursor(13,1);
  lcd.print(":");
  if (second()!=CurrentSec)
  {
    PrintDigits(second(),2);
    CurrentSec=second();
  }
  lcd.setCursor(0,1);
  lcd.print("Standby ");
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
      Beep(1);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.clear();
      setSyncProvider(RTC.get);
      StayInside=false;
      lcd.print("Not saved!");
      Beep(2);
    }
  }
  delay(2000);
  Mode=1;
  NotArmed=true;
}

void SetupInterface()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Setup Interface ");
  lcd.setCursor(0,1);
  lcd.print("Key Beep:    ");
  boolean StayInside=true;
  boolean KeybeepTemp=Keybeep;
  while (StayInside)
  {
    if ((Keypress()==LEFTKEY) || (Keypress()==RIGHTKEY))
      KeybeepTemp=!(KeybeepTemp);
    lcd.setCursor(10,1);
    if (KeybeepTemp)
      lcd.print("ON ");
    else
      lcd.print("OFF");
    if (Keypress()==ENTERKEY)
    {
      Keybeep=KeybeepTemp;
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
      Beep(1);
      if (Keybeep)
        WriteToMem(0,1);
      else
        WriteToMem(0,0);
      if (BackLight)
        WriteToMem(2,1);
      else
        WriteToMem(2,0);
      delay(1000);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Not saved!      ");
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
  int tmpPostDelay=PostDelay;
  while (StayInside)
  {
    if (Keypress()==LEFTKEY)
    {
      tmpPostDelay-=1;
      if (tmpPostDelay<0)
        tmpPostDelay=3000;
    }
    if (Keypress()==RIGHTKEY)
    {
      tmpPostDelay+=1;
      if (tmpPostDelay>3000)
        tmpPostDelay=0;
    }
    lcd.setCursor(9,1);
    PrintDigits(tmpPostDelay,4);
    if (Keypress()==ENTERKEY)
    {
      PreDelay=tmpPreDelay;
      StayInside=false;
      lcd.print("Delays Saved!   ");
      StayInside=false;
      Beep(1);
      WriteToMem(4,PreDelay);
      WriteToMem(6,PostDelay);
      delay(1000);
    }
    if (Keypress()==BACKKEY)
    {
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Not saved!      ");
      Beep(2);
      StayInside=false;    
      delay(1000);
    }
  }  
  Mode=1;
  NotArmed=true;
}


void Trigger()
{
  if (PreDelay!=0)
    delay(PreDelay);
  pinMode(Optocoupler1, HIGH);
  pinMode(Optocoupler2, HIGH);
  Camera.shutterNow();
  Beep(1);
  delay(PostDelay);
  pinMode(Optocoupler1, LOW);
  pinMode(Optocoupler2, LOW);
  Beep(2);
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

boolean EnterKey()
{
  if (digitalRead(EnterButton)==LOW)
    return true;
  else
    return false;
}
