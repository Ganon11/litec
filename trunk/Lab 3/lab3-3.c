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

#define PW_MIN 2027
#define PW_NEUT 2764
#define PW_MAX 3502

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void);
void Interrupt_Init(void);
void XBR0_Init(void);
void SMB_Init(void);
void PCA_Init (void);

//-----------------------------------------------------------------------------
// Our functions
//-----------------------------------------------------------------------------
int Read_Ranger(void);
void Drive_Motor(int range);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int MOTOR_PW = 0;
unsigned int Counts = 0;  // Number of overflows, used for setting new_range
unsigned int Overflows = 0; // Number of overflows
unsigned char new_range = 0; // flag to start new range reading
unsigned int PCA_COUNTS = 36864; // number of counts in 20ms

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  unsigned char i;
  unsigned char info[1] = {'\0'}; // Data to write to the ranger
  unsigned char addr = 0xE0; // Address of the ranger
  int range; // Result of the read operation
  Sys_Init();
  putchar(' ');
  Port_Init();
  Interrupt_Init();
  XBR0_Init();
  SMB_Init();
  PCA_Init();

  // print beginning message
  printf("Embedded Control Motor Controlled by Ultrasonic Ranger\r\n");
  
  info[0] = 0x51; // Signal to start a ping and record result in cm

  // set initial value
  MOTOR_PW = PW_NEUT;
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;

  // add code to set the servo motor in neutral for one second
  Overflows = 0; // Overflows is incremented once every 20 ms.  1 s = 1000 ms.  1000 / 20 = 50.  Wait 50 counts
  printf("Waiting...\r\n");
  while (Overflows < 50);

  printf("Done waiting 1 second.\r\n");
  
  // Some trickery to only print the range every 20 times we read it (a few times per second)
  i = 0;

  while (1) {
    if (new_range) {
      i++;
      range = Read_Ranger(); // Read the ultrasonic ranger
      if (i > 5) {
        printf("range read: %d cm\r\n", range);
        i = 0;
      }
      Drive_Motor(range); // Change the speed based on the range read.
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
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
 int Read_Ranger(void) {
  unsigned char info[2] = {'\0'}; // Space for us to read information from ranger
  int range = 0; // Inititalize the range value to 0.
  unsigned char addr = 0xE0; // Address of the ranger

  i2c_read_data(addr, 2, info, 2); // Read 2 bytes (size of an unsigned int) starting at register 2
//  printf("Got data; first byte %d ('%c'), second byte %d ('%c')\r\n", info[0], info[0], info[1], info[1]);
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
  // Change MOTOR_PW based on the range variable
  if (range >= 40 && range <= 50) { // Range is between 40 and 50 cm
    MOTOR_PW = PW_NEUT;
  } else if (range < 40) { // Set forward speed
    if (range < 10) { // Don't allow range to be less than 10 cm
      range = 10;
    }
    MOTOR_PW = (((40 - range) * 246) / 10) + PW_NEUT;
  } else { // Set reverse speed
    if (range > 90) { // Don't allow range to be greater than 90 cm
      range = 90;
    }

    MOTOR_PW = (((50 - range) * 184) / 10) + PW_NEUT;
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
    // Increment Counts variable (used for waiting for some amount of time)
    Counts++;
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