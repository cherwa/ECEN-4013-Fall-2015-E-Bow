/*
 * Ebow_Main.cpp
 *
 * Created: 10/30/2015 9:42:53 AM
 *  Author: Katie Hobble
 *	With: Trevor Eckert, Ben Wellington, Michael Gotwald
 *
 *  Registers used and their purposes can be found on ATMega328P datasheet
 *  http://www.atmel.com/images/Atmel-8271-8-bit-AVR-Microcontroller-ATmega48A-48PA-88A-88PA-168A-168PA-328-328P_datasheet_Complete.pdf
 */ 

//Include the AVR Libraries
#include <avr/io.h>
#include <avr/interrupt.h>

//Include Ben's LCD Libraries
#include "SPI.h"
#include "Adafruit_ILI9340.h"

//Define F_CPU and include the delay library
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

//I have no idea but I'm afraid to take it out
#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

// These are the pins used for the UNO - Redefine them for LCD display method
// for Due/Mega/Leonardo use the hardware SPI pins (which are different)
// Written by Ben Wellington
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 8
#define _rst 9

//========================
//DECLARE GLOBAL VARIABLES
//========================
// Written by Katie Hobble

//Create a new istance of the LCD Display
Adafruit_ILI9340 lcd = Adafruit_ILI9340(_cs, _dc, _rst);
//Declare global uint8_t health
//::Used to check if the bow is destroyed or not, is decreased in external IR receive interrupt
volatile int health;
//Declare global uint8_t mana
//::Used to check if the bow needs to enter cool down mode, is replenished via
//::timer overflow interrupts, is decreased after firing IR
static int mana;
//Declare global uint8_t manaCount
//::Checked in the timer overflow interrupt, once this is incremented to MANA_INCREASE constant,
//::mana can be increased. Used to control time between mana increases.
static int manaCount;

//LCD variable declarations
// Written by Ben Wellington
int plasma = 1;//bow fired
int ice = 2;//cooldown mode
int power = 3;//drawstrength
int damage = 4;//damage recieved
int fire = 5;//shots fired
int HealthBar = 280; 
int ManaBar = 280; 
int PowerBar = 280;
int Fire = 0;
int Hit = 40;
int Mana = 40;
int Manas = 5;
int Health = 5;
int counter;

//Audio variable declarations
// Written by Michael Gotwald
char soundInput = 1; //draw strength from Trevor
char shoot = 2; //shoot strength from Nick
char cooldown = 3; //Cool down from system
char destroy = 4;  //Destroy from Nick/system

//========================
//DECLARE GLOBAL CONSTANTS
//========================
// Written by Katie Hobble

//Declare constant uin8_t MANA_INCREASE
//::The number of timer overflow interrupts needed before mana can increase by 1
const int MANA_INCREASE = 150;
//Declare constant uint8_t MAX_MANA
//::The maximum amount of mana the bow can have, mana will be initialized this amount
const int MAX_MANA = 10;
//Declare constant uint8_t MIN_MANA
//::The minimum amount of mana needed to exit cool down mode
const int MIN_MANA = 3;
//Declare constant uint8_t MAX_HEALTH
//::The maximum amount of health the bow can have, health will be initialized to this amount
const int MAX_HEALTH = 5;

//Declare all methods
//Written by Katie Hobble
void initialize(void);
void mainLoop(void);
void coolDownMode(void);
bool bowNocked(void);
int measureDrawStrength(void);
int ReadADC(int ADCchannel);
bool isFired(int adcArray[]);
void sendIR(int drawStrength);
void FiveSixK(int count, int pin);
void IR_Receive(void);
void initLCDSetup(void);
void LCDdisplay(int element, int value);
void playAudio(char input, int value);

/*==========================================
int main()
	
	Calls the initialize function then calls 
	the main loop
	
	Written by: Katie Hobble
=============================================*/
int main() {
	//Call the initialize function
	initialize();
	
	//Once all pins, communications, and global variables are initialized
	//enter the main loop of the program
	mainLoop();
	
	//Main loop has exited, this means the bow has no health
	//disable any other interrupts
	cli();
	//Display the bow destroyed screen
	LCDdisplay(4, 0);
	
	//return 0, end main
	return 0;
}//end main

/*=============================
void initialize

inputs: none
returns: n/a

Initializes all global variables,
I/O pins, and the timer0 overflow
interrupt

Written by: Katie Hobble
=================================*/
void initialize() {
	
	//Intialize global variables
	//Written by: Katie Hobble
	//Set health to be MAX_HEALTH
	health = MAX_HEALTH;
	//Set mana to be MAX_MANA
	mana = MAX_MANA;
	//Initialize the manaCount variable to be 0
	manaCount = 0;

	//Initialize ADC for pull sensor
	//Written by Trevor Eckert
	ADMUX = (1<<REFS0); //setting to VCC (01)
	ADCSRA =(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //prescalar by 128 so 125kHz
	DDRC = 0b11011110; //Set PC0 to be input for ADC
	
	//Enable the TIMER0 Overflow Interrupt in the Timer0 Interrupt Mask Register
	//Written by Katie Hobble
	TIMSK0=(1<<TOIE0);
	//Set the prescalar for the Timer Overflow to be 1024 in the Timer/Counter0 Control Register
	//using Clock Select bits 0 and 2 in the TCCR0B register
	TCCR0B = (1<<CS20)|(1<<CS22);
	//Clear the TIMER0 overflow flag in the Timer0 Interrupt Flag Register
	TIFR0 = (1<<TOV0);
	
	/*
	So, below is where I would initialize timer 2 to be used as our IR Receive timer intterupt.
	It reads PC5 at a rate of 112kHz (2x56kHz) but it doesn't work with our current IR send code
	so we took it out :(
	//Initialize Timers
	//Written by Katie Hobble
	//Make timer 2 synchronus with clk
	ASSR = 0x00; 
	//Set counter to correct value. (set by using O-scope to see if the frequency was correct)
	OCR2A = 141;
	//Reset the counter register whenever intterupt is triggered
	TCCR2A |= (1 << WGM21);
	//Set a prescalar of 8
	TCCR2B = 1<<CS00;
	//Enable the timer 2 compare interrupt
	TIMSK2 = 1<< (OCIE2A);
	//Clear the counter
	TCNT2 = 0;*/
	
	//Call this instruction to enable all global interrupts
	//This is an internal instruction to set Bit 7 (global interrupt bit)
	//in the AVR status register (SREG)
	//Written by Katie Hobble
	sei();
	
	//initialize audio
	//Written by Michael Gotwald
	DDRD = 0xFF; //Set all the pins of portD as output
	PORTD = 0x00; //Play the startup sound
	_delay_ms(300); //delay a little bit to ensure it plays
	PORTD = 0xFF; //turn off all sounds
	
	//initialize lcd
	//Written by Ben Wellington
	//Serial is a library that I did not write]
	Serial.begin(115200);
	//Begin the lcd instance
	lcd.begin();
	//call the initLCDSetup method to draw the bow's initial screen
	initLCDSetup();
}//end initialize

/*=============================
void mainLoop

inputs: none
returns: n/a

Creates a loop while health > 0 that first
checks to see if mana is above zero,
then if it is not it calls coolDownMode(),
otherwise it polls the bowNocked method
to check for user input. If bowNocked is
TRUE, get the drawStrength in a local uint8_t
by calling drawStrength(), and calls sendIR() using
the drawStrength variable.

Written by: Katie Hobble
=================================*/
void mainLoop() {
	//Create an infinite loop to run while the bow is in use
	while(health > 0) {
		//First, check to see if mana is less than 0. If it is, enter
		//cool down mode to replenish mana
		if(mana <= 0) {
			//Mana needs to be replenished, enter cool down mode.
			coolDownMode();
		}//end if
		
		//We get here only if mana is at an acceptable amount.
		//Poll the bowNocked method to see if there has been any user input
		if(bowNocked()) {
	
			//Play the bow nocked sound
			playAudio(soundInput, 0);
			//User input was detected by bowNocked() method. Call the
			//drawStrength() method to get the draw strength of the user's
			//shot in the form of a uint8_t
			int drawStrength = measureDrawStrength();
			
			//Pass the uint8_t drawStrength to the sendIR method if its greater than 0
			if(drawStrength > 0) {
				//Display the bow's firing power
				LCDdisplay(fire,drawStrength);
				//Send the damage packets
				sendIR(drawStrength);
			}//end if
		}//end if
	}//end while
	
	//Health is less than zero, send out some stun packets through PC3 (short range)
	for(int i = 0; i < 5; i++){
		//Send the start envelope
		FiveSixK(10, 0x08);
		//Send the first stun data envelpe
		FiveSixK(40, 0x08);
		//Repeat the stun data envelope
		FiveSixK(40, 0x08);
		//Send stop envelope
		FiveSixK(150, 0x08);
		//Delay a little bit
		_delay_ms(50);
	}//end for
}//end mainLoop

/*=============================
ISR(TIMER0_OVF_vect)

inputs: none
returns: n/a

Triggers when 8-bit timer0 reaches 256
Checks to see if mana can be increased
if manaCount is at the appropriate value
(MANA_INCREASE) and if mana is not at its
full value (MAX_MANA). If both those conditions
are true, then it increases mana by 1 and resets
the manaCount variable. Otherwise, if mana
is not at its maximum value and manaCount has not
reached the increase value, it increases manaCount by 1.

Written by: Katie Hobble
Source: http://efundies.com/avr/avr_timer_interupts_c_simple.htm
=================================*/
ISR(TIMER0_OVF_vect) {
	//Check to see if manaCount is at the increase amount (MANA_INCREASE) and
	//if mana is able to be increased (it is not at MAX_MANA)
	if (manaCount == MANA_INCREASE && mana < MAX_MANA) {
		//Reset manaCount to 0
		manaCount = 0;
		//Increase mana by 1
		++mana;
		//Display the new mana
		LCDdisplay(plasma, mana);
	}
	else {
		//If mana is able to be increased, increase the manaCount by 1 to continue the
		//counter
		if(mana < MAX_MANA) {
			//Increase manaCount by 1
			++manaCount;
		}//end if
	}//end if
}//end ISR(TIMER0_OVF_vect)

/*=============================
void coolDownMode

inputs: none
returns: n/a

Creates a loop that checks to see if mana
is at the minimum value needed to exit cool
down mode (MIN_MANA). If it is, the loop
exits and the method is finished. It does
not need to do anything inside the loop
because the overflow ISR will automatically
replenish mana.

Written by: Katie Hobble
=================================*/
void coolDownMode() {
	//Reset manaCount to ensure we are starting from the beginning
	manaCount = 0;
	//Play audio and display visual for cool down mode
	LCDdisplay(ice, 0);
	playAudio(cooldown, 0);
	
	//This will check to see if mana is at the minimum
	//value it needs to be to exit cool down mode
	while(mana < MIN_MANA) {
		//Do nothing, let timer overflow do its thing
		_delay_ms(8000);
		//Clear the LCD display	
		LCDdisplay(10,0);
				
	}//end while
	
	//Turn off sound (in case the cooldown sound is trying to loop)
	PORTD = 0xFF;
	//Because IR receive doesn't work, we are going to decrement health every time cooldown mode is entered
	//decrement health
	health--;
	
	//If health has reached zero, just turn off interrupts
	//Otherwise, display the health
	if(health<0){
		//Clear the interrupts
		cli();
	}
	else{
		//Display new health
		LCDdisplay(damage, health);
	}//end if
}//end coolDownMode()

/*************************************************
===================================================
             BEGIN PULL SENSOR SECTION
===================================================
***************************************************/

/*=============================
bool bowNocked

inputs: none
returns: True/False

Checked the ADC pin value to see if
it has reached the nock threshold.
If it has, return true, otherwise
return false.

Written by: Trevor Eckert
=================================*/
bool bowNocked() {
		//create a variable strength that reads the ADC
		int strength = ReadADC(0);
		//Set the nock threshold (we played with this for weeks and we found this value is best)
		int nockThresh = 280;
		//If the strength is less than the nock threshold, return true, otherwise return false
		//The strength is less than the nock thresh because as the string is pulled, resistance
		//increses and digital values (voltage) decrease
		if (strength < nockThresh){
			//nock thresh met, return true
			return true;
		}
		else {
			//nock thresh not met, return false
			return false;
		}//end if
}//end bowNocked

/*=============================
int measureDrawStrength()

inputs: none
returns: int drawStrength

Reads in the current ADC while the bow
has not fired, and returns the bow's
draw back strength on a scale of 1 to 4
while the bow is being drawn back and 
when the bow fires

Written by: Trevor Eckert
Assistd by: Katie Hobble
=================================*/
int measureDrawStrength() {
	//range values
	int adc_val;
	int nockThresh = 280;
	int returnValue = 0;
	
	//adc values at each window
	int oneThresh = nockThresh - 5; //275
	int twoThresh = nockThresh - 35; //245
	int threeThresh = nockThresh - 55; //225
	int fourThresh = nockThresh - 75; //205
	
	//an array of the adc values initilized with the same threshold
	//value to make the initial slope zero
	int adcArray[6] = {nockThresh,nockThresh,nockThresh,nockThresh,nockThresh,nockThresh};
	
	//get the adc value
	adc_val = ReadADC(0);
	
	//continue to read in adc while the bow hasn't been fired
	while(!isFired(adcArray)){
		//get the adc value
		adc_val = ReadADC(0);
		//this if statment checks to see if the bow has been returned to the initial state without being fired
		if(!bowNocked() && adcArray[0] < nockThresh) {
			//bow has returned, set the return value to zero
			returnValue=0;
			//clear the power bar
			LCDdisplay(power, 0);
			//turn off sound
			PORTD = 0xFF;
			//break out of the loop
			break;
		}//end if
	
		//play drawback audio if the bow hasn't been fired or returned
		if(returnValue > 0) {
			playAudio(soundInput, returnValue);
		}//end if
		
		//compare the adc values to the thresholds and set display its corresponding power
		//also set the return value to be what that power is
		
		if (adc_val <= oneThresh && adc_val > twoThresh) {
			//Call visual passing 1
			LCDdisplay(power,1);
			returnValue = 1;
		}
		else if (adc_val <= twoThresh && adc_val > threeThresh) {
			//Call visual passing 2
			LCDdisplay(power,2);
			returnValue = 2;
		}
		else if (adc_val <= threeThresh && adc_val > fourThresh) {
			//Call visual passing 3
			LCDdisplay(power,3);
			returnValue = 3;
		}
		else if(adc_val <= fourThresh) {
			//Call visual passing 4
			LCDdisplay(power, 4);
			returnValue = 4;
		}//end if
		
		//create a temporary array with the new adc_val
		int temp[6] = {adc_val, adcArray[0],adcArray[1],adcArray[2],adcArray[3],adcArray[4]};
		//set adcArray to be the temporary array
		for(int i = 0; i < 6; i ++) {
			adcArray[i] = temp[i];
		}//end for
	}//end while
	
	//Return the final ADC value by looking at thresholds
	//we look at adcArray[5] to see where the bow was at when it was fired
	if(adcArray[5] <= oneThresh && adcArray[5] > twoThresh) {
		returnValue = 1;
	}
	else if(adcArray[5] <= twoThresh && adcArray[5] > threeThresh) {
		returnValue = 2;
	}
	else if (adcArray[5] <= threeThresh && adcArray[5] > fourThresh) {
		returnValue = 3;
	}
	else if(adcArray[5] <= fourThresh) {
		returnValue = 4;
	}//end if
	
	return returnValue;
}//end measureDrawStrength

/*=============================
void coolDownMode

inputs: int ADCchannel
returns: ADC

Reads the ADC pin and returns its
digital value

Written by: Trevor Eckert
Sources: http://extremeelectronics.co.in/avr-tutorials/using-adc-of-avr-microcontroller/
		 http://maxembedded.com/2011/06/the-adc-of-the-avr/
=================================*/
int ReadADC(int ADCchannel) {
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while ((ADCSRA & (1<<ADSC)));
	{
		
	}//end while
	return ADC;
}//end ReadADC

/*=============================
bool isFired

inputs: int adcArray[]
returns: True/False

Calculates the slope of the most
recent ADC values to determine whether
or not the bow has been fired based on 
a pre-determined slope

Written by: Trevor Eckert
=================================*/
bool isFired(int adcArray[]) {
	//Some slope threshold to make a decision
	int slopeThresh = 35;
	
	//calculate the average slope of the past three adc values
	int slope = ((adcArray[0]-adcArray[5]));
	
	//int slope = (adcArray[1] - adcArray[0])/2;
	//Less than nocked...
	if(slope >= slopeThresh){
		return true;
	}
	else{
		return false;
	}//end if
}//end isFired

/*************************************************
===================================================
                 BEGIN IR SECTION
===================================================
***************************************************/
	
/*=============================
void sendIR

inputs: int drawStrength
returns: n/a

Fires a damage packet for every increment
of drawStrength. Also decreases mana based
on the drawStrength.

Written by: Katie Hobble
=================================*/
void sendIR(int drawStrength) {
	//Decrease mana if the strength is greater than 0
	if (drawStrength > 0) {
		//Decrease mana according to its value (it'll be either 0 or mana-drawStrength)
		if(mana - drawStrength < 0) {
			//Set mana to be zero if it was going to be negative
			mana = 0;
			manaCount = 0;
		}
		else {
			//Set mana to be itself minus the drawstrength
			mana = mana - drawStrength;
			manaCount = 0;
		}//end if
		
		//Play the firing sound
		playAudio(shoot, drawStrength);
		//Delay for a little to let it play
		_delay_ms(300);
		//Turn off sound
		PORTD = 0xFF;
		
		//Send Damage Packet to PC4 (long range)
		for (int i = 0; i < drawStrength; i ++) {
			//Send start envelope
			FiveSixK(10, 0x10);
			//Send damage data envelope
			FiveSixK(20, 0x10);
			//Send damage repeat data evelope
			FiveSixK(20, 0x10);
			//Send stop envelope
			FiveSixK(150, 0x10);
			//Delay for a little
			_delay_ms(50);
		}//end for
		
		//Display the new mana
		LCDdisplay(plasma, mana);
	}//end if
}//end sendIR

/*=============================
void FiveSixK

inputs: int count
returns: n/a

Sends a 56 KHz wave high for the 
duration of count and low for the duration
of 150-count
=================================*/
void FiveSixK(int count, int pin) {
	//Get the wait time, which is 150-count
	int invCount = 150 - count;
	
	//Send count number of packets (MIRP protocol)
	while(count > 0) {
		//Turn on pin
		PORTC |= pin;
		//Figured out delay with scope to get proper frequency
		_delay_us(8.66);
		//Turn off pin
		PORTC &= !pin;
		//Figured out delay with scope to get proper frequency
		_delay_us(8.66);
		
		count--;
	}//end while
	
	//Wait 150 - count packets (MIRP protocol
	while(invCount > 0) {
		_delay_us(17.32);
		invCount --;
	}//end while
}//end FiveSixK

/*
Here lies or IR Receive Timer2 compare interrupt. RIP

//Set this to be true once pin goes low (it's an active low receiver)
volatile bool hasIR = false;
//Counts the packets we receive
volatile int receiveCounter=0;
//Counts the nubmer of times the pin has been read
volatile int otherCounter=0;
/*====================================
ISR(TIMER2_COMPA_vect)

Reads in the receive pin at a frequency of 112 kHz
If the pin is low, a 56 kHz packet was received so
it increments the receive counter. It then looks
at the receive counter to see if it has received a
damage packet. It then decrements health if so.

Written by: Katie Hobble
Assisted by: Matt Litchfield (I bounced ideas off him and he explained Timer 2 to me)
======================================*/
/*
ISR(TIMER2_COMPA_vect) {
	//If we don't have IR but the pin goes low, set hasIR to be true
	if(!hasIR &&!(PINC && 0x20)) {
		hasIR=true;
	}//end if
	//If we do have IR, count if we have a MIRPS packet
	if(hasIR){
		//if the pin is low, increment the receive counter
		if(!(PINC && 0x20)) {
			receiveCounter ++;
		}
		//increment the overall counter
		otherCounter++;
	}//end if
	//If the overall counter has reached the number of packets in 4 elvelopes (150*4 = 600), the entire packet is over
	//so count the received packets
	if(otherCounter>600){
		//Damage packets send 10+20+20 = 50 packets, so check if the receive counter is between a certain number of them
		//note, receive counter will be 2 * the number of packets due to the timer's frequency
		if(receiveCounter > 90 && receiveCounter < 120){
			//Decrement health, damage received
			health--;
			//If health is zero, just clear all interrupts, otherwise display new health
			if(health <= 0) {
				cli();
			}
			else {
				LCDdisplay(damage, health);
			}//end if
		}//end if
		//Reset all variables
		hasIR=false;
		receiveCounter=0;
		otherCounter=0;
	}//end if
	
	
}//end ISR(TIMER2_COMPA_vect)
*/

/**************************************************
===================================================
             BEGIN LCD DISPLAY SECTION
===================================================
***************************************************/

/*=============================
void initLCDSetup

inputs: none
returns: n/a

Initializes the LCD screen to display
all the proper startup shapes and values

Written by: Ben Wellington
Source: https://github.com/adafruit/Adafruit_ILI9340 (the ILI9340 libraries provided by adafruit)
=================================*/
void initLCDSetup() {
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
	for(int i = 0; i<=health;i++) {
		lcd.print("\3");//heart character
		
	}//end for
	
	//Mana Bar
	lcd.setCursor(300, 125);
	lcd.setTextColor(ILI9340_GREEN);
	lcd.setTextSize(4);
	for(int i = 0; i<=mana;i++){
		lcd.print("\5");//mana/tree character
	}//end for
	
	lcd.setCursor(0, 210);
	lcd.setTextColor(ILI9340_WHITE);
	lcd.setTextSize(4);
	lcd.print("Power:");
	
}//end initLCDDisplay

/*=============================
void LCDdisplay

inputs: int element, int value
returns: n/a

Based on the element, the LCD display
updates the needed value to be changed.
This function speaks to the LCD display 
and tells it what to draw

Written by: Ben Wellington
Source: https://github.com/adafruit/Adafruit_ILI9340 (the ILI9340 libraries provided by adafruit)
=================================*/
void LCDdisplay(int element, int value){

	Serial.print(F("Test "));//simple communication test

	if(element==ice) {
		//display power as 0 and mana as value and print (Cooldown) mode in blue
		lcd.fillRect(0, 155, 280, 55, ILI9340_BLACK);
		lcd.setCursor(300, 112);
		lcd.setTextColor(ILI9340_CYAN);
		lcd.setTextSize(2);
		lcd.print("           (Cooldown)");
		
		lcd.setCursor(300, 125);
		lcd.setTextColor(ILI9340_GREEN);
		lcd.setTextSize(4);
		for(int i = 0; i<=value;i++) {
			lcd.print("\5");//mana/tree character
		}//end for
		//lcd.fillRect(ManaBar, 148, 280, 52, ILI9340_BLACK);
	}
	else if(element == power) {
		//display power as value
		lcd.fillRect(5, 245, (value*59), 60, ILI9340_YELLOW);
		lcd.fillRect(((value*59)+5), 245, 280, 60, ILI9340_BLACK);
	}
	else if(element == damage) {
		if(value ==0){
			//display health as zero and then print bow is dead
			lcd.fillScreen(ILI9340_BLACK);
			lcd.setCursor(0, 15);
			lcd.setTextColor(ILI9340_WHITE);
			lcd.setTextSize(4);
			lcd.print("   Your   weapon has  no more health and  is now     dead! ");
			lcd.setTextColor(ILI9340_RED);
			lcd.print("     :( ");
		}
		else {
			//display health as value
			lcd.fillRect(0, 50, 280, 60, ILI9340_BLACK);
			lcd.setCursor(300, 0);
			lcd.setTextColor(ILI9340_RED);
			lcd.setTextSize(7);
			for(int i = 0; i<=value;i++){
				lcd.print("\3");//heart character
			}//end for
		}//end if
	}
	else if(element == plasma) {
		//bow has been fired
		lcd.fillRect(0, 245, 280, 60, ILI9340_BLACK);
		lcd.fillRect(0, 156, 280, 40, ILI9340_BLACK);//Paint over mana
		lcd.setCursor(300, 125);
		lcd.setTextColor(ILI9340_GREEN);
		lcd.setTextSize(4);
		for(int i = 0; i<=value;i++){
			lcd.print("\5");//mana/tree character
		}//end for
		//dipslay power as zero and retrieve mana or use value
	}
	else if(element == 10) {
		//this is for when exiting cooldown 
		//erase (cooldown) message
		lcd.fillRect(120, 120, 280, 30, ILI9340_BLACK);
	}
	else if(element == 5) {
		//show power bar as red because bow was fired
		lcd.fillRect(5, 245, (value*59), 60, ILI9340_RED);
		lcd.fillRect(((value*59)+5), 245, 280, 60, ILI9340_BLACK);
	}
	else {
		//invalid data received
		lcd.fillScreen(ILI9340_BLACK);
		lcd.setCursor(0, 15);
		lcd.setTextColor(ILI9340_WHITE);
		lcd.setTextSize(4);
		lcd.print("Invalid data was passed as an element.");
	}//end if
	
	Serial.println(F("Done!"));
}//LCDDisplay

/*************************************************
===================================================
                BEGIN AUDIO SECTION
===================================================
***************************************************/

/*=============================
void playAudio

inputs: char input, int value
returns: n/a

Plays the necessary audio for the
specified input and value.

Written by: Michael Gotwald
=================================*/
void playAudio(char input, int value) {
	//Clear any interrupts while this is working
	cli();
	
	// The soundInput is read from Trevor
	if(input == soundInput && value == 0) {
		//draw of 0 = nock
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00000111; //nock sound LOW to play
	}
	else if(input == soundInput && value == 1) {
		//draw of 1 = small draw
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00000011; //turn on draw sound
	}
	else if(input == soundInput && value == 2) {
		//draw of 2 = med draw (small draw sound)
		PORTD = 0b11111111; //turn off all buttons
		PORTD = 0b00000011; //turn on draw sound
	}
	else if(input == soundInput && value == 3) {
		//draw of 3 = far draw
		PORTD = 0b11111111; //turn off all buttons
		//turn on loud draw sound
		PORTD = 0b00001111;
	}
	else if(input == soundInput && value == 4) {
		//draw of 4 = extreme draw
		PORTD = 0b11111111; //turn off all buttons
		PORTD = 0b00001111; //turn on loud draw sound
	}
	else if(input == shoot && value == 1) {
		//read from Katie for small fire
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00111111; //turn on quiet shoot sound
	}
	else if(input == shoot && value == 2) {
		//read from Katie for small fire
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00111111; //turn on quiet shoot sound
	}
	else if(input == shoot && value == 3) {
		//read from Katie for large fire
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b01111111; //turn on loud shoot sound
	}
	else if(input == shoot && value == 4) {
		//read from Katie for large fire
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b01111111; //turn on loud shoot sound
	}
	else if(input == cooldown) {
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00011111; //Turns on cool down mode
	}
	else if(input == destroy) {
		PORTD = 0b11111111; //Turns off all sounds
		PORTD = 0b00000011; //Turns on destroy mode
	}//end if
	
	//Enable interrupts again
	sei();
}//end playAudio
