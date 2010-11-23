/*
 * Names: Michael Stark, David Melecio-Vazquez, Mike Wilkins, and Alan Schimmel
 * Section: 4 (Side A)
 * Date: 16 November 2010
 * File name: lab5.c
 * Program description:
 */
 
#include <stdio.h>
#include <c8051_SDCC.h>
#include <i2c.h>

#define THRUST_PW_MIN 2027 // 1.1 ms pulsewidth
#define THRUST_PW_NEUT 2764 // 1.5 ms pulsewidth
#define THRUST_PW_MAX 3502 // 1.9 ms pulsewidth

#define STEER_PW_MIN 2000
#define STEER_PW_NEUT 2750
#define STEER_PW_MAX 3500

#define DESIRED_HEIGHT 50 // Desire a height of 50 cm.

#define MAX_LEN 5

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
// Thrust functions
//-----------------------------------------------------------------------------
int Read_Ranger(void); // Read the ultrasonic ranger.
void Thrust_Fans(int range); // Vary the thrust fans PW based on the range in
                             // cm.

//-----------------------------------------------------------------------------
// Steering functions
//-----------------------------------------------------------------------------
int ReadCompass(void); // Read the electronic compass

// Vary the steering PW based on the heading in degrees and the gain constants.
signed int Steer(int current_heading, unsigned int kp, unsigned int kd,
           signed int prev_error);

//-----------------------------------------------------------------------------
// Other functions
//-----------------------------------------------------------------------------
unsigned char Read_Port_1(void); // Performs A/D Conversion
float ConvertToVoltage(unsigned char battery);
void LCD_Display(unsigned int current_heading, int range);
unsigned int GetHeadingPGain(void);	// Retrieve the user's input for the
                                    // proportional steering gain
unsigned int GetHeadingDGain(void); // Retrieve the user's input for the
                                    // derivative steering gain
unsigned int GetPowerPGain(void); // Retrieve the user's input for the
                                    // proportional power gain
unsigned int GetPowerDGain(void); // Retrieve the user's input for the
                                    // derivative power gain
unsigned int GetDesiredHeading(void); // Retrieve the user's input for the
                                      // desired heading

int atoi(char *buf); // Converts a string of characters to the equivalent
                              // integer, or -1 if invalid.

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int MOTOR_PW = 0; // Pulsewidth to use for the drive motor.
signed int STEER_PW = 0; // Pulsewidth to use for the steering motor
unsigned char D_Counts = 0; // Number of overflows, used for setting new_range
unsigned char S_Counts = 0; // Number of overflows, used for setting new_heading
unsigned int Overflows = 0; // Number of overflows, used for waiting 1 second
unsigned char new_range = 0; // flag to start new range reading
unsigned char new_heading = 0; // flag to start new direction reading
unsigned int PCA_COUNTS = 36864; // number of counts in 20 ms.  Constant.
sbit at 0xB6 DSS; // Slide switch controlling the Thrust_Fans function
sbit at 0xB7 SSS; // Slide switch controlling the Steer function.

//-----------------------------------------------------------------------------
// XData Constants
//-----------------------------------------------------------------------------
xdata int desired_heading; // Desired direction
xdata unsigned int heading_p_gain; // Proportional gain constant for steering.
xdata unsigned int heading_d_gain; // Derivative gain constant for steering.
xdata unsigned int thrust_p_gain; // Proportional gain constant for power.
xdata unsigned int thrust_d_gain; // Derivative gain constant for power.

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void) {
  unsigned char info[1] = {'\0'}; // Data to write to the ranger
  unsigned char addr = 0xE0; // Address of the ranger
  int range = 0; // Result of the read operation
  int current_heading = 0; // Heading read by the electronic compass
  signed int prev_error = 0;

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
  MOTOR_PW = THRUST_PW_NEUT;
  PCA0CPL2 = 0xFFFF - MOTOR_PW;
  PCA0CPH2 = (0xFFFF - MOTOR_PW) >> 8;

  Overflows = 0; // Overflows is incremented once every 20 ms.  1 s = 1000 ms.
                 // 1000 / 20 = 50.  Wait 50 counts
  printf("Waiting 1 second...\r\n");
  while (Overflows < 50);
  printf("Done waiting 1 second.\r\n");

  desired_heading = GetDesiredHeading();
  heading_p_gain = GetHeadingPGain();
  heading_d_gain = GetHeadingDGain();
  thrust_p_gain = GetPowerPGain();
  thrust_d_gain = GetPowerDGain();

  lcd_clear();


  Overflows = 0;
  while (1) {
    if (Overflows > 20) {
//      battery = Read_Port_1();
      printf("%u\t%u\t%u\t%u\r\n", desired_heading, current_heading,
             DESIRED_HEIGHT, range);
      LCD_Display(current_heading, range);
      Overflows = 0;
    }

    if (new_heading) {
      current_heading = ReadCompass(); // Read the electronic compass
      
      if (!SSS) {
        prev_error = Steer(current_heading, heading_p_gain, heading_d_gain,
                           prev_error);
      } else {
        // Make the wheels straight
        STEER_PW = STEER_PW_NEUT;
        PCA0CPL0 = STEER_PW;
        PCA0CPH0 = STEER_PW >> 8;
      }
      
      new_heading = 0;
    }
    
    if (new_range) {
      range = Read_Ranger(); // Read the ultrasonic ranger
      
      if (!DSS) {
        Thrust_Fans(range); // Change the speed based on the range read.
      } else {
        Thrust_Fans(DESIRED_HEIGHT); // Set the motor to neutral.
      }
      
      i2c_write_data(addr, 0, info, 1); // Write the ping signal to register 0
                                        // of the ranger
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
  P1MDIN &= 0xDF; // set P1.5 as an anlog input
  P1MDOUT |= 0x0F; // set output pin for CEX0 through 4 in push-pull mode
  P1MDOUT &= 0xDF; // set input pin P1.7 in open-drain mode.
  P3MDOUT &= 0x3F; // set input pins P3.6 and P3.7 (Slide switches) in
                   // open-drain mode
  P1 |= 0x20; // Set input pin P1.7 (A/D) to high impedance.
  P3 |= 0xC0; // Set input pins P3.6 and P3.7 (Slide switches) to high
              // impedance
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
  XBR0 = 0x27; // configure crossbar with UART0, SPI, SMBus, and CEX channels
               // as in worksheet 7
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
// Reads the latest ping value from the Ultrasonic Ranger, and returns the
// result.
//
int Read_Ranger(void) {
  unsigned char info[2] = {'\0'}; // Space for us to read information from
                                  // ranger
  int range = 0; // Inititalize the range value to 0.
  unsigned char addr = 0xE0; // Address of the ranger

  i2c_read_data(addr, 2, info, 2); // Read 2 bytes (size of an unsigned int)
                                   // starting at register 2
  range = (((int)info[0] << 8) | info[1]); // Convert the two bytes of data to
                                           // one short int
  return range;
}

//-----------------------------------------------------------------------------
// Thrust_Fans
//-----------------------------------------------------------------------------
//
// Vary the pulsewidth based on the user input to change the speed
// of the drive motor.
//
void Thrust_Fans(int range) {
  if (range >= 40 && range <= 50) { // Range is between 40 and 50 cm
    MOTOR_PW = THRUST_PW_NEUT;
  } else if (range < 40) { // Set forward speed
    if (range < 10) { // Don't allow range to be less than 10 cm
      range = 10;
    }
    MOTOR_PW = (((40 - range) * 246) / 10) + THRUST_PW_NEUT; 
	MOTOR_PW += (error - prev_error)/D_Counts;
		// Varies linearly
        // based on range between THRUST_PW_MAX and THRUST_PW_NEUT
  } else { // Set reverse speed
    if (range > 90) { // Don't allow range to be greater than 90 cm
      range = 90;
    }

    MOTOR_PW = (((50 - range) * 184) / 10) + THRUST_PW_NEUT;
	MOTOR_PW += (error - prev_error)/D_Counts;
		// Varies linearly
        // based on range between THRUST_PW_MIN and THRUST_PW_NEUT
  }
  
  D_Counts = 0;
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
	heading = (((signed int)Data[0] << 8) | Data[1]); // combines the two numbers
                                                    // into degrees accurate to
                                                    // 1/10 of a degree
	return heading; //return heading (in degrees)
}

//-----------------------------------------------------------------------------
// Steer
//-----------------------------------------------------------------------------
//
// Function to turn the wheels towards desired heading.
//
signed int Steer(int current_heading, unsigned int kp, unsigned int kd,
           signed int prev_error) {
  signed int delta_error;
	signed int error = (signed int)((signed int)desired_heading -
      (signed int)current_heading);

	if (error < -1800) { // If error is too low (car spun around past 1 cycle),
                       // add 360 degrees
		error += 3600;
	}	else if (error > 1800) { // If error is too high, subtract 360 degrees
		error -= 3600;
	}

  delta_error = error - prev_error;

  STEER_PW = STEER_PW_NEUT + (((int)kp * error) / 10) + (((int)kd * (delta_error / 40)) / 10);
//  printf("current heading = %d, kp = %u, kd = %u, prev_error = %d, error = %d, OLD_STEER_PW = %u, ", current_heading, kp, kd, prev_error, error, STEER_PW);

  if (STEER_PW < STEER_PW_MIN) { 
    STEER_PW = STEER_PW_MIN;
  } else if (STEER_PW > STEER_PW_MAX) {
    STEER_PW = STEER_PW_MAX;
  }

//  printf("NEW_STEER_PW = %u\r\n", STEER_PW);
	PCA0CPL0 = 0xFFFF - STEER_PW;
	PCA0CPH0 = (0xFFFF - STEER_PW) >> 8;

  return error;
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
// Displays statistics about the car.
//
void LCD_Display(unsigned int current_heading, int range) {
  lcd_clear();
  lcd_print("Heading: %d\nRange: %d cm\n", current_heading, range);
}

//-----------------------------------------------------------------------------
// GetHeadingPGain
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the
// proportional steering gain constant.
//
unsigned int GetHeadingPGain(void) {
  char keypad, i = 0, buf[MAX_LEN];
  buf[MAX_LEN - 1] = '\0';

  lcd_clear();
  lcd_print("Heading P. Gain? ");
  while (1) {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);

    // Wait until user releases the keypad
    do {
      Overflows = 0;
      while (Overflows < 1);
    } while (read_keypad() != -1);

    printf("keypad == '%c'\r\n", keypad);
    
    // keypad now holds 1 character.
    if (keypad == '*' || keypad == '#') {
      buf[i] = '\0';
      break;
    }
    buf[i] = keypad;
    i++;

    if (i == MAX_LEN - 1) {
      break;
    }
  }

  printf("Got Heading P Gain \"%s\"\r\n", buf);
  return atoi(buf); // Subtract the value of '0' to get the numeric value
                         // between 0 and 9.
}

//-----------------------------------------------------------------------------
// GetHeadingDGain
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the
// derivative steering gain constant.
//
unsigned int GetHeadingDGain(void) {
  char keypad, i = 0, buf[MAX_LEN];
  buf[MAX_LEN - 1] = '\0';

  lcd_clear();
  lcd_print("Heading D. Gain? ");
  while (1) {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);

    // Wait until user releases the keypad
    do {
      Overflows = 0;
      while (Overflows < 1);
    } while (read_keypad() != -1);

    printf("keypad == '%c'\r\n", keypad);
    
    // keypad now holds 1 character.
    if (keypad == '*' || keypad == '#') {
      buf[i] = '\0';
      break;
    }
    buf[i] = keypad;
    i++;

    if (i == MAX_LEN - 1) {
      break;
    }
  }

  printf("Got Heading D Gain \"%s\"\r\n", buf);
  return atoi(buf); // Subtract the value of '0' to get the numeric value
                         // between 0 and 9.
}

//-----------------------------------------------------------------------------
// GetPowerPGain
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the
// proportional power gain constant.
//
unsigned int GetPowerPGain(void) {
  char keypad, i = 0, buf[MAX_LEN];
  buf[MAX_LEN - 1] = '\0';

  lcd_clear();
  lcd_print("Power P. Gain? ");
  while (1) {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);

    // Wait until user releases the keypad
    do {
      Overflows = 0;
      while (Overflows < 1);
    } while (read_keypad() != -1);

    printf("keypad == '%c'\r\n", keypad);
    
    // keypad now holds 1 character.
    if (keypad == '*' || keypad == '#') {
      buf[i] = '\0';
      break;
    }
    buf[i] = keypad;
    i++;

    if (i == MAX_LEN - 1) {
      break;
    }
  }

  printf("Got Power P Gain \"%s\"\r\n", buf);
  return atoi(buf); // Subtract the value of '0' to get the numeric value
                         // between 0 and 9.
}

//-----------------------------------------------------------------------------
// GetPowerDGain
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the 
// derivative power gain constant.
//
unsigned int GetPowerDGain(void) {
  char keypad, i = 0, buf[MAX_LEN];
  buf[MAX_LEN - 1] = '\0';

  lcd_clear();
  lcd_print("Power D. Gain? ");
  while (1) {
    do {
      keypad = read_keypad();
      Overflows = 0;
      while (Overflows < 1);
    } while (keypad == -1);

    // Wait until user releases the keypad
    do {
      Overflows = 0;
      while (Overflows < 1);
    } while (read_keypad() != -1);

    printf("keypad == '%c'\r\n", keypad);
    
    // keypad now holds 1 character.
    if (keypad == '*' || keypad == '#') {
      buf[i] = '\0';
      break;
    }
    buf[i] = keypad;
    i++;

    if (i == MAX_LEN - 1) {
      break;
    }
  }

  printf("Got Power D Gain \"%s\"\r\n", buf);
  return atoi(buf); // Subtract the value of '0' to get the numeric value
                         // between 0 and 9.
}

//-----------------------------------------------------------------------------
// GetDesiredHeading
//-----------------------------------------------------------------------------
//
// Using the LCD display and keypad, queries the user to determine the desired
// heading constant.
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


//-----------------------------------------------------------------------------
// atoi
//-----------------------------------------------------------------------------
//
// Converts an array of number characters to the equivalent number.
//
int atoi(char *buf) {
  int sum = 0;
  char i = 0;
  if (buf == NULL) return 0;
  while (buf[i]) {
    sum *= 10;
    if (buf[i] < '0' || buf[i] > '9') return 0;
    sum += buf[i] - '0';
    i++;
  }
  return sum;
}