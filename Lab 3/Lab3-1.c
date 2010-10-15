/*
 * Names: Michael Stark and David Melecio-Vazquez
 * Section: 4 (Side A)
 * Date: 8 October 2010
 * File name: lab3-1.c
 * Program description: Sample code for speed control using PWM.
 */

#include <stdio.h>
#include <c8051_SDCC.h>

#define PCA_COUNTS 36864
#define PW_MIN 2027
#define PW_NEUT 2764
#define PW_MAX 3502

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void Interrupt_Init(void);
void Drive_Motor(void);
unsigned int MOTOR_PW = 0;
unsigned int Counts = 0;

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  // initialize board
  Sys_Init();
  putchar(' '); // the quotes in this line may not format correctly
  Port_Init();
  XBR0_Init();
  PCA_Init();

  // print beginning message
  printf("Embedded Control Drive Motor Control\r\n");

  // set initial value
  MOTOR_PW = PW_NEUT;
  printf("MOTOR_PW = %d\r\n", MOTOR_PW);

  // add code to set the servo motor in neutral for one second
  Counts = 0; // Counts is incremented once every 20 ms.  1 s = 1000 ms.  1000 / 20 = 50.  Wait 50 counts
  while (Counts < 50) {
    printf("Waiting...Counts = %d\r\n", Counts);
  }

  printf("Done waiting 1 second.\r\n");

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
  // wait for a key to be pressed
  input = getchar();
  if (input == 'f') { // if 'f' is pressed by the user
    printf("You pressed 'f'.  Going faster.\r\n");
    if (MOTOR_PW < PW_MAX) {
      MOTOR_PW = MOTOR_PW + 10; // increase the steering pulsewidth by 10
    }
  } else if (input == 's') { // if 's' is pressed by the user
    printf("You pressed 's'.  Going slower.\r\n");
    if (MOTOR_PW > PW_MIN) {
      MOTOR_PW = MOTOR_PW - 10; // decrease the steering pulsewidth by 10
    }
  } else {
    printf("You pressed '%c'.  Huh?\r\n", input);
  }
  printf("MOTOR_PW = %d\r\n", MOTOR_PW);
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
  P0MDOUT |= 0x40; // set output pin for CEX2 in push-pull mode
}

//-----------------------------------------------------------------------------
// Interrupt_Init
//-----------------------------------------------------------------------------
//
// Enable proper interrupts.
//
void Interrupt_Init() {
  // IE and EIE1
  EA = 1; // Enable interrupts globally
  EIE1 |= 0x08; // Enable PCA0 interrupts
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init() {
  XBR0 = 0x27; // configure crossbar with UART0, SPI, SMBus, and CEX channels as in worksheet
}

//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void) {
  // reference to the sample code in Example 4.5 - Pulse Width Modulation implemented using the PCA (Programmable Counter Array, p. 50 in Lab Manual.
  // Use a 16 bit counter with SYSCLK/12.
  // WE WILL USE CCM 2!  Felix and Alan will use CCM 0!
  PCA0MD = 0x81;
  PCA0CPM2 = 0xC2;
  PCA0CN |= 0x40;
}

//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR ( void ) interrupt 9 {
  if (CF) {
    printf("INTERRUPT LOLOLOL\r\n");
    // Reset PCA to the correct start value (65,535 - PCA_COUNTS)
    PCA0L = 0xFFFF - PCA_COUNTS;
    PCA0H = (0xFFFF - PCA_COUNTS) << 8;
    // Increment Counts variable (used for waiting for some amount of time)
    Counts++;
  } else {
    PCA0CN &= 0xC0; // all other type 9 interrupts
  }
}