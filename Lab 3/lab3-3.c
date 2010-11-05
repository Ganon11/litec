/*
 * Names: Michael Stark and David Melecio-Vazquez
 * Section: 4 (Side A)
 * Date: 2 November 2010
 * File name: lab3-3.c
 * Program description: The readings from the ultrasonic ranger will not control the speed of the car
 */
 
#include <stdio.h>
#include <c8051_SDCC.h>
#include <i2c.h>

#define PW_MIN 2027 // 1.1 ms pulsewidth
#define PW_NEUT 2764 // 1.5 ms pulsewidth
#define PW_MAX 3502 // 1.9 ms pulsewidth

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void); // Initialize input/output ports.
void Interrupt_Init(void); // Set up PCA0 interrupts.
void XBR0_Init(void); // Initialize crossbar.
void SMB_Init(void); // Initialize system bus.
void PCA_Init (void); // Initialize the PCA counter.

//-----------------------------------------------------------------------------
// Our functions
//-----------------------------------------------------------------------------
int Read_Ranger(void); // Read the ultrasonic ranger.
void Drive_Motor(int range); // Vary the motor PW based on the range in cm.

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int MOTOR_PW = 0; // Pulsewidth to use.
unsigned int Counts = 0;  // Number of overflows, used for setting new_range
unsigned int Overflows = 0; // Number of overflows, used for waiting 1 second
unsigned char new_range = 0; // flag to start new range reading
unsigned int PCA_COUNTS = 36864; // number of counts in 20 ms.  Constant.
sbit at 0xB6 SS; // Slide switch controlling the Drive_Motor function

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  unsigned char i = 0; // Index variable for printing range every 400 ms or so.
  unsigned char info[1] = {'\0'}; // Data to write to the ranger
  unsigned char addr = 0xE0; // Address of the ranger
  int range; // Result of the read operation

  // System initialization
  Sys_Init();
  putchar(' ');
  Port_Init();
  Interrupt_Init();
  XBR0_Init();
  SMB_Init();
  PCA_Init();

  // print beginning message
  printf("\rEmbedded Control Motor Controlled by Ultrasonic Ranger\r\n");
  
  // Signal to start a ping and record result in cm
  info[0] = 0x51;

  // set initial value
  MOTOR_PW = PW_NEUT;
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;

  Overflows = 0; // Overflows is incremented once every 20 ms.  1 s = 1000 ms.  1000 / 20 = 50.  Wait 50 counts
  printf("Waiting 1 second...\r\n");
  while (Overflows < 50);
  printf("Done waiting 1 second.\r\n");

  while (1) {
    if (new_range) {
      i++; // This is used for printing the range every 6 times we read a new range, or about every 480 ms
      range = Read_Ranger(); // Read the ultrasonic ranger
      if (i > 5) {
        printf("range read: %d cm\r\n", range);
        printf("SS = %d\r\n", SS);
        i = 0;
      }
      if (!SS) {
        Drive_Motor(range); // Change the speed based on the range read.
      } else {
        Drive_Motor(45); // Set the motor to neutral.
      }
      i2c_write_data(addr, 0, info, 1); // Write the ping signal to register 0 of the ranger
      new_range = 0; // Reset the flag and wait for 80ms
    }
  }
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Port_Init() {
  P1MDOUT |= 0x04; // set output pin for CEX2 in push-pull mode
  P3MDOUT &= 0xBF; // set input pin P3.6 (Slide switch) in open-drain mode
  P3 |= 0x40; // Set input pin P3.6 (Slide switch) to high impedance
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
               // 0001 1111
  XBR0 = 0x27; // configure crossbar with UART0, SPI, SMBus, and CEX channels as in worksheet 7
}

//-----------------------------------------------------------------------------
// SMB_Init
//-----------------------------------------------------------------------------
//
// Set up the system bus
//
void SMB_Init(void) {
  SMB0CR = 0x93; // set SCL to use 100 kHz
  ENSMB = 1; // Bit 6 of SMB0CN.  Enables the system bus.
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
  PCA0CN = 0x40;
  PCA0CPM2 = 0xC2;
  PCA0MD = 0x81;
}

//-----------------------------------------------------------------------------
// Read_Ranger
//-----------------------------------------------------------------------------
//
// Reads the latest ping value from the Ultrasonic Ranger, and returns the result.
//
int Read_Ranger(void) {
  unsigned char info[2] = {'\0'}; // Space for us to read information from ranger
  int range = 0; // Inititalize the range value to 0.
  unsigned char addr = 0xE0; // Address of the ranger

  i2c_read_data(addr, 2, info, 2); // Read 2 bytes (size of an unsigned int) starting at register 2
  range = (((int)info[0] << 8) | info[1]); // Convert the two bytes of data to one short int
  return range;
}

//-----------------------------------------------------------------------------
// Drive_Motor
//-----------------------------------------------------------------------------
//
// Vary the pulsewidth based on the user input to change the speed
// of the drive motor.
//
void Drive_Motor(int range) {
  if (range >= 40 && range <= 50) { // Range is between 40 and 50 cm
    MOTOR_PW = PW_NEUT;
  } else if (range < 40) { // Set forward speed
    if (range < 10) { // Don't allow range to be less than 10 cm
      range = 10;
    }
    MOTOR_PW = (((40 - range) * 246) / 10) + PW_NEUT; // Varies linearly based on range between PW_MAX and PW_NEUT
  } else { // Set reverse speed
    if (range > 90) { // Don't allow range to be greater than 90 cm
      range = 90;
    }

    MOTOR_PW = (((50 - range) * 184) / 10) + PW_NEUT; // Varies linearly based on range between PW_MIN and PW_NEUT
  }
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;
}

//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR ( void ) interrupt 9 {
  if (CF) {
    // Reset PCA to the correct start value (65,535 - PCA_COUNTS)
    PCA0L = 0xFFFF - PCA_COUNTS;
    PCA0H = (0xFFFF - PCA_COUNTS) >> 8;
    // Increment Counts variable (used for waiting 80 ms)
    Counts++;
    // Increment Overflows variable (used for waiting 1 s)
    Overflows++;
    if(Counts >= 4) {
      new_range = 1; // signal start of read operation
      Counts = 0;  //Reset Counts
  	}
    CF = 0;
  } else {
    PCA0CN &= 0xC0; // all other type 9 interrupts
  }
}