#ifndef PWM_2_H
#define PWM_2_H
#include <Arduino.h>


/**********************************************************
                Fast PWM Test
Demostrates the generation of high speed PWM
using timers 1 and 4
There are two pieces of code:
One for pins 9, 10 and 11 using TIMER 1
with frequencies up to 62kHz
Other for pins 6 and 13 using TIMER 4
with frequencies up to 187kHz
History:
  12/12/2014 - Version 1.0
  22/12/2014 - Adding a missing OCR4C value
  
***********************************************************/

/**********************************************************
   Fast PWM on pins 9,10,11 (TIMER 1)
   
   Do not use analogWrite to pins 9, 10 or 11 if using 
   this functions as they use the same timer.
   
   Those functions will probably conflict with the 
   servo library.
   
   Uses 5 PWM frequencies between 61Hz and 62.5kHz
   
**********************************************************/

// Frequency modes for TIMER1
#define PWM62k   1   //62500 Hz
#define PWM8k    2   // 7812 Hz
#define PWM1k    3   //  976 Hz
#define PWM244   4   //  244 Hz
#define PWM61    5   //   61 Hz

// Direct PWM change variables
#define PWM9   OCR1A
#define PWM10  OCR1B
#define PWM11  OCR1C


// Frequency modes for TIMER4
#define PWM187k 1   // 187500 Hz
#define PWM94k  2   //  93750 Hz
#define PWM47k  3   //  46875 Hz
#define PWM23k  4   //  23437 Hz
#define PWM12k  5   //  11719 Hz
#define PWM6k   6   //   5859 Hz
#define PWM3k   7   //   2930 Hz

// Direct PWM change variables
#define PWM6        OCR4D
#define PWM13       OCR4A

// Terminal count
#define PWM6_13_MAX OCR4C

// Macro to converts from duty (0..100) to PWM (0..255)
#define DUTY2PWM(x)  ((255*(x))/100)
#define PWM_FREQ 20000.0f
#define MAXFORCE (F_CPU/(PWM_FREQ*2)) //16000000 is system clock of Leonardo

#define MINFORCE (-MAXFORCE)
#define Dir_X       8
#define Dir_Y       11

class Pwm {
 public:   
   Pwm(void);
   ~Pwm(void);
   void begin();
   void setPWM_X(int16_t forces);
   void setPWM_Y(int16_t forces);
   
   private:
    void pwm91011configure(int mode);
    void pwmSet9(int value);
    void pwmSet10(int value);
    void pwmSet11(int value);
    void pwm613configure(int mode);
    void pwmSet6(int value);
    void pwmSet13(int value);

};

#endif