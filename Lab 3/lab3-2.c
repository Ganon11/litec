/*
 * Names: Michael Stark and David Melecio-Vazquez
 * Section: 4 (Side A)
 * Date: 26 October 2010
 * File name: lab3-2.c
 * Program description: FUN TIMES WITH ULTRASONIC RANGEER!
 */

#include <stdio.h>
#include <c8051_SDCC.h>
#include <i2c.h>

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void SMB_Init(void);
void Interrupt_Init(void);
unsigned int Read_Ranger(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

unsigned int Counts = 0;  // number of PCA overflows
unsigned char new_range = 0; // flag to start new range reading
unsigned int PCA_COUNTS = 36864; // number of counts in 20ms

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  unsigned char info[1] = {'\0'}; // Data to write to the ranger
  unsigned char addr = 0xE0; // Address of the ranger
  unsigned int range; // Result of the read operation
  // initialize board
  Sys_Init(); // Initialize the system
  putchar(' ');
  Port_Init();
  Interrupt_Init();
  XBR0_Init();
  SMB_Init();
  PCA_Init();

  printf("\r\nEmbedded Control Ultrasonic Ranger Test\r\n");
  
  info[0] = 0x51; // Signal to start a ping and record result in cm
  
  while(1) {
    if(new_range) {
	    range = Read_Ranger();
    	printf("Range = %d \r\n", range);
      i2c_write_data(addr, 0, info, 1); // Write 1 byte (the ping signal) to register 0
      new_range = 0;
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
	if(Counts >= 4) {
	  new_range = 1; // signal start of read operation
	  Counts = 0;  //Reset Counts
	}
    CF = 0;
  } else {
    PCA0CN &= 0xC0; // all other type 9 interrupts
  }
}

//-----------------------------------------------------------------------------
// Read_Ranger
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
unsigned int Read_Ranger(void) {
  unsigned char info[2] = {'\0'}; // Space for us to read information from ranger
  unsigned int range = 0; // Inititalize the range value to 0.
  unsigned char addr = 0xE0; // Address of the ranger

  i2c_read_data(addr, 2, info, 2); // Read 2 bytes (size of an unsigned int) starting at register 2
//  printf("Got data; first byte %d ('%c'), second byte %d ('%c')\r\n", info[0], info[0], info[1], info[1]);
  range = (((unsigned int)info[0] << 8) | info[1]); // Convert the two bytes of data to one short int
  return range;
}