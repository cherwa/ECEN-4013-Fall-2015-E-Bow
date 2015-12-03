/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
//Beginning of Auto generated function prototypes by Atmel Studio
void setup();
void loop();
void LCDdisplay(int element, int value);
unsigned long initLCDSetup();
//End of Auto generated function prototypes by Atmel Studio


#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

// These are the pins used for the UNO
// for Due/Mega/Leonardo use the hardware SPI pins (which are different)
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 8
#define _rst 9
#define firedPin 3
#define damadgePin 2

//Adafruit_ILI9340 lcd = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
//Setting up "lcd" as the Adafruit_ILI9340 variable for access to the libraries
//Adafruit_ILI9340 is the library required to communicate with the lcd display I am using but I did not write it
Adafruit_ILI9340 lcd = Adafruit_ILI9340(_cs, _dc, _rst);


  //variable declarations
  boolean now = true;
  
  int plasma = 1;//bow fired
  int ice = 2;//cooldown mode
  int power = 3;//drawstrength
  int damadge = 4;//damadge recieved
  
  int HealthBar = 280; // 
  int ManaBar = 280; // 
  int PowerBar = 280;
  int Fire = 0;
  int Hit = 40;
  int Mana = 40;
  int Manas = 5;
  int Health = 5;

  int tempMana = 5;
  int tempPower = 0;
void setup() 
{
  //establish Serial Connections
  //Serial is a library that I did not write
  Serial.begin(9600);  
  //Test text
  Serial.println("Adafruit 2.2\" SPI lcd Test!");
  //initialize lcd
  lcd.begin();
  //lcd.setRotation(3);

  Serial.print(F("Text "));
  /////////////////////////////////////////////////////////////////////////////////////////
  Serial.println(initLCDSetup());//This is necessary for the initial setup of the display//
  delay(1000);/////////////////////////////////////////////////////////////////////////////

}

/**
 * Call LCDdisplay with (constant: damadge,value of total Health) when damadge is recieved. I am expecting values from 0 to 5
 * 
 * Call LCDdisplay with (constat: power, power level) when bow is drawn back. I am expecting values from 0 to 4
 * 
 * Call LCDdisplay with (constatnt: ice, value of mana) for cooldown. I am expecting values from 0 to 10
 * 
 * Call LCDdisplay with (constatnt: plasma, value of mana) when bow is fired.
 * 
 * Call LCDdisplay with (10, 0) in order to exit cooldown.
 * 
 * When bow is dead just call with (constant: damadge,value of total Health) and death display will be handled
 */
void loop() {
    //checking 
    
    LCDdisplay(power,tempPower);//full power
    //delay is a fuction that I did not write
    delay(1000);//this delay is only in use for debugging
    tempPower++;
    if(tempPower==5){tempPower=0;}

    
    LCDdisplay(plasma, tempMana);
    delay(1000);
    tempMana++;


    LCDdisplay(ice, 0);
    delay(1000);
  
    LCDdisplay(damadge, Health);
    delay(1000);
    Health--; 
    if(Health==0){Health=5;}

    if(tempMana == 11)
      {tempMana = 0;}

    if(tempMana<=3){//exit cooldown
    LCDdisplay(10,0);
    delay(1000);
    }
}

/////////////////////////////////////////////////////////
void LCDdisplay(int element, int value){

Serial.print(F("Test "));//simple communication test

    if(element==ice){
    
        lcd.fillRect(0, 155, 280, 55, ILI9340_BLACK);
        lcd.setCursor(300, 112);
        lcd.setTextColor(ILI9340_CYAN);
        lcd.setTextSize(2);
        lcd.print("           (Cooldown)");
        
        lcd.setCursor(300, 125);
        lcd.setTextColor(ILI9340_GREEN);
        lcd.setTextSize(4);
        for(int i = 0; i<=value;i++){
        lcd.print("\5");//mana/tree character
        }
      //lcd.fillRect(ManaBar, 148, 280, 52, ILI9340_BLACK);
      //display power as 0 and mana as value and print (Cooldown) mode in blue
    }
    
    else if(element == power){
      lcd.fillRect(5, 245, (value*59), 60, ILI9340_YELLOW);
      lcd.fillRect(((value*59)+5), 245, 280, 60, ILI9340_BLACK);
      //display power as value
    }
    
    else if(element == damadge){
    
      if(value ==0){
        //display health as zero and then print bow is dead
       
        lcd.fillScreen(ILI9340_BLACK);
        lcd.setCursor(0, 15);
        lcd.setTextColor(ILI9340_WHITE);
        lcd.setTextSize(5);
        lcd.print("Your weapon has no more health and is now dead!");
        lcd.setTextColor(ILI9340_RED);
        lcd.print("   XP ");
      }
      else{
         //display health as value
        lcd.fillRect(0, 50, 280, 60, ILI9340_BLACK);
        lcd.setCursor(300, 0);
        lcd.setTextColor(ILI9340_RED);
        lcd.setTextSize(7); 
        for(int i = 0; i<=value;i++){
          
           lcd.print("\3");//heart character
        
          }  
      }  
    }
    else if(element == plasma){//bow has been fired
      
        lcd.fillRect(0, 245, 280, 60, ILI9340_BLACK);
        lcd.fillRect(0, 156, 280, 40, ILI9340_BLACK);//Paint over mana
        lcd.setCursor(300, 125);
        lcd.setTextColor(ILI9340_GREEN);
        lcd.setTextSize(4);
        for(int i = 0; i<=value;i++){
          lcd.print("\5");//mana/tree character
        }
      //dipslay power as zero and retrieve mana or use value
      }
    else if(element == 10)//this is for when exiting cooldown **************COOLDOWN****************
         { //erase (cooldown) message
           lcd.fillRect(120, 120, 280, 30, ILI9340_BLACK);        
         }
    else{
          lcd.fillScreen(ILI9340_BLACK);
          lcd.setCursor(0, 15);
          lcd.setTextColor(ILI9340_WHITE);
          lcd.setTextSize(4);
          lcd.print("Invalid data was passed as an element.");

      }
    
      Serial.println(F("Done!"));
    }


///////////////////////////////////////////////////////////////////////////
//setup the location of all the values and prepare display for the game
unsigned long initLCDSetup() 
{
  
      lcd.fillScreen(ILI9340_BLACK);
      unsigned long start = micros();//micros is a function that I did not write
       //Write Health
      lcd.setCursor(0, 22);
      lcd.setTextColor(ILI9340_WHITE);
      lcd.setTextSize(4);
      lcd.print("Health:");
      
      //write Mana
      lcd.setCursor(0, 125);
      lcd.setTextColor(ILI9340_WHITE);
      lcd.setTextSize(4);
      lcd.print("Mana:");
    
      // Health Hearts
      lcd.setCursor(300, 0);
      lcd.setTextColor(ILI9340_RED);
      lcd.setTextSize(7); 
      for(int i = 0; i<=Health;i++){
        lcd.print("\3");//heart character
    
       }

    
      //Mana Bar   
      lcd.setCursor(300, 125);
      lcd.setTextColor(ILI9340_GREEN);
      lcd.setTextSize(4);
      for(int i = 0; i<=10;i++){
        lcd.print("\5");//mana/tree character
      }
      //lcd.fillRect(0, 155, ManaBar, 20, ILI9340_BLUE);
      
      lcd.setCursor(0, 210);
      lcd.setTextColor(ILI9340_WHITE);
      lcd.setTextSize(4);
      lcd.print("Power:");
      return micros() - start;
      
}




