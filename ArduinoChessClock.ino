#include <Wire.h>
#include <LiquidCrystal_I2C.h>


//IO Declaration
//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

String timeTemp;

//Rotary Encoder
int rotaryEncoder = A0;
int rotaryEVal = 0;

//Button
int ppButPin = 2; //avoid PWM
int p1ButPin = 4;
int p2ButPin = 7;
int optButPin = 8;
int ppButState, p1ButState, p2ButState,optButState;

//Player LED Indicators
int p1LEDPin = 12;
int p2LEDPin = 13;


//Declaring States
int screenState;  //state of display 0-> Play | 1 -> Options | 2 -> Gametime | 3-> Increment
int playState;    //state of play 0-> Stopped | 1-> Paused | 2-> P1 Clock Run | 3-> P2 Clock Run | 4 -> Time Up
int pauseMem;     //Pause memory to remember which player was playing last before pausing
int cursorState;

//Declaring Time Values
signed short p1H, p1M, p1S;
signed short p2H, p2M, p2S;
signed short incM, incS;
signed short startH, startM, startS;

/*
 * 
 * Custom Characters
 * 
*/
//[0]Play Arrow
byte customChar0[8] = {
  0b11000,
  0b11100,
  0b11110,
  0b11111,
  0b11111,
  0b11110,
  0b11100,
  0b11000
};

//[1]Pause
byte customChar1[8] = {
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011
};

//[2]Stop
byte customChar2[8] = {
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b00000
};

//[3]Game Options
byte customChar3[8] = {
  0b00000,
  0b00011,
  0b00100,
  0b00100,
  0b00111,
  0b01100,
  0b11000,
  0b10000
};

//[4]Turn Player Indicator, Toggled Selector
byte customChar4[8] = {
  0b00000,
  0b00000,
  0b01100,
  0b01110,
  0b01110,
  0b01100,
  0b00000,
  0b00000
};

//[5]Untoggled Selector
byte customChar5[8] = {
  0b00000,
  0b00000,
  0b01100,
  0b01010,
  0b01010,
  0b01100,
  0b00000,
  0b00000
};


void setup()
{
  //LED INIT
  pinMode(p1LEDPin, OUTPUT);
  pinMode(p2LEDPin, OUTPUT);

  //BUTTON INIT
  pinMode(ppButPin, INPUT);
  pinMode(p1ButPin, INPUT);
  pinMode(p2ButPin, INPUT);
  pinMode(optButPin, INPUT);

  //LCD INIT
  lcd.begin();
  lcd.backlight();
  lcd.clear();

  //Splash Screen
  lcd.setCursor(0,0); //(Column, Row)
  lcd.print("Get Mwanged inc");
  lcd.setCursor(0,1);
  lcd.print("Chess Clock");
  delay(3000);
  lcd.clear();

  //Set States
  playState = 0;
  screenState = 0;
  pauseMem = 0;
  cursorState = 0;

  //Set default times (30 min game with 5 sec 15 sec increments)
  incM = 0;
  incS = 15;
  startH = 0;
  startM = 30;
  startS = 0;



  //Creating and Storing all custom chars
  lcd.createChar(0, customChar0); //Play Arrow
  lcd.createChar(1, customChar1); //Pause
  lcd.createChar(2, customChar2); //Stop
  lcd.createChar(3, customChar3); //Options
  lcd.createChar(4, customChar4); //Toggled Indicator
  lcd.createChar(5, customChar5); //Untoggled Indicator
  //lcd.write(byte([X]));
}

String printTime(int a)
{
  if(a < 10){
    return "0" + a;
    }
  else{
    return ""+a;
    }
}

//Does an entire reset of the clock digits
void resetClock() 
{
  lcd.setCursor(6,0);
  lcd.write(incM);
  lcd.setCursor(8,0);
  lcd.print(printTime(incS));
  lcd.setCursor(0,1);
  lcd.print("0:00:00  0:00:00");
  lcd.setCursor(0,1);
  lcd.write(p1H);
  lcd.setCursor(2,1);
  lcd.print(printTime(p1M));
  lcd.setCursor(5,1);
  lcd.print(printTime(p1S));
  lcd.setCursor(9,1);
  lcd.write(p2H);
  lcd.setCursor(11,1);
  lcd.print(printTime(p2M));
  lcd.setCursor(14,1);
  lcd.print(printTime(p2S));
}

//Sets player Icon based on play state
void setPlayer(){
  switch(playState){
      case 0: //stopped
      {
        lcd.setCursor(1,0);
        lcd.write(byte(5));
        lcd.setCursor(13,0);
        lcd.write(byte(5));
        break;
      }
      case 1://paused
      {
        break;
      }
      case 2://p1 turn
      {
        lcd.setCursor(1,0);
        lcd.write(byte(4));
        lcd.setCursor(13,0);
        lcd.write(byte(5));
        break;
      }
      case 3://p2 turn
      {
        lcd.setCursor(1,0);
        lcd.write(byte(5));
        lcd.setCursor(13,0);
        lcd.write(byte(4));
        break;
      }
    }  
}

void loop()
{
  //Input Value Grabbers
  rotaryEVal = analogRead(rotaryEncoder); // (<10 - >1020)
  ppButState = digitalRead(ppButPin); //LOW -> HIGH
  p1ButState = digitalRead(p1ButPin);
  p2ButState = digitalRead(p2ButPin);
  optButState = digitalRead(optButPin);




  //button interactions
  if(screenState == 0) //playScreen
  {
    //Play/Pause button pressed while game is playing: pause the game and store previous turn for continue
    if(((playState == 2) or (playState == 3)) and (ppButState == HIGH)) 
    {
      pauseMem = playState;
      playState = 1;
    
    }
    //Play/Pause button pressed while game is paused: continue the game from last player turn
    else if ((playState == 1) and (ppButState == HIGH)) 
    {
      playState = pauseMem;
    }
    //P1 button pressed: begin p2 clock when in pause, stopped, or currently p1 turn
    if(((playState == 2)or(playState == 1)or(playState == 0)) and (p1ButState == HIGH)) 
    {
      playState = 3;
    }
    //P2 button pressed: begin p1 clock when in pause, stopped, or currently p2 turn
    if(((playState == 3) or (playState == 1)or(playState == 0)) and (p2ButState == HIGH))
    {
      playState = 2;
    }
    //Options button pressed: Pause gamestate and enter options menu
    if((optButState == HIGH) and (screenState == 0))
    {
      playState = 1;
      screenState = 1;
    }
    
  }
  else if(screenState == 1)//Options menu
  {
    //Pause and Player buttons do nothing in this state
    //When Opt is pressed again, go to screenState that corresponds
    if((optButState == HIGH) and (cursorState == 0))
    {
      screenState = 2;
    }
    else if((optButState == HIGH) and (cursorState == 1))
    {
      screenState = 3;
    }
    else if((optButState == HIGH) and (cursorState == 2))
    {
      screenState = 0;
    }
  }
  else if((screenState == 2)or(screenState == 3)) //either gamestate or increment change menus
  {
    //opt but return to original options menu, p1 but, p2 but, and pause but have no function
    if(optButState == HIGH)
    {
      screenState = 1;
    }
  }


  //Screen State
  switch(screenState)
  {
    case 0: //Game Screen
      lcd.setCursor(0,0);
      lcd.print(" _P1 +0:00 _ _P2");
      lcd.setCursor(11,0);
      lcd.write(byte(0));
      resetClock();
      setPlayer();

      //GAME SCREEN PLAY STATES
      switch(playState)
      {
        case 0://stopped
        { 
          p1H = startH;
          p2H = startH;
          p1M = startM;
          p2M = startM;
          p1S = startS;
          p2S = startS;
          resetClock();
          lcd.setCursor(11,0);
          lcd.write(byte(2));
          break;
        }
        case 1: //paused
        {
          lcd.setCursor(11,0);
          lcd.write(byte(1));
          break;
        }
        case 2: //P1 turn
        {
          lcd.setCursor(0,1);
          p1S += incS;
          p1M += incM;
          if(p1S > 59){
            p1S - 60;
            p1M += 1;
            }
          if(p1M > 59){
            p1M - 60;
            p1H += 1;
            }
            
          if(p1S == 0 and (p1M > 0 || p1H > 0)){
            p1S = 60;
            if(p1M == 0){
                p1H -= 1;
                p1M = 60;
              }
            p1M -= 1;
            p1S -= 1;
            delay(1000);
            }
           else if(p1S > 0)
           {
             p1S -= 1;
             delay(1000);
           }
           else
           {
            lcd.print(">TIME UP");
            playState = 4;
            break;
           }

           
          lcd.write(p1H);
          lcd.setCursor(2,1);
          lcd.print(printTime(p1M));
          lcd.setCursor(5,1);
          lcd.print(printTime(p1S));

          break;
        }
        case 3: //P2 turn
        {
          lcd.setCursor(9,1);
          p2S += incS;
          p2M += incM;
          if(p2S > 59){
            p2S - 60;
            p2M += 1;
            }
          if(p2M > 59){
            p2M - 60;
            p2H += 1;
            }
            
          if(p2S == 0 and (p2M > 0 || p2H > 0)){
            p2S = 60;
            if(p2M == 0){
                p2H -= 1;
                p2M = 60;
              }
            p2M -= 1;
            p2S -= 1;
            delay(1000);
            }
           else if(p2S > 0)
           {
             p2S -= 1;
             delay(1000);
           }
           else
           {
            lcd.print(">TIME UP");
            playState = 4;
            break;
           }

          lcd.write(p2H);
          lcd.setCursor(11,1);
          lcd.print(printTime(p2M));
          lcd.setCursor(14,1);
          lcd.print(printTime(p2S));
          break;
        }
        case 4:
        {
          lcd.setCursor(11,0);
          lcd.write(byte(2));
          break;
        }
      }
      break;
    case 1: //Options Screen
    {
      lcd.setCursor(0,0);
      lcd.write(byte(3));
      lcd.print(" GAMETIME      ");
      lcd.setCursor(0,1);
      lcd.print("  INCREMENT  DONE");

      //Rotary Encoder Mess
      if((rotaryEVal <= 114)or(rotaryEVal > 342 and rotaryEVal <= 456)or(rotaryEVal > 684 and rotaryEVal <= 798))//selecting Gametime
      {
        lcd.setCursor(1,0);
        lcd.write(byte(5));
        lcd.setCursor(1,1);
        lcd.print(" ");
        lcd.setCursor(11,1);
        lcd.print(" ");

        //Letting button know where cursor is
        cursorState = 0;
      }
      else if((rotaryEVal > 114 and rotaryEVal <= 228)or(rotaryEVal > 456 and rotaryEVal <= 570)or(rotaryEVal > 798 and rotaryEVal <= 912))
      {//selecting Increment
        lcd.setCursor(1,0);
        lcd.print(" ");
        lcd.setCursor(1,1);
        lcd.write(byte(5));
        lcd.setCursor(11,1);
        lcd.print(" ");
        
        //Letting button know where cursor is
        cursorState = 1;
      }
      else if((rotaryEVal > 228 and rotaryEVal <= 342)or(rotaryEVal > 570 and rotaryEVal <= 684)or(rotaryEVal > 912)) //Done
      {
        lcd.setCursor(1,0);
        lcd.print(" ");
        lcd.setCursor(1,1);
        lcd.print(" ");
        lcd.setCursor(11,1);
        lcd.write(byte(5));
        
        //Letting button know where cursor is
        cursorState = 2;
      }
          
      break;
    }
    case 2: //Gametime Change
    {
      lcd.setCursor(1,0);
      lcd.print("   GAMETIME    ");
      lcd.setCursor(0,1);
      lcd.print("    ");
      lcd.print(printTime(startH));
      lcd.print(":");
      lcd.print(printTime(startM));
      lcd.print(":");
      lcd.print(printTime(startS));
      break;

      
      if(rotaryEVal <= 85) //setting gametime, 30sec, 1 min, 2min, 3min, 5min, 10min, 15min, 20min, 30min, 45min, 1hr, 1.5hr
      {
        startH = 0;
        startM = 0;
        startS = 30;
      }
      else if(rotaryEVal > 85 and rotaryEVal <= 170)
      {
        startH = 0;
        startM = 1;
        startS = 0;
      }
      else if(rotaryEVal > 170 and rotaryEVal <= 255)
      {
        startH = 0;
        startM = 2;
        startS = 0;
      }
      else if(rotaryEVal > 255 and rotaryEVal <= 340)
      {
        startH = 0;
        startM = 3;
        startS = 0;
      }
      else if(rotaryEVal > 340 and rotaryEVal <= 425)
      {
        startH = 0;
        startM = 5;
        startS = 0;
      }
      else if(rotaryEVal > 425 and rotaryEVal <= 510)
      {
        startH = 0;
        startM = 10;
        startS = 0;
      }
      else if(rotaryEVal > 510 and rotaryEVal <= 595)
      {
        startH = 0;
        startM = 15;
        startS = 0;
      }
      else if(rotaryEVal > 595 and rotaryEVal <= 680)
      {
        startH = 0;
        startM = 20;
        startS = 0;
      }
      else if(rotaryEVal > 680 and rotaryEVal <= 765)
      {
        startH = 0;
        startM = 30;
        startS = 0;
      }
      else if(rotaryEVal > 765 and rotaryEVal <= 850)
      {
        startH = 0;
        startM = 45;
        startS = 0;
      }
      else if(rotaryEVal > 850 and rotaryEVal <= 935)
      {
        startH = 1;
        startM = 0;
        startS = 0;
      }
      else if(rotaryEVal > 935)
      {
        startH = 1;
        startM = 30;
        startS = 0;
      }
    }
    case 3: //Increment Change
    {
      lcd.setCursor(1,0);
      lcd.print("   INCREMENT   ");
      lcd.setCursor(4,1);
      lcd.print("+ :         ");
      lcd.setCursor(5,1);
      lcd.print(printTime(incM));
      lcd.setCursor(7,1);
      timeTemp = printTime(incS);
      lcd.print(timeTemp);

      if(rotaryEVal <= 85) //setting increment, 0:00, 0:05, 0:10, 0:15, 0:30, 0:45, 1:00, 1:30, 2:00, 3:00, 4:00, 5:00
      {
        incS = 0;
        incM = 0;
      }
      else if(rotaryEVal > 85 and rotaryEVal <= 170)
      {
        incS = 5;
        incM = 0;
      }
      else if(rotaryEVal > 170 and rotaryEVal <= 255)
      {
        incS = 10;
        incM = 0;
      }
      else if(rotaryEVal > 255 and rotaryEVal <= 340)
      {
        incS = 15;
        incM = 0;
      }
      else if(rotaryEVal > 340 and rotaryEVal <= 425)
      {
        incS = 30;
        incM = 0;
      }
      else if(rotaryEVal > 425 and rotaryEVal <= 510)
      {
        incS = 45;
        incM = 0;
      }
      else if(rotaryEVal > 510 and rotaryEVal <= 595)
      {
        incS = 0;
        incM = 1;
      }
      else if(rotaryEVal > 595 and rotaryEVal <= 680)
      {
        incS = 30;
        incM = 1;
      }
      else if(rotaryEVal > 680 and rotaryEVal <= 765)
      {
        incS = 0;
        incM = 2;
      }
      else if(rotaryEVal > 765 and rotaryEVal <= 850)
      {
        incS = 0;
        incM = 3;
      }
      else if(rotaryEVal > 850 and rotaryEVal <= 935)
      {
        incS = 0;
        incM = 4;
      }
      else if(rotaryEVal > 935)
      {
        incS = 0;
        incM = 5;
      }
      break;
    }
  }




}
