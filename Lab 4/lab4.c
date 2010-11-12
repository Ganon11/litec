/*
 * Names: Michael Stark, David Melecio-Vazquez, Mike Wilkins, and Alan Schimmel
 * Section: 4 (Side A)
 * Date: 2 November 2010
 * File name: lab4.c
 * Program description: Combining the drive control with the steering control,
 *     adding the LCD display, and reading the battery voltage.
 */
 
#include <stdio.h>
#include <c8051_SDCC.h>
#include <i2c.h>

#define DRIVE_PW_MIN 2027 // 1.1 ms pulsewidth
#define DRIVE_PW_NEUT 2764 // 1.5 ms pulsewidth
#define DRIVE_PW_MAX 3502 // 1.9 ms pulsewidth

#define STEER_PW_MIN 0xF985
#define STEER_PW_NEUT 0xF550
#define STEER_PW_MAX 0xF031

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void); // Initialize input/output ports.
void Interrupt_Init(void); // Set up PCA0 interrupts.
void XBR0_Init(void); // Initialize crossbar.
void SMB_Init(void); // Initialize system bus.
void PCA_Init (void); // Initialize the PCA counter.
void ADC_Init(void); // Initialize A/D Conversion

//-----------------------------------------------------------------------------
// Motor functions
//-----------------------------------------------------------------------------
int Read_Ranger(void); // Read the ultrasonic ranger.
void Drive_Motor(int range); // Vary the motor PW based on the range in cm.

//-----------------------------------------------------------------------------
// Steering functions
//-----------------------------------------------------------------------------
int ReadCompass(void); // Read the electronic compass
void Steer(int current_heading, unsigned int k); // Vary the steering PW based on the heading in degrees and the gain constant.

//-----------------------------------------------------------------------------
// Other functions
//-----------------------------------------------------------------------------
unsigned char Read_Port_1(void); // Performs A/D Conversion
float ConvertToVoltage(unsigned char battery);
void LCD_Display(unsigned char battery, unsigned int current_heading, int range);
unsigned int GetGain(void); // Retrieve the user's input for the steering gain
unsigned int GetDesiredHeading(void); // Retrieve the user's input for the desired heading

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int MOTOR_PW = 0; // Pulsewidth to use for the drive motor.
signed int STEER_PW = 0; // Pulsewidth to use for the steering motor
unsigned int D_Counts = 0; // Number of overflows, used for setting new_range
unsigned int S_Counts = 0; // Number of overflows, used for setting new_heading
unsigned int Overflows = 0; // Number of overflows, used for waiting 1 second
unsigned char new_range = 0; // flag to start new range reading
unsigned char new_heading = 0; // flag to start new direction reading
unsigned int PCA_COUNTS = 36864; // number of counts in 20 ms.  Constant.
int desired_heading = 2700; // Desired direction
sbit at 0xB6 DSS; // Slide switch controlling the Drive_Motor function
sbit at 0xB7 SSS; // Slide switch controlling the Steer function.

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  unsigned char i = 0, j = 0; // Index variables for printing every 400 ms or so.
  unsigned char info[1] = {'\0'}; // Data to write to the ranger
  unsigned char addr = 0xE0; // Address of the ranger
  int range = 0; // Result of the read operation
  int current_heading = 0; // Heading read by the electronic compass
  unsigned int steer_gain = 2; // k value for steering control
  unsigned char battery; // Reading for battery voltage

  // System initialization
  Sys_Init();
  putchar(' ');
  Port_Init();
  Interrupt_Init();
  XBR0_Init();
  SMB_Init();
  PCA_Init();
  ADC_Init();

  // print beginning message
  printf("\rEmbedded Control Motor Controlled by Ultrasonic Ranger\r\n");
  
  // Signal to start a ping and record result in cm
  info[0] = 0x51;

  // set initial value
  MOTOR_PW = DRIVE_PW_NEUT;
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;

  Overflows = 0; // Overflows is incremented once every 20 ms.  1 s = 1000 ms.  1000 / 20 = 50.  Wait 50 counts
  printf("Waiting 1 second...\r\n");
  while (Overflows < 50);
  printf("Done waiting 1 second.\r\n");

  steer_gain = GetGain();
  desired_heading = GetDesiredHeading();
  lcd_clear();

  printf("Gain %d, desired heading %d\r\n", steer_gain, desired_heading);

  battery = Read_Port_1();
  printf_fast_f("Battery voltage: %2.1f V\r\n", ConvertToVoltage(battery));

  Overflows = 0;
  while (1) {
    if (Overflows > 20) {
      battery = Read_Port_1();
      printf_fast_f("Battery: %2.1f V\r\n", ConvertToVoltage(battery));
      LCD_Display(battery, current_heading, range);
      Overflows = 0;
    }

    if (new_heading) {
      j++; // Used for printing the heading every 10 times we read, or about every 480 ms.
      current_heading = ReadCompass(); // Read the electronic compass
      
      if (j > 11) {
        printf_fast_f("heading read: %4.1f degrees\r\n", current_heading / 10.0);
        j = 0;
      }
      
      if (!SSS) {
        Steer(current_heading, steer_gain); // Change steering based on the current heading.
      } else {
        // Make the wheels straight
        STEER_PW = STEER_PW_NEUT;
        PCA0CPL0 = STEER_PW;
        PCA0CPH0 = STEER_PW >> 8;
      }
      
      new_heading = 0;
    }
    
    if (new_range) {
      i++; // This is used for printing the range every 6 times we read a new range, or about every 480 ms
      range = Read_Ranger(); // Read the ultrasonic ranger
      
      if (i > 5) {
        printf("range read: %d cm\r\n", range);
        i = 0;
      }
      
      if (!DSS) {
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
  P1MDIN &= 0x7F; // set P1.7 as an anlog input
  P1MDOUT |= 0x05; // set output pin for CEX2 and CEX0 in push-pull mode
  P1MDOUT &= 0x7F; // set input pin P1.7 in open-drain mode.
  P3MDOUT &= 0x3F; // set input pins P3.6 and P3.7 (Slide switches) in open-drain mode
  P1 |= 0x80; // Set input pin P1.7 (A/D) to high impedance.
  P3 |= 0xC0; // Set input pins P3.6 and P3.7 (Slide switches) to high impedance
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
  PCA0CPM0 = 0xC2;
  PCA0CPM2 = 0xC2;
  PCA0MD = 0x81;
}

//-----------------------------------------------------------------------------
// ADC_Init
//-----------------------------------------------------------------------------
//
// Initialize the analog to digital conversion.
//
void ADC_Init(void) {
  REF0CN &= 0xF7; // 1111 0111 Configure ADC1 to use VREF
  REF0CN |= 0x03; // 0000 0011
  ADC1CF = 0x01; // 0000 0001 Set a gain of 1
  ADC1CN |= 0x80; // 1000 0000 Enable ADC1
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
    MOTOR_PW = DRIVE_PW_NEUT;
  } else if (range < 40) { // Set forward speed
    if (range < 10) { // Don't allow range to be less than 10 cm
      range = 10;
    }
    MOTOR_PW = (((40 - range) * 246) / 10) + DRIVE_PW_NEUT; // Varies linearly based on range between DRIVE_PW_MAX and DRIVE_PW_NEUT
  } else { // Set reverse speed
    if (range > 90) { // Don't allow range to be greater than 90 cm
      range = 90;
    }

    MOTOR_PW = (((50 - range) * 184) / 10) + DRIVE_PW_NEUT; // Varies linearly based on range between DRIVE_PW_MIN and DRIVE_PW_NEUT
  }
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;
}

//-----------------------------------------------------------------------------
// ReadCompass
//-----------------------------------------------------------------------------
//
// Fuction to read the electronic compass.
//
signed int ReadCompass() {
	unsigned char addr = 0xC0; // address of the sensor
	unsigned char Data[2]; // array with length of 2
	signed int heading; // the heading returned in degrees between 0 and 3599
	i2c_read_data(addr, 2, Data, 2); // reads 2 bytes into Data[]
	heading = (((signed int)Data[0] << 8) | Data[1]); //combines the two numbers into degrees accurate to 1/10 of a degree
	return heading; //return heading (in degrees)
}

//-----------------------------------------------------------------------------
// Steer
//-----------------------------------------------------------------------------
//
// Fuction to turn the wheels towards desired heading.
//
void Steer(int current_heading, unsigned int k) {
	signed int error = (signed int)((signed int)desired_heading - (signed int)current_heading);

	if (error < -1800) { // If error is too low (car spun around past 1 cycle), add 360 degrees
		error += 3600;
	}	else if (error > 1800) { // If error is too high, subtract 360 degrees
		error -= 3600;
	}

  STEER_PW = STEER_PW_NEUT - (((int)k * error) / 3);

	PCA0CPL0 = STEER_PW;
	PCA0CPH0 = STEER_PW >> 8;
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
    // Increment D_Counts variable (used for waiting 80 ms)
    D_Counts++;
    // Increment S_Counts variable (used for waiting 40 ms)
    S_Counts++;
    // Increment Overflows variable (used for waiting 1 s)
    Overflows++;
    if (D_Counts >= 4) {
      new_range = 1; // signal start of read operation
      D_Counts = 0;  //Reset Counts
  	}
    if (S_Counts >= 2) {
      new_heading = 1;
      S_Counts = 0;
    }
    CF = 0;
  } else {
    PCA0CN &= 0xC0; // all other type 9 interrupts
  }
}

//-----------------------------------------------------------------------------
// Read_Port_1
//-----------------------------------------------------------------------------
//
// Reads the value on Port 1 Pin 7, and performs A/D conversion to return a
// value between 0 and 255.
//
unsigned char Read_Port_1(void) {
  AMX1SL = 7; // Set the ADC conversion to read P1.7
  ADC1CN &= 0xDF; // 1101 1111 Clear the flag from the previous ADC1 conversion
  ADC1CN |= 0x10; // 0001 0000 Start A/D Conversion
  while ((ADC1CN & 0x20) == 0x00); // Wait for conversion to be complete
 	return ADC1; // Return the A/D conversion result
}

//-----------------------------------------------------------------------------
// ConvertToVoltage
//-----------------------------------------------------------------------------
//
// Converts the battery value fron digital units to voltage (float).
//
float ConvertToVoltage(unsigned char battery) {
  return ((12.0 * battery) / 255.0);
}

//-----------------------------------------------------------------------------
// LCD_Display
//-----------------------------------------------------------------------------
//
// Displays statistics about the car (battery voltage, current direction, range read)
//
void LCD_Display(unsigned char battery, unsigned int current_heading, int range) {
  lcd_clear();
  lcd_print("Battery: %d V\nHeading: %d\nRange: %d cm", (battery * 12) / 255, current_heading, range);
}

//-----------------------------------------------------------------------------
// GetGain
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the steering gain constant.
//
unsigned int GetGain(void) {
  char keypad, temp;
//  printf("GetGain()\r\n");

  lcd_clear();
  lcd_print("Gain? ");

  do {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);
//    printf ("  read %c\r\n", keypad);

    Overflows = 0;
    while (Overflows < 1);

    // Wait until user releases the keypad
    do {
      temp = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (temp != -1);
  } while (keypad < '1' || keypad > '9'); // If the user hit * or #, we don't want it.

//  printf("GetGain() returning %d\r\n", keypad - '0');
  return (keypad - '0'); // Subtract the value of '0' to get the numeric value between 0 and 9.
}

//-----------------------------------------------------------------------------
// GetDesiredHeading
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the desired heading constant.
//
unsigned int GetDesiredHeading(void) {
  char keypad, temp;
//  printf("GetDesiredHeading()\r\n");

  lcd_clear();
  lcd_print("Desired heading?\n1) 0 deg 2) 90 deg\n3) 180 deg\n4) 270 deg");

  do {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);
//    printf ("  read %c\r\n", keypad);

    Overflows = 0;
    while (Overflows < 1);

    // Wait until user releases the keypad
    do {
      temp = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (temp != -1);
  } while (keypad < '1' || keypad > '4'); // Restrict input to 1, 2, 3, or 4

//  printf("GetDesiredHeading(): user hit %c\r\n", keypad);

  switch (keypad) {
    case '1':
      return 0;
    case '2':
      return 900;
    case '3':
      return 1800;
    case '4':
      return 2700;
    default: // This should never happen
      return 0;
  }
}