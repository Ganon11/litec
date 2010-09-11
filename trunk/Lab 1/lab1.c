/*
 * Names: Michael Stark + David Melecio-Vázquez 
 * Section: 04
 * Date: 6 September 2010
 * File name: lab1.c
 * Program description:
 *
 * This program is incomplete. Part of the code is provided as an example. You
 * need to modify the code, adding code to satisfy the stated requirements. Blank
 * lines have also been provided at some locations, indicating an incomplete line.
 */
 
#include <c8051_SDCC.h> // include files. This file is available online
#include <stdio.h>

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------
void PortInit(void); // Initialize ports for input and output
int CheckPushButton1(void); // function which checks CheckPushButton1
int CheckPushButton2(void); // function which checks CheckPushButton1
int CheckSlideSwitch(void); // function that checks the Slide switch
void SetOutputs(void); // function to set output bits
void PrintInputStatus(int slide, int push1, int push2); // Helper function to print the status of the inputs.
char* newline(); // Helper function that returns "\r\n"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
sbit at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
sbit at 0xB0 PB1 ; // Push button 0, associated with Port 3, Pin 0
sbit at 0xB1 PB2; // Push button 1, associated with Port 3 Pin 1
sbit at 0xB3 BILED0; // BILED0, associated with Port 3 Pin 3
sbit at 0xB4 BILED1; // BILED1, associated with Port 3 Pin 4
sbit at 0xB6 LED0; // LED0, associated with Port 3 Pin 6
sbit at 0xB7 BUZZER; // Buzzer, associated with Port 3 Pin 7

//-----------------------------------------------------------------------------
// Function Definitions
//-----------------------------------------------------------------------------
void main(void) {
  Sys_Init(); // System Initialization
  putchar(' ');
  PortInit(); // Initialize ports 2 and 3
  while (1) { // infinite loop
    // main program manages the function calls
    SetOutputs();
  }
}

/*
 * Initializes Ports 2 and 3 in the desired modes for input and output
 */
void PortInit(void) {
  // Port 2 Constant Masks
  unsigned char P2MDOUT_HI = 0x00; // 0000 0000 - unused in lab 1-1
  unsigned char P2MDOUT_LO = 0xFE; // 1111 1110
  unsigned char P2_HI = 0x01; // 0000 0001
  
  // Port 3 Constant Masks
  unsigned char P3MDOUT_HI = 0xD8; // 1101 1000
  unsigned char P3MDOUT_LO = 0xFC; // 1111 1100
  unsigned char P3_HI = 0x03; // 0000 0011

  // Set Port 2 MDOUT high bits
  P2MDOUT |= P2MDOUT_HI; // in lab 1-1, does nothing
  // Set Port 2 MDOUT low bits
  P2MDOUT &= P2MDOUT_LO;
  // Set Port 2 impedence (high) bits
  P2 |= P2_HI;

  // Set Port 3 MDOUT high bits
  P3MDOUT |= P3MDOUT_HI;
  // Set Port 3 MDOUT low bits
  P3MDOUT &= P3MDOUT_LO;
  // Set Port 3 impedence (high) bits
  P3 |= P3_HI;
}

/*
 * Set outputs.
 *
 * The logic is as follows:
 *   1) If the Slide Switch is off, LED0 turns on, and all other outputs are
 *     turned off.
 *   2) If the Slide Switch is on and both PushButtons are pushed, the Buzzer
 *     is activated.
 *   3) If the Slide Switch is on and PushButton 1 only is pressed, the BiLED
 *     turns green.
 *   4) If the Slide Switch is on and PushButton 2 only is pressed, the BiLED
 *     turns red.
 */
void SetOutputs(void) {
  // Check inputs
  int slide = CheckSlideSwitch();
  int push1 = CheckPushButton1();
  int push2 = CheckPushButton2();
  PrintInputStatus(slide, push1, push2);
  if (!slide) { // if Slide switch is off
    BILED0 = 1; // Turn off Green LED
    BILED1 = 1; // Turn off Red LED
    LED0 = 0; // Turn ON LED
    BUZZER = 1; // Turn off Buzzer
  } else { // if Slide Switch is on
    LED0 = 1; // turn off LED

    if (push1 && push2) { // if both pushbuttons are pressed
      // Turn on Buzzer
      BUZZER = 0;
      // Turn other outputs off
      BILED0 = 1; // Turn off Green LED
      BILED1 = 1; // Turn off Red LED      
    } else if (push1) { // if pushbutton 1 is pressed
      // Set BiLED to GREEN
      BILED0 = 0;
      // Turn other outputs off
      BILED1 = 1;
      BUZZER = 1;
    } else if (push2) { // if pushbutton 2 is pressed
      // Set BiLED to RED
      BILED1 = 0;
      // Turn other outputs off
      BILED0 = 1;
      BUZZER = 1;
    }
  }
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
    printf("Pushbuttons 1 and 2 activated.%s", newline());
  } else if (push1) {
    // Only pushbutton 1 pressed.
    printf("Pushbutton 1 activated.%s", newline());
  } else if (push2) {
    // Only pushbutton 2 pressed.
    printf("Pushbutton 2 activated.%s", newline());
  } else {
    // No pushbuttons pressed.
    printf("Pushbuttons 1 and 2 not activated.%s", newline());
  }
}

/*
 * Helper function to return a string containing a newline character and return
 * character.
 */
char* newline() {
  char* retval = "\r\n";
  return retval;
}