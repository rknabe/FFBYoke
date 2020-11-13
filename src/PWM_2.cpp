#include "PWM_2.h"
#include "DigitalWriteFast.h"


 Pwm::Pwm(void) {

 }
 
 Pwm::~Pwm() {
	 
 }

void Pwm::begin(){

    pinMode(Dir_X,OUTPUT);
    pinMode(Dir_Y,OUTPUT);
// Configure Timer 1 (Pins 9, 10 and 11)
// It will operate at 62kHz
// Valid options are: 
//      PWM62k, PWM8k, PWM1k, PWM244 and PWM61
pwm91011configure(PWM62k);

// Configure Timer 4 (Pins 6 and 13)
// It will operate at 187kHz
// Valid options are:
//     PWM187k, PWM94k, PWM47k, PWM23k, PWM12k, PWM6k and PWM3k
//pwm613configure(PWM23k);

// Generate PWM at pin 11 with 30% duty
// We need to call pwm91011configure before
// We use here the DUTY2PWM macro
//pwmSet11(DUTY2PWM(30));

// Generate PWM at pin 13 with 75% duty
// We need to call pwm613configure
// We don't here the DUTY2PWM macro
// We set a PWM value (0..255) instead
//pwmSet13(0);

// Pins 9 and 6 will change values in the loop function
// We first configure them

// Prepare pin 9 to use PWM
// We need to call pwm91011configure before
// For now, we set it at 0%
pwmSet9(0);
pwmSet10(0);
// Prepare pin 6 to use PWM
// We need to call pwm613configure
// For now, we set it at 0%
//pwmSet6(0);
  
}

void Pwm::setPWM_X(int16_t forces) {
	int nomalizedForce_X =  map (forces, -255,255, MINFORCE, MAXFORCE); 
	
	if (nomalizedForce_X >= 0) {
        digitalWriteFast(Dir_X,HIGH);
		PWM9 = nomalizedForce_X;
	} else {
		digitalWriteFast(Dir_X,LOW);
		PWM9 = -nomalizedForce_X;
	}

	

	delay(10);
 }

 void Pwm::setPWM_Y(int16_t forces) {
	int nomalizedForce_Y =  map (forces, -255,255, MINFORCE, MAXFORCE); 
		if (nomalizedForce_Y >= 0) {
				digitalWriteFast(Dir_Y,HIGH);
				PWM10 = nomalizedForce_Y;
			} else {
				digitalWriteFast(Dir_Y,LOW);
				PWM10 = -nomalizedForce_Y;
			}
			delay(10);

 }
 

// Configure the PWM clock
// The argument is one of the 5 previously defined modes
void Pwm::pwm91011configure(int mode)
{
// TCCR1A configuration
//  00 : Channel A disabled D9
//  00 : Channel B disabled D10
//  00 : Channel C disabled D11
//  01 : Fast PWM 8 bit

//TCCR1A=1;

TCCR1A = 0b10100000;
TCCR1B = 0b00010001;   
ICR1 = MAXFORCE;
OCR1A = 0;
OCR1B = 0;   

// TCCR1B configuration
// Clock mode and Fast PWM 8 bit
//TCCR1B=mode|0x08;  

// TCCR1C configuration
TCCR1C=0;
}

// Set PWM to D9
// Argument is PWM between 0 and 255
void Pwm::pwmSet9(int value)
{
OCR1A=value;   // Set PWM value
DDRB|=1<<5;    // Set Output Mode B5
TCCR1A|=0x80;  // Activate channel
}

// Set PWM to D10
// Argument is PWM between 0 and 255
void Pwm::pwmSet10(int value)
{
OCR1B=value;   // Set PWM value
DDRB|=1<<6;    // Set Output Mode B6
TCCR1A|=0x20;  // Set PWM value
}

// Set PWM to D11
// Argument is PWM between 0 and 255
void Pwm::pwmSet11(int value)
{
OCR1C=value;   // Set PWM value
DDRB|=1<<7;    // Set Output Mode B7
TCCR1A|=0x08;  // Set PWM value
}

// Configure the PWM clock
// The argument is one of the 7 previously defined modes
void Pwm::pwm613configure(int mode)
{
// TCCR4A configuration
TCCR4A=0;

// TCCR4B configuration
TCCR4B=mode;

// TCCR4C configuration
TCCR4C=0;

// TCCR4D configuration
TCCR4D=0;

// TCCR4D configuration
TCCR4D=0;

// PLL Configuration
// Use 96MHz / 2 = 48MHz
PLLFRQ=(PLLFRQ&0xCF)|0x30;
// PLLFRQ=(PLLFRQ&0xCF)|0x10; // Will double all frequencies

// Terminal count for Timer 4 PWM
OCR4C=255;
}

// Set PWM to D6 (Timer4 D)
// Argument is PWM between 0 and 255
void Pwm::pwmSet6(int value)
{
OCR4D=value;   // Set PWM value
DDRD|=1<<7;    // Set Output Mode D7
TCCR4C|=0x09;  // Activate channel D
}

// Set PWM to D13 (Timer4 A)
// Argument is PWM between 0 and 255
void Pwm::pwmSet13(int value)
{
OCR4A=value;   // Set PWM value
DDRC|=1<<7;    // Set Output Mode C7
TCCR4A=0x82;  // Activate channel A
}

