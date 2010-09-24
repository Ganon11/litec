/*
 * Names: Michael Stark + David Melecio-Vázquez
 * Section: 4 A
 * Date: 14 September 2010
 * File name: lab1-2.c
 * Description: Play a simple game with the user, involving Timer0,
 * pushbuttons, LEDs, a BiLED, a slide switch, and the microcontroller.
 */
/*
 * This program demonstrates the use of T0 interrupts. The code will count the
 * number of T0 timer overflows that occur while a slide switch is in the ON position.
 */

#include <c8051_SDCC.h>// include files. This file is available online
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);      // Initialize ports for input and output
void Timer_Init(void);     // Initialize Timer 0 
void ADC_Init(void);       // Initialize A/D Conversion
void ADC_Test(void);       // Performs A/D Conversion
void Interrupt_Init(void); // Initialize interrupts
void Timer0_ISR(void) interrupt 1; // Called at Timer0 overflow
unsigned char unique_random(unsigned char last_state); // Generates a random number different from the last one
unsigned char random(unsigned char N); // Generates a random number between 0 and N-1
void wait_one_second(void); // Waits 1 second
void flash_lights(void); // Flashes BiLED green and red while game is paused.
void light_green(void); // Light BiLED green
void light_red(void); // Light BiLED red
int CheckPushButton1(void); // function which checks CheckPushButton1
int CheckPushButton2(void); // function which checks CheckPushButton1
int CheckSlideSwitch(void); // function that checks the Slide switch
void PrintInputStatus(int slide, int push1, int push2); // Helper function to print the status of the inputs.
char* newline(); // Helper function, used when printing (returns "\r\n")

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

sbit at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
sbit at 0xB0 PB1; // Push button 0, associated with Port 3, Pin 0
                  // Black PB is PB1
sbit at 0xB1 PB2; // Push button 1, associated with Port 3 Pin 1
                  // Red PB is PB2
sbit at 0xB3 BILED0; // BILED0, associated with Port 3 Pin 3
sbit at 0xB4 BILED1; // BILED1, associated with Port 3 Pin 4
sbit at 0xB5 LED1; // LED1, associated with Port 3 Pin 5
sbit at 0xB6 LED0; // LED0, associated with Port 3 Pin 6
sbit at 0xB7 BUZZER; // Buzzer, associated with Port 3 Pin 7

unsigned int Counts = 0;


//***************
void main(void) {
  // Local Variable Declarations
  unsigned char turns[10];
  int i, push1, push2, correct;

  Sys_Init();      // System Initialization
  Port_Init();     // Initialize ports 2 and 3 
  Interrupt_Init();
  Timer_Init();    // Initialize Timer 0 
  ADC_Init();      // Initialize A/D Conversion
  putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
  putchar('\r');

  // ************************************************
  // BASIC SIMON GAME
  // ************************************************

  // Enable Timer 0
  TR0 = 1;

}

//***************
void Port_Init(void) {
  // Port 1 Constant Masks
  unsigned char P1MD1IN &= 0xFD; // 1111 1101, Set P1.1 as an analog input 
  unsigned char P1MDOUT &= 0xFD; // 1111 1101, Set P1.1 as a input port bit
  unsigned char P1 |= 0x02 // 0000 0010 Set P1.1 to a high impedance state


  // Port 2 Constant Masks
  unsigned char P2MDOUT_HI = 0x00; // 0000 0000 - unused in lab 1
  unsigned char P2MDOUT_LO = 0xFE; // 1111 1110
  unsigned char P2_HI = 0x01; // 0000 0001
  
  // Port 3 Constant Masks
  unsigned char P3MDOUT_HI = 0xF8; // 1111 1000
  unsigned char P3MDOUT_LO = 0xFC; // 1111 1100
  unsigned char P3_HI = 0x03; // 0000 0011
  
  // Set Port 2 MDOUT high bits
  P2MDOUT |= P2MDOUT_HI; // in lab 1, does nothing
  // Set Port 2 MDOUT low bits
  P2MDOUT &= P2MDOUT_LO;
  // Set Port 2 impedence (high) bits
  P2 |= P2_HI;
  
  // Set Port 3 MDOUT high bits
  P3MDOUT |= P3MDOUT_HI;
  // Set Port 3 MDOUT low bits
  P3MDOUT &= P3MDOUT_LO;
  // Set Port 2 impedence (high) bits
  P3 |= P3_HI;
}

void Interrupt_Init(void) {
  IE |= 0x82; // 1000 0010
}

//***************
void Timer_Init(void) {
  CKCON |= 0x08; // 0000 1000 (use SYSCLK)
  TMOD &= 0xF0; // clear the 4 least significant bits (1111 0000)
  TMOD |= 0x01; // 0000 0001 (Timer0 in mode 1, for 16-bit)
	TR0 = 0; // Stop Timer0
	TL0 = 0; // Clear low byte of register T0
	TH0 = 0; // Clear high byte of register T0
}
void ADC_Init(void) {
  REF0CN &= 0XF7 // 1111 0111 Configure ADC1 to use VREF
  REF0CN |= 0x03 // 0000 0011
  ADC1CF =0x01 // 0000 0001 Set a gain of 1
  ADC1CN |= 0x80 // 1000 0000 Enable ADC1
}
void ADC_Test(void) {
  AMX1SL = 0x01; // 0000 0001 Set the Port pin number
  ADC1CN &= 0xDF; // 1101 1111 Clear the flag from the previous ADC1 conversion
  ADC1CN |= 0x10; // 0001 0000 Start A/D Conversion
  while ((ADC1CN & 0x20) == 0x00){ // Wait for conversion to be complete
  	P1_1_RESULT = ADC1; //Assign the A/D conversion result
  }
}
//***************
void Timer0_ISR(void) interrupt 1 {
  TF0 = 0; // clear interrupt request
  Counts++;
}

/******************************************************************************/

/*
 * Returns a random number different from last_state.
 */
unsigned char unique_random(unsigned char last_state) {
  unsigned char return_value;
  do {
    return_value = random(3);
  } while(return_value == last_state);
  return return_value;
}

/*
 * Returns a random integer between 0 and N-1 (a range of N numbers).
 */
unsigned char random(unsigned char N) {
  return (rand() % N);
}

/*
 * Waits 1 second.
 */
void wait_one_second(void) {
  Counts = 0;
  while (Counts < 338);
}

/*
 * Flash lights green and red while the slide switch is off.
 */
void flash_lights(void) {
  int original_value = Counts;
  while (1) {
    light_green();
    // Wait 0.5 seconds, or return if slide switch is on
    Counts = 0;
    while (Counts < 169) {
      if (CheckSlideSwitch()) {
        // Turn off the BiLED
        BILED0 = 1;
        BILED1 = 1;
        Counts = original_value;
        return;
      }
    }
    light_red();
    // Wait 0.5 seconds, or return if slide switch is on
    Counts = 0;
    while (Counts < 169) {
      if (CheckSlideSwitch()) {
        // Turn off the BiLED
        BILED0 = 1;
        BILED1 = 1;
        Counts = original_value;
        return;
      }
    }
  }
}

/*
 * Light BiLED green.
 */
void light_green(void) {
  BILED0 = 1;
  BILED1 = 0;
}

/*
 * :Light BiLED red.
 */
void light_red(void) {
  BILED0 = 0;
  BILED1 = 1;
}

/*
 * Returns a 0 if CheckPushButton1 not activated
 * or a 1 if CheckPushButton1 is activated.
 * This code reads a single input only, associated with PB1
 */
int CheckPushButton1(void) {
  // !PB1 will evaluate to 1 if PB1 is off, or 0 if PB1 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB1;
}

/*
 * Returns a 0 if CheckPushButton2 not activated
 * or a 1 if CheckPushButton2 is activated.
 * This code reads a single input only, associated with PB2
 */
int CheckPushButton2(void) {
  // !PB2 will evaluate to 1 if PB2 is off, or 0 if PB2 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB2;
}

/*
 * Returns a 0 if the Slide switch is ‘off’
 * or a 1 if the Slide switch if ‘on’.
 * This code reads a single input only, associated with SS
 */
int CheckSlideSwitch(void) {
  // !SS will evaluate to 1 if SS is off, or 0 if SS is on.  These are the
  // desired return values, so we'll just return the statement.
  return !SS;
}

/*
 * Prints a message to standard output indicating the status of the Slide
 * switch and Pushbuttons 1 and 2.
 */
void PrintInputStatus(int slide, int push1, int push2) {
  // Print the Slide Switch status
  if (slide) {
    printf("Slide switch on, ");
  } else {
    printf("Slide switch off, ");
  }

  // Print the Pushbuttons status
  if (push1 && push2) {
    // Both pushbuttons are pressed.
    printf("Pushbuttons 1 and 2 activated.");
  } else if (push1) {
    // Only pushbutton 1 pressed.
    printf("Pushbutton 1 activated.");
  } else if (push2) {
    // Only pushbutton 2 pressed.
    printf("Pushbutton 2 activated.");
  } else {
    // No pushbuttons pressed.
    printf("Pushbuttons 1 and 2 not activated.");
  }
  printf("%s", newline());
}

/*
 * Helper function to return a string containing a newline character and return
 * character.
 */
char* newline() {
  char* retval = "\r\n";
  return retval;
}