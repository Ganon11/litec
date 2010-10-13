/*
 * Names: Michael Stark and David Melecio-Vazquez
 * Section: 4 (Side A)
 * Date: 8 October 2010
 * File name: lab3-1.c
 * Program description: Sample code for speed control using PWM.
 */

#include <stdio.h>
#include <c8051_SDCC.h>

#define PW_MIN _____
#define PW_MAX _____
#define PW_NEUT _____

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void Interrupt_Init(void);
void Drive_Motor(void);
unsigned int MOTOR_PW = 0;

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  // initialize board
  Sys_Init();
  putchar(' '); //the quotes in this line may not format correctly
  Port_Init();
  XBR0_Init();
  PCA_Init();

  //print beginning message
  printf("Embedded Control Drive Motor Control\r\n");

  //set initial value
  MOTOR_PW = PW_NEUT;

  //add code to set the servo motor in neutral for one second

  while(1) {
    Drive_Motor();
  }
}

//-----------------------------------------------------------------------------
// Drive_Motor
//-----------------------------------------------------------------------------
//
// Vary the pulsewidth based on the user input to change the speed
// of the drive motor.
//
void Drive_Motor() {
  char input;
  //wait for a key to be pressed
  input = getchar();
  if (input == 'f') { //if 'f' is pressed by the user
    if (MOTOR_PW < PW_MAX)
      MOTOR_PW = MOTOR_PW + 10; //increase the steering pulsewidth by 10
  } else if (input == 's') { //if 's' is pressed by the user
    if (MOTOR_PW > PW_MIN)
      MOTOR_PW = MOTOR_PW - 10; //decrease the steering pulsewidth by 10
  }
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Port_Init() {
  P1MDOUT = ________ ;//set output pin for CEX2 in push-pull mode
}

//-----------------------------------------------------------------------------
// Interrupt_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Interrupt_Init() {
// IE and EIE1
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init() {
  XBR0 = __________ ; //configure crossbar with UART, SPI, SMBus, and CEX channels as in worksheet
}

//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void) {
  // reference to the sample code in Example 4.5 - Pulse Width Modulation implemented us-ing the PCA (Programmable Counter Array, p. 50 in Lab Manual.
  // Use a 16 bit counter with SYSCLK/12.
}

//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR ( void ) interrupt 9 {
}