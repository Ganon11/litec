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
unsigned char random(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

sbit at 0xA0 SS; // Slide switch, associated with Port 2 Pin 0
sbit at 0xB0 PB1 ; // Push button 0, associated with Port 3, Pin 0
sbit at 0xB1 PB2; // Push button 1, associated with Port 3 Pin 1
sbit at 0xB3 BILED0; // BILED0, associated with Port 3 Pin 3
sbit at 0xB4 BILED1; // BILED1, associated with Port 3 Pin 4
sbit at 0xB5 LED1; // LED1, associated with Port 3 Pin 5
sbit at 0xB6 LED0; // LED0, associated with Port 3 Pin 6
sbit at 0xB7 BUZZER; // Buzzer, associated with Port 3 Pin 7

unsigned int Counts = 0;


//***************
void main(void) {
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
    BILED1 = 0;  // Turn OFF the BILED
    BILED2 = 0;

    while( SS ); // while SS0 is ON (high)
    TR0 = 1;     // Timer 0 enabled
    while (PB1); // wait until PB1 is pressed
    Counts = 0;

    while (!PB1);// wait until PB1 is released

    printf("\rNumber of Overflows = %d\n", Counts);
    BILED1 = 1;  // Turn ON the BILED
    BILED2 = 0;

    TR0 = 0;    // Timer 0 disabled
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
  CKCON |= 0x08; // 0000 1000 
  TMOD &= 0xF0; // clear the 4 least significant bits (1111 0000)
  TMOD |= 0x01; // 0000 0001Timer0 in mode 1
	TR0 = 0; // Stop Timer0
	TL0 = 0; // Clear low byte of register T0
	TH0 = 0; // Clear high byte of register T0
}

//***************
void Timer0_ISR(void) interrupt 1
{
// add interrupt code here, in this lab, the code will increment the 
// global variable 'counts'
}

/******************************************************************************/
/*
 * This function demonstrates how to obtain a random integer between 0 and 1 in
 * C. You may modify and use this code to get a random integer between 0 and N.
 */

/*
 * return a random integer number between 0 and 1
 */
unsigned char random(void) {
    return (rand()%2);  // rand returns a random number between 0 and 32767.
                        // the mod operation (%) returns the remainder of 
                        // dividing this value by 2 and returns the result,
                        // a value of either 0 or 1.
}
