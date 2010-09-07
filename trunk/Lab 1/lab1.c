/* Names: Michael Stark
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
sbit at 0xB6 LED0; // LED0, associated with Port 3 Pin 6
//sbit at 0x__ BILED0; // BILED0, associated with ?????
//sbit at 0x__ BILED1; // BILED1, associated with ?????
//sbit at 0x__ BUZZER; // Buzzer, associated with ?????
sbit at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
sbit at 0xB0 PB1 ; // Push button 0, associated with Port 3, Pin 0
//sbit at 0x__ PB2; // Push button 1, associated with ?????

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
  // Port 3
  //P3MDOUT ________; // set Port 3 output pins to push-pull mode (fill in the blank)
  //P3MDOUT ________; // set Port 3 input pins to open drain mode (fill in the blank)
  //P3 ________; // set Port 3 input pins to high impedance state (fill in the blank)
  // Port 2
  // configure Port 2 as needed
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
    LED0 = 0; // Light LED
  } else { // if Slide Switch is on
    LED0 = 1; // turn off LED

    if (push1 && push2) { // if both pushbuttons are pressed
      // Turn on Buzzer
    } else if (push1) { // if pushbutton 1 is pressed
      // Set BiLED to GREEN
    } else if (push2) { // if pushbutton 2 is pressed
      // Set BiLED to RED
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
//  return !PB2;
  return 0;
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