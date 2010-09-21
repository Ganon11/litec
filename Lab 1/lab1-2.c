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
void Interrupt_Init(void); //Initialize interrupts
void Timer0_ISR(void) interrupt 1;
unsigned char random(unsigned char N);
int CheckPushButton1(void); // function which checks CheckPushButton1
int CheckPushButton2(void); // function which checks CheckPushButton1
int CheckSlideSwitch(void); // function that checks the Slide switch
void PrintInputStatus(int slide, int push1, int push2); // Helper function to print the status of the inputs.
char* newline();

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
  int PB1_Status, PB2_Status, SS_Status;
  Sys_Init();      // System Initialization
  Port_Init();     // Initialize ports 2 and 3 
  Interrupt_Init();
  Timer_Init();    // Initialize Timer 0 
  putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
  
  printf("Start\r\n");
  // the following loop prints the number of overflows that occur
  // while the pushbutton is pressed, the BILED is lit while the
  // button is pressed
  while (1) {
    SS_Status = CheckSlideSwitch();
    PB1_Status = CheckPushButton1();
    PB2_Status = CheckPushButton2();
    PrintInputStatus(SS_Status, PB1_Status, PB2_Status);
    if (!SS_Status) {
      // Light BiLED Red
      BILED0 = 0;

      // Turn Green BiLED off
      BILED1 = 1;
    } else {
      // Light BiLED Green
      BILED1 = 0;

      // Turn Red BiLED off
      BILED0 = 1;
    }

    if (PB1_Status) {
      LED0 = 0;
    } else {
      LED0 = 1;
    }

    if (PB2_Status) {
      LED1 = 0;
    } else {
      LED1 = 1;
    }
  }
}

//***************
void Port_Init(void) {
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

//***************
void Timer0_ISR(void) interrupt 1 {
  TF0 = 0; // clear interrupt request
  Counts++;
}

/******************************************************************************/

/*
 * Returns a random integer between 0 and N-1 ( a range of N numbers).
 */
unsigned char random(unsigned char N) {
  return (rand() % N);
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