// --- TriggerAid v2 ---------------------------------------------------------------------------------------------------------------------------------
//
//     (c)2013-2014, Antonis Maglaras
//     maglaras at gmail dot com
//     http://www.slr.gr 
//
//     More information: https://www.facebook.com/TheTriggerAid -- http://vegos.github.io/TriggerAid -- http://github.com/vegos/TriggerAid/wiki
//     Latest source code available at: https://github.com/vegos/TriggerAid
//     Photos etc at: http://www.facebook.com/TheTriggerAid & http://www.slr.gr/trigger
//     
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
//     EEPROM Mapping
//
//      0 - Buzzer On/Off (1/0 / True/False)
//      4 - PreDelay (0..3000 ms / 0..3 sec)
//      6 - Shutter Delay (0..3000 ms / 0..3 sec)
//      8 - Afer Shot Delay (0..3000 ms / 0..3 sec)
//     10 - Camera Brand for Infrared (1..6 / 1: Off, 2: Olympus, 3: Pentax, 4: Canon, 5: Nikon, 6: Sony)
//     12 - Prefocus On/Off (1/0 / True/False)
//     14 - Shortcut Menu Option
//     16 - Optocouplers Status (1: Both On, 2: First, 3: Second, 4: Both Off)
//     18 - Highspeed Delay (1..500 ms)
//     20 - Highspeed Limit (2..50 times)
//     22 - Built-in Light Trigger on High/Low (1/0 / True/False)
//     26 - External Trigger on High/Low (1/0 / True/False)
//     24 - Timelapse Exposure (1..300 seconds / 5 min) -- Currently Disabled
//     28 - Timelapse Interval (1..900 seconds / 15 min)
//     30 - Button Delay (10..250 ms)
//     32 - Buzzer Duration (10..250 ms)
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------



// Libraries used in this sketch          -- Many thanks to the creators of the following libraries!
#include <LiquidCrystal.h>                // Arduino included Library
#include <MemoryFree.h>                   // http://playground.arduino.cc/Code/AvailableMemory
#include <EEPROM.h>                       // Arduino included Library
#include <multiCameraIrControl.h>         // http://sebastian.setz.name/arduino/my-libraries/multi-camera-ir-control/



LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define  Version         "2.1.7"          // Current Version
//#define CUSTOMMESSAGE    "0123456789012345"        // Custom Message (16 characters) -- To display the message, uncomment this line.

// Create two (2) custom characters (for visual identification of armed/disarmed mode)
byte ONChar[8] =  { B00000, B01110, B11111, B11111, B11111, B01110, B00000, B00000 };
byte OFFChar[8] = { B00000, B01110, B10001, B10001, B10001, B01110, B00000, B00000 };

// Definition of buttons
#define  LEFTKEY            1
#define  RIGHTKEY           2
#define  ENTERKEY           3
#define  BACKKEY            4
#define  SHOOTKEY           5
#define  NOKEY              0

// Definition of pins used by Arduino
#define  IRLedPin          10      // For Infrared triggering
#define  LightSensorPin    A1      // Built-in Light trigger
#define  ExternalPin        8      // External (digital) sensor pin
#define  BuzzerPin         A0      // Buzzer -- I used a PWM pin. Maybe in next version it will play some music :)
#define  LeftButton        A4      // Left Button Pin
#define  RightButton       A3      // Right Button Pin
#define  EnterButton       A2      // Enter Button Pin
#define  BackButton         6      // Back Button Pin
#define  ShootButton        7      // Shoot Button Pin
#define  Optocoupler1Pin   13      // Optocoupler 1 is used for focus triggering    \____ Olympus E-3 (the camera that I make the testing
#define  Optocoupler2Pin    9      // Optocoupler 2 is used for shutter triggering  /     needs focus & shutter to be triggered together (at least).
                                   //                                                     With that setup, it's possible to trigger two (2) external
                                   //                                                     flashes for example, or two cameras etc.

//Olympus IR remote trigger
Olympus OlympusCamera(IRLedPin);   // \
Pentax PentaxCamera(IRLedPin);     //  \
Nikon NikonCamera(IRLedPin);       //   >--- Create instances of camera types on the same pin, for use with infrared trigger
Sony SonyCamera(IRLedPin);         //  /
Canon CanonCamera(IRLedPin);       // /


// Main Menu Strings
char* MenuItems[9] = { "",
                       "Light Trigger   ",    // Mode 1: Built-in light trigger
                       "External Trigger",    // Mode 2: External Trigger (works with digital modules, like a sound or light module.
                       "Time-Lapse      ",    // Mode 3: Time lapse. Exposure is available on setup menu (for use with Bulb mode, only on wired triggers).
                       "Bulb Mode       ",    // Mode 4: Bulb mode (My camera can't keep the shutter for more than 60 secs, so that way I can go up to 8 minutes!
                       "High Speed Burst",    // Mode 5: High Speed Burst mode (For flash units, as I guess that there are no cameras so fast).
                       "Setup           ",    // Mode 6: Setup System (Delays, Buzzer, Triggers)
                       "Information     ",    // Mode 7: Version information, memory free, etc.
                       "Factory Reset   ",    // Mode 8: Factory Reset.
                      };


// Definition of global variables
byte CameraBrand = 1;
byte MenuSelection = 1;
byte Mode = 1, ShortCut, OptocouplersStatus;
byte LightThreshold = 0, Light = 0;
boolean StandBy = true;
boolean Armed = false;
boolean MakeSounds, PreFocus, Optocoupler1Enabled, Optocoupler2Enabled, BuiltinTriggerOnHigh, ExtTriggerOnHigh;
boolean StayInside = false;
byte HighSpeedDelay, LimitTimes, ButtonDelay, BuzzerDelay;
int PreDelay, ShutterDelay, AfterDelay, Limit, TimeLapseInterval; // TimeLapseExposure;
int tmpDelay;
long StartMillis;




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
  #ifdef CUSTOMMESSAGE
    lcd.setCursor(0,1);
    lcd.print(CUSTOMMESSAGE);
    delay(1000);
  #endif
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
  pinMode(ShootButton, INPUT);  digitalWrite(ShootButton,HIGH);
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
    BuiltinTriggerOnHigh=true;
  else
    BuiltinTriggerOnHigh=false;
  if (ReadFromMem(26)==1)
    ExtTriggerOnHigh=true;
  else
    ExtTriggerOnHigh=false;
//  TimeLapseExposure=ReadFromMem(24);
  TimeLapseInterval=ReadFromMem(28);
  BuzzerDelay=ReadFromMem(32);
  ButtonDelay=ReadFromMem(30);
  if (ButtonDelay == 0)                    // Check for zero delay on keys and set it to 50
    ButtonDelay = 50;
  switch (OptocouplersStatus)
  {
    case 1:
      Optocoupler1Enabled=true;
      Optocoupler2Enabled=true;
      break;
    case 2:
      Optocoupler1Enabled=true;
      Optocoupler2Enabled=false;
      break;
    case 3:
      Optocoupler1Enabled=false;
      Optocoupler2Enabled=true;
      break;
    case 4:
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
      lcd.print("Shooting...     ");
      Trigger();
      ClearScreen();
    }
    // On BACK key pressed for 3 seconds, go to shortcut
    if (BackKey())
    {
      long Temp=millis();
      while (BackKey())
      {
        if (millis()-Temp>3000)
        {
          if (MakeSounds)
            Beep(3);
          lcd.setCursor(0,1);
          lcd.print("    Shortcut    ");
          while (BackKey());
          Mode=ShortCut;
          StandBy=false;
          return;
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
        Light = map(analogRead(LightSensorPin),0,1023,1,100);
        lcd.setCursor(15,1);
        if (BuiltinTriggerOnHigh)
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
        while (!BackKey())
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              if (PreFocus)
                PreFocusStart();
              lcd.setCursor(0,0);
              lcd.print("Light Trigger  ");
              lcd.setCursor(0,1);
              lcd.print("Active!        ");
              lcd.setCursor(15,0);
              lcd.write(1);
              while (!BackKey())
              {
                Light = map(analogRead(LightSensorPin),0,1023,1,100);
                if ((Light>LightThreshold) && (BuiltinTriggerOnHigh))
                  Trigger();
                if ((Light<LightThreshold) && !(BuiltinTriggerOnHigh))
                  Trigger();
              }
              if (PreFocus)
                PreFocusStop();
              Armed=false;
              lcd.setCursor(0,0);
              lcd.print("Light:         ");
              lcd.setCursor(0,1);
              lcd.print("Threshold:     ");
              lcd.setCursor(15,1);
              if (BuiltinTriggerOnHigh)
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
              if (BuiltinTriggerOnHigh)
                lcd.print("H");
              else
                lcd.print("L");                            
              lcd.setCursor(15,0);
              lcd.write(2);               
            }
          }
          Light = map(analogRead(LightSensorPin),0,1023,1,100);
          lcd.setCursor(7,0);
          PrintDigits(Light,3);
          if (Keypress() == RIGHTKEY)
          {
            LightThreshold+=1;
            if (LightThreshold>100)
              LightThreshold=1;
          }
          if (Keypress() == LEFTKEY)
          {
            LightThreshold-=1;
            if (LightThreshold<1)
              LightThreshold=100;
          }
          lcd.setCursor(11,1);
          PrintDigits(LightThreshold,3);          
        }
        if (PreFocus)
          PreFocusStop();                      
        StandBy=true;
        ClearScreen();
        break;
      case 2:
        lcd.clear();
        lcd.print("Ext. Trigger    ");
        lcd.setCursor(0,1);
        lcd.print("Trigger on      ");
        Armed=false;
        lcd.setCursor(15,0);
        lcd.write(2);
        while (!BackKey())
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              if (PreFocus)
                PreFocusStart();
              lcd.setCursor(0,1);
              lcd.print("Active!        ");
              lcd.setCursor(15,0);
              lcd.write(1);
              while (!BackKey())
              {
                if ((ExtTriggerOnHigh) && (digitalRead(ExternalPin) == HIGH))
                  Trigger();
                if ((!ExtTriggerOnHigh) && (digitalRead(ExternalPin) == LOW))
                  Trigger();
              }
              lcd.setCursor(0,1);
              lcd.print("Trigger on      ");
              Armed=false;
              lcd.setCursor(15,0);
              lcd.write(2); 
              delay(100);             
            }
            else
            {
              if (PreFocus)
                PreFocusStop();              
              lcd.setCursor(0,1);
              lcd.print("Trigger on      ");
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          lcd.setCursor(11,1);
          if ((Keypress() == LEFTKEY) || (Keypress() == RIGHTKEY))
            ExtTriggerOnHigh=!(ExtTriggerOnHigh);
          if (ExtTriggerOnHigh)
            lcd.print("HIGH");
          else
            lcd.print("LOW ");
        }
        StandBy=true;
        ClearScreen();
        break;
      case 3:
        lcd.clear();
        lcd.print("TimeLapse (   ) ");
        lcd.setCursor(0,1);
        lcd.print("Interval:    \"   ");
        tmpDelay=TimeLapseInterval;
        lcd.setCursor(10,1);
        PrintDigits(tmpDelay,3);
        StartMillis=millis();
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;
        while (!BackKey())
        {
          if (Keypress() == ENTERKEY)
          {
            Armed = !(Armed);
            if (Armed)
            {
              if (PreFocus)
                PreFocusStart();
              lcd.setCursor(15,0);
              lcd.write(1);
              StartMillis=millis();
            }
            else
            {
              if (PreFocus)
                PreFocusStop();                                          
              lcd.setCursor(15,0);
              lcd.write(2);
            }
          }
          if (Armed)
          {
            long remaining = (tmpDelay-((millis()-StartMillis)/1000));
            lcd.setCursor(11,0);
            if (remaining < 0)
              remaining = 0;
            PrintDigits(remaining,3);   
            if (remaining <= 1)
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
              tmpDelay=300;
          }
          if (Keypress() == RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>300)
              tmpDelay=1;
          }
          lcd.setCursor(10,1);
          PrintDigits(tmpDelay,3);          
        }    
        if (PreFocus)    
          PreFocusStop();
        StandBy=true;
        ClearScreen();
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
        while (!BackKey())
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
              tmpDelay=900;
          }
          if (Keypress() == RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>900)
              tmpDelay=1;
          }
          lcd.setCursor(6,1);
          PrintDigits(tmpDelay,3);
        }
        StopBulb();
        StandBy=true;
        ClearScreen();
        break;
      case 5:
        lcd.clear();
        lcd.print("HighSpeed Burst ");
        lcd.setCursor(0,1);
        lcd.print("Interval:     ms");
        tmpDelay=HighSpeedDelay;
        lcd.setCursor(15,0);
        lcd.write(2);
        Armed=false;  
        Limit = LimitTimes;
        while (!BackKey())
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
                if (BackKey())
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
              tmpDelay=900;
          }
          if (Keypress() == RIGHTKEY)
          {
            tmpDelay+=1;
            if (tmpDelay>900)
              tmpDelay=1;
          }
          lcd.setCursor(10,1);
          PrintDigits(tmpDelay,3);         
        }
        StandBy=true;
        ClearScreen();
        break;
      case 6:
        SetupMenu();
        ClearScreen();
        break;
      case 7:
        long result;
        lcd.clear();
        lcd.print("Version: ");
        lcd.print(Version);
        lcd.setCursor(0,1);
        PrintDigits(freeMemory(),4);
        lcd.print(" bytes free");
        while (!BackKey()) {}; // Delay for Back key
        delay(200);
        lcd.clear();
        lcd.print("Voltage:");
        while (!BackKey()) 
        {
          // Read 1.1V reference against AVcc
          ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
          delay(2); // Wait for Vref to settle
          ADCSRA |= _BV(ADSC); // Convert
          while (bit_is_set(ADCSRA,ADSC));
          result = ADCL;
          result |= ADCH<<8;
          result = 1126400L / result; // Back-calculate AVcc in mV        
          lcd.setCursor(0,1);
          lcd.print(result,DEC);
          lcd.print(" mV     ");
          delay(100);
        }
        delay(200);
        lcd.clear();
        lcd.print("    (c) 2014    ");
        lcd.setCursor(0,1);
        lcd.print("Antonis Maglaras");
        delay(200);
        while (!BackKey()) {}; // Delay for Back key     
        StandBy=true;
        ClearScreen();
        break;
      case 8:
        FactoryReset();
        StandBy=true;
        ClearScreen();
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
    if (BackKey())
    {
      Mode=MenuSelection;
      ClearScreen();
      StayInside=false;
      return; 
    }
  }
  ClearScreen();
  delay(10);
}




// --- RETURN KEYPRESS function ----------------------------------------------------------------------------------------------------------------------
// Check for keypress and return key.

byte Keypress()
{
  if (digitalRead(LeftButton) == LOW)
  {
    delay(ButtonDelay);
    return LEFTKEY;
  }
  else
    if (digitalRead(RightButton) == LOW)
    {
      delay(ButtonDelay);
      return RIGHTKEY;
    }
    else
      if (digitalRead(EnterButton) == LOW)
      {
        delay(ButtonDelay);
        return ENTERKEY;
      }
      else
        if (digitalRead(BackButton) == LOW)
        {
          delay(ButtonDelay);
          return BACKKEY;
        }
        else
          if (digitalRead(ShootButton) == LOW)
          {
            return SHOOTKEY;
          }
          else
            return NOKEY;
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

void Beep(byte x)
{
/*
  for (int j=0; j<x; j++)
  {
    digitalWrite(BuzzerPin, HIGH);
    delay(BuzzerDelay);
    digitalWrite(BuzzerPin, LOW);
    delay(BuzzerDelay);
  }
*/
  for (byte j=0; j<x; j++)
  {
    tone(BuzzerPin, 440);
    delay(BuzzerDelay);
    noTone(BuzzerPin);
    delay(BuzzerDelay);
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
    if (BackKey())
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("Wired Triggers  ");
  lcd.setCursor(0,1);
  StayInside=true;
  byte tmpOptocouplersStatus = OptocouplersStatus;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpOptocouplersStatus-=1;
      if (tmpOptocouplersStatus<1)
        tmpOptocouplersStatus=4;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpOptocouplersStatus+=1;
      if (tmpOptocouplersStatus>4)
        tmpOptocouplersStatus=1;
    }
    lcd.setCursor(0,1);
    switch (tmpOptocouplersStatus)
    {
      case 1:     
        lcd.print("Both Enabled    ");
        break;
      case 2:
        lcd.print("First Only      ");
        break;
      case 3:
        lcd.print("Second Only     ");
        break;
      case 4:
        lcd.print("Both Disabled   ");
        break;
    }
    if (Keypress() == ENTERKEY)
    {
      OptocouplersStatus = tmpOptocouplersStatus;
      switch (OptocouplersStatus)
      {
        case 1:
          Optocoupler1Enabled=true;
          Optocoupler2Enabled=true;
          break;
        case 2:
          Optocoupler1Enabled=true;
          Optocoupler2Enabled=false;
          break;
        case 3:
          Optocoupler1Enabled=false;
          Optocoupler2Enabled=true;
          break;
        case 4:
          Optocoupler1Enabled=false;
          Optocoupler2Enabled=false;
      }
      WriteToMem(16,OptocouplersStatus);    
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  lcd.clear();
  lcd.print("Infrared Trigger");
  lcd.setCursor(0,1);
  StayInside=true;
  byte tmpCameraBrand=CameraBrand;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpCameraBrand-=1;
      if (tmpCameraBrand<1)
        tmpCameraBrand=6;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpCameraBrand+=1;
      if (tmpCameraBrand>6)
        tmpCameraBrand=1;
    }
    lcd.setCursor(0,1);
    switch (tmpCameraBrand)
    {
      case 1: // None
        lcd.print("Disabled");
        break;
      case 2: // Olympus
        lcd.print("Olympus ");
        break;
      case 3: // Pentax
        lcd.print("Pentax  ");
        break;
      case 4:
        lcd.print("Canon   ");
        break;
      case 5: // Nikon
        lcd.print("Nikon   ");
        break;
      case 6: // Sony
        lcd.print("Sony    ");
        break;
    }
    if (Keypress() == ENTERKEY)
    {
      CameraBrand=tmpCameraBrand;
      WriteToMem(10,CameraBrand);
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("Light Trigger  ");
  lcd.setCursor(0,1);
  lcd.print("Trigger on     ");
  boolean tmpBuiltinTriggerOnHigh=BuiltinTriggerOnHigh;
  while (StayInside)
  {
    lcd.setCursor(11,1);
    if (tmpBuiltinTriggerOnHigh)
      lcd.print("HIGH");
    else
      lcd.print("LOW ");
    if ((Keypress() == LEFTKEY) != (Keypress() == RIGHTKEY))
      tmpBuiltinTriggerOnHigh = !(tmpBuiltinTriggerOnHigh);
    if (Keypress() == ENTERKEY)
    {
      BuiltinTriggerOnHigh=tmpBuiltinTriggerOnHigh;
      if (BuiltinTriggerOnHigh)
        WriteToMem(22,1);
      else
        WriteToMem(22,0);
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("Ext. Trigger   ");
  lcd.setCursor(0,1);
  lcd.print("Trigger on     ");
  boolean tmpExtTriggerOnHigh=ExtTriggerOnHigh;
  while (StayInside)
  {
    lcd.setCursor(11,1);
    if (tmpExtTriggerOnHigh)
      lcd.print("HIGH");
    else
      lcd.print("LOW ");
    if ((Keypress() == LEFTKEY) != (Keypress() == RIGHTKEY))
      tmpExtTriggerOnHigh = !(tmpExtTriggerOnHigh);
    if (Keypress() == ENTERKEY)
    {
      ExtTriggerOnHigh=tmpExtTriggerOnHigh;
      if (ExtTriggerOnHigh)
        WriteToMem(26,1);
      else
        WriteToMem(26,0);
      SettingsSaved();
    }
    if (BackKey())
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
    if (BackKey())
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
    if (BackKey())
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
    if (BackKey())
    SettingsNotSaved();
  }
/*  
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
      if (tmpTimeLapseExposure<1)
        tmpTimeLapseExposure=300;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpTimeLapseExposure+=1;
      if (tmpTimeLapseExposure>300)
        tmpTimeLapseExposure=1;
    }
    lcd.setCursor(9,1);
    PrintDigits(tmpTimeLapseExposure,3);
    if (Keypress() == ENTERKEY)
    {
      TimeLapseExposure=tmpTimeLapseExposure;
      WriteToMem(24,TimeLapseExposure);
      SettingsSaved();
    }
    if (BackKey())
    SettingsNotSaved();
  }
*/  
  StayInside=true;
  lcd.clear();
  lcd.print("Timelapse Intrvl");
  lcd.setCursor(0,1);
  lcd.print("Seconds: ");
  int tmpTimeLapseInterval=TimeLapseInterval;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpTimeLapseInterval-=1;
      if (tmpTimeLapseInterval<1)
        tmpTimeLapseInterval=900;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpTimeLapseInterval+=1;
      if (tmpTimeLapseInterval>900)
        tmpTimeLapseInterval=1;
    }
    lcd.setCursor(9,1);
    PrintDigits(tmpTimeLapseInterval,3);
    if (Keypress() == ENTERKEY)
    {
      TimeLapseInterval=tmpTimeLapseInterval;
      WriteToMem(28,TimeLapseInterval);
      SettingsSaved();
    }
    if (BackKey())
    SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("HighSpeed Delay ");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  byte tmpHighSpeedDelay=HighSpeedDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpHighSpeedDelay-=1;
      if (tmpHighSpeedDelay<1)
        tmpHighSpeedDelay=100;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpHighSpeedDelay+=1;
      if (tmpHighSpeedDelay>100)
        tmpHighSpeedDelay=1;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpHighSpeedDelay,3);
    if (Keypress() == ENTERKEY)
    {
      HighSpeedDelay=tmpHighSpeedDelay;
      WriteToMem(18,HighSpeedDelay);
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("HighSpeed Limit");
  lcd.setCursor(0,1);
  lcd.print("Times: ");
  byte tmpLimitTimes=LimitTimes;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpLimitTimes-=1;
      if (tmpLimitTimes<2)
        tmpLimitTimes=50;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpLimitTimes+=1;
      if (tmpLimitTimes>50)
        tmpLimitTimes=2;
    }
    lcd.setCursor(7,1);
    PrintDigits(tmpLimitTimes,2);
    if (Keypress() == ENTERKEY)
    {
      LimitTimes=tmpLimitTimes;
      WriteToMem(20,LimitTimes);
      SettingsSaved();
    }
    if (BackKey())
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
    if (BackKey())
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
    if (BackKey())
      SettingsNotSaved();
  }  
  StayInside=true;
  lcd.clear();
  lcd.print("Buzzer Duration ");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  byte tmpBuzzerDelay=BuzzerDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpBuzzerDelay-=1;
      if (tmpBuzzerDelay<10)
        tmpBuzzerDelay=250;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpBuzzerDelay+=1;
      if (tmpBuzzerDelay>250)
        tmpBuzzerDelay=10;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpBuzzerDelay,3);
    if (Keypress() == ENTERKEY)
    {
      BuzzerDelay=tmpBuzzerDelay;
      WriteToMem(32,BuzzerDelay);
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  StayInside=true;
  lcd.clear();
  lcd.print("Button Delay    ");
  lcd.setCursor(0,1);
  lcd.print("Millis: ");
  byte tmpButtonDelay=ButtonDelay;
  while (StayInside)
  {
    if (Keypress() == LEFTKEY)
    {
      tmpButtonDelay-=1;
      if (tmpButtonDelay<10)
        tmpButtonDelay=250;
    }
    if (Keypress() == RIGHTKEY)
    {
      tmpButtonDelay+=1;
      if (tmpButtonDelay>250)
        tmpButtonDelay=10;
    }
    lcd.setCursor(8,1);
    PrintDigits(tmpButtonDelay,3);
    if (Keypress() == ENTERKEY)
    {
      ButtonDelay=tmpButtonDelay;
      WriteToMem(30,ButtonDelay);
      SettingsSaved();
    }
    if (BackKey())
      SettingsNotSaved();
  }
  Mode=1;
  StandBy=true;
}




// --- WRITE TO EEPROM procedure ---------------------------------------------------------------------------------------------------------------------
// Write numbers (0-65535) to EEPROM (using 2 bytes).

void WriteToMem(byte address, int number)
{
  byte a = number/256;
  byte b = number % 256;
  EEPROM.write(address,a);
  EEPROM.write(address+1,b);
}




// --- READ FRON EEPROM procedure --------------------------------------------------------------------------------------------------------------------
// Read numbers (0-65535) from EEPROM (using 2 bytes).

int ReadFromMem(byte address)
{
  byte a=EEPROM.read(address);
  byte b=EEPROM.read(address+1);

  return a*256+b;
}




// --- PREFOCUS START procedure ----------------------------------------------------------------------------------------------------------------------
// Start by keeping the focus HIGH.

void PreFocusStart()
{
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, HIGH);
}




// --- PREFOCUS STOP procedure -----------------------------------------------------------------------------------------------------------------------
// Start by keeping the focus HIGH.

void PreFocusStop()
{
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
}




// --- TRIGGER procedure -----------------------------------------------------------------------------------------------------------------------------
// Trigger the camera.

void Trigger()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if ((!PreFocus) && (Optocoupler1Enabled))
    digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  ShootIR();
  delay(ShutterDelay);  
  if ((!PreFocus) && (Optocoupler1Enabled))
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  delay(AfterDelay);
  if (MakeSounds)
    Beep(3);
}



/*
// --- TRIGGER TimeLapse -----------------------------------------------------------------------------------------------------------------------------
// Trigger the camera.

void TriggerTimeLapse()
{
  if (PreDelay!=0)
    delay(PreDelay);
  if ((!PreFocus) && (Optocoupler1Enabled))
    digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  ShootIR();
  for (int x=0; x<TimeLapseExposure; x++)
    delay(1000);  
  if ((!PreFocus) && (Optocoupler1Enabled))
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  if (MakeSounds)
    Beep(3);
}
*/



// --- HIGH SPEED TRIGGER procedure ------------------------------------------------------------------------------------------------------------------
// High-Speed Trigger (for flashes). Can trigger up to 50 times, on every millisecond. Works best with high speed flash units (due to recharging).


void HighSpeedTrigger()
{
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  delay(HighSpeedDelay);
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
}



// --- BULB START procedure --------------------------------------------------------------------------------------------------------------------------
// Start bulb mode.

void StartBulb()
{
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, HIGH);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, HIGH);
  if (MakeSounds)
    Beep(1);
}



// --- BULB STOP procedure ---------------------------------------------------------------------------------------------------------------------------
// Stop bulb mode.

void StopBulb()
{
  if (Optocoupler1Enabled)
    digitalWrite(Optocoupler1Pin, LOW);
  if (Optocoupler2Enabled)
    digitalWrite(Optocoupler2Pin, LOW);
  delay(AfterDelay);
  if (MakeSounds)
    Beep(2);
}




// --- BACKKEY function ------------------------------------------------------------------------------------------------------------------------------
// For faster checking for back keypress.

boolean BackKey()
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
          WriteToMem(24,1);
          WriteToMem(26,1);
          WriteToMem(28,15);
          WriteToMem(30,50);
          WriteToMem(32,50);
          lcd.setCursor(0,1);
          lcd.print("Done!           ");
          Beep(3);
          delay(1000);
          SoftReset();
        }
        if (BackKey())
        {
          StayInside=false;
          StayInside2=false;
        }      
      }
    }
    if (BackKey())
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




// --- Shoot using Infrared --------------------------------------------------------------------------------------------------------------------------
// Trigger the selected camera using Infrared.

void ShootIR()
{
  if (CameraBrand != 1)
  {
    switch (CameraBrand)
    {
      case 2: // Olympus
        OlympusCamera.shutterNow();
        break;
      case 3: // Pentax
        PentaxCamera.shutterNow();
        break;
      case 4: // Canon
        CanonCamera.shutterNow();
        break;
      case 5: // Nikon
        NikonCamera.shutterNow();
        break;
      case 6: // Sony
        SonyCamera.shutterNow();
        break;
    }
  }
}



// --- Clear Screen ----------------------------------------------------------------------------------------------------------------------------------
// Show the start screen.                            

void ClearScreen()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   TriggerAid   ");
  lcd.setCursor(0,1);
  lcd.print("Ready!          ");        
}
