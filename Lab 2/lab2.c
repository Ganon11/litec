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

// Initialization Functions
void Port_Init(void);      // Initialize ports for input and output
void Timer_Init(void);     // Initialize Timer 0 
void ADC_Init(void);       // Initialize A/D Conversion
unsigned char Read_Port_1(void);       // Performs A/D Conversion
void Interrupt_Init(void); // Initialize interrupts
void Timer0_ISR(void) interrupt 1; // Called at Timer0 overflow

// Game Functions
void play_game(void); // Plays the LITEC Memory game.
int CalculateMaxCounts(unsigned char x); // Converts the port 1 result into a number of overflows to wait for.
unsigned char light_LED(unsigned char LED_to_light, short on_time, short off_time); // Light the designated LED for on_time, then wait for off_time
unsigned char ReadPushbuttons(); // Wait for a pushbutton event, and return which one was pressed
unsigned char unique_random(unsigned char last_state); // Generates a random number different from the last one
unsigned char random(unsigned char N); // Generates a random number between 0 and N-1
void wait_one_second(void); // Waits 1 second
void light_green(void); // Light BiLED green
void light_red(void); // Light BiLED red
int CheckPushButton1(void); // function which checks push button 1
int CheckPushButton2(void); // function which checks push button 2
int CheckPushButton3(void); // function which checks push button 3
int CheckPushButton4(void); // function which checks push button 4
void PrintInputStatus(int slide, int push1, int push2); // Helper function to print the status of the inputs.
char* newline(); // Helper function, used when printing (returns "\r\n")

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

sbit at 0xA0 PB3; // Push button 3, associated with Port 2, Pin 0
sbit at 0xA1 PB4; // Push button 4, associated with Port 2, Pin 1
sbit at 0xB0 PB1; // Push button 1, associated with Port 3, Pin 0
                  // Black PB is PB1
sbit at 0xB1 PB2; // Push button 2, associated with Port 3 Pin 1
                  // Red PB is PB2
sbit at 0xB2 LED2; // LED2, associated with Port 3, Pin 2
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
  ADC_Init();      // Initialize A/D Conversion
  putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
  putchar('\r');

  // Enable Timer 0
  TR0 = 1;

  while (1) {
    play_game();
  }
}

//***************
void Port_Init(void) {
  // Port 1 Constant Masks
  unsigned char P1MDIN_LO = 0xFD; // 1111 1101, Set P1.1 as an analog input 
  unsigned char P1MDOUT_LO = 0xFD; // 1111 1101, Set P1.1 as a input port bit
  unsigned char P1_HI = 0x02; // 0000 0010 Set P1.1 to a high impedance state

  // Port 2 Constant Masks
  unsigned char P2MDOUT_LO = 0xFC; // 1111 1100
  unsigned char P2_HI = 0x03; // 0000 0011
  
  // Port 3 Constant Masks
  unsigned char P3MDOUT_HI = 0xFC; // 1111 1100
  unsigned char P3MDOUT_LO = 0x03; // 0000 0011
  unsigned char P3_HI = 0xFC; // 1111 1100

  P1MDIN &= P1MDIN_LO;
  P1MDOUT &= P1MDOUT_LO;
  P1 |= P1_HI;

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
  CKCON |= 0x08; // 0000 1000 (use SYSCLK)
  TMOD &= 0xF0; // clear the 4 least significant bits (1111 0000)
  TMOD |= 0x01; // 0000 0001 (Timer0 in mode 1, for 16-bit)
	TR0 = 0; // Stop Timer0
	TL0 = 0; // Clear low byte of register T0
	TH0 = 0; // Clear high byte of register T0
}

void ADC_Init(void) {
  REF0CN &= 0XF7; // 1111 0111 Configure ADC1 to use VREF
  REF0CN |= 0x03; // 0000 0011
  ADC1CF =0x01; // 0000 0001 Set a gain of 1
  ADC1CN |= 0x80; // 1000 0000 Enable ADC1
}

unsigned char Read_Port_1(void) {
  AMX1SL = 0x01; // 0000 0001 Set the Port pin number
  ADC1CN &= 0xDF; // 1101 1111 Clear the flag from the previous ADC1 conversion
  ADC1CN |= 0x10; // 0001 0000 Start A/D Conversion
  while ((ADC1CN & 0x20) == 0x00); // Wait for conversion to be complete
 	return ADC1; //Assign the A/D conversion result
}

//***************
void Timer0_ISR(void) interrupt 1 {
  TF0 = 0; // clear interrupt request
  Counts++;
}

/******************************************************************************/

/*
 * Play the LITEC Memory game.  Returns if PB4 is ever pressed.
 */
void play_game(void) {
  unsigned char turns[5], inputs[5], port1_result, i, correct;
  unsigned short on_max_counts, off_max_counts;

  // Turn off all outputs
  LED0 = 1;
  LED1 = 1;
  LED2 = 1;
  BUZZER = 1;
  BILED0 = 1;
  BILED1 = 1;

  port1_result = Read_Port_1();
  on_max_counts = CalculateMaxCounts(port1_result);
  off_max_counts = (on_max_counts / 2);

  turns[0] = unique_random(3);
  for (i = 1; i < 5; i++) {
    turns[i] = unique_random(turns[i - 1]);
  }

  // Light LED sequence
  for (i = 0; i < 5; i++) {
    // Restart the game if the player presses PushButton4.
    if (light_LED(turns[i], on_max_counts, off_max_counts) == 1) {
      return;
    }
  }

  light_green();

  for (i = 0; i < 5; i++) {
    inputs[i] = ReadPushbuttons();
    if (inputs[i] == 3) { // Pushbutton 4 pressed, restart game
      return;
    }
  }

  correct = 1;

  for (i = 0; i < 5; i++) {
    if (turns[i] == inputs[i]) {
      printf("Input %d was correct!%s", i + 1, newline());
    } else {
      correct = 0;
      printf("Input %d was incorrect!%s", i + 1, newline());
    }
  }

  if (correct) {
    // flash LEDs 3 times
    for (i = 0; i < 3; i++) {
      // Turn LEDs on
      LED0 = 0;
      LED1 = 0;
      LED2 = 0;
      // Wait about a quarter of a second
      Counts = 0;
      while (Counts < 75);
      // Turn LEDs off
      LED0 = 1;
      LED1 = 1;
      LED2 = 1;
      // Wait about a quarter of a second
      Counts = 0;
      while (Counts < 75);
    }
  } else {
    // sounds buzzer for 1.5 seconds
    BUZZER = 0;
    Counts = 0;
    while (Counts < 506);
    BUZZER = 1; // Turn buzzer back off (thank God)
  }

  // Turn off BILED
  BILED0 = 1;
  BILED1 = 1;

  while (!CheckPushButton4());
}

/*
 * Returns the number of overflows to wait for in on_time.
 *
 * NOTE: off_time is simply (on_time / 2).
 */
int CalculateMaxCounts(unsigned char x) {
  int on_time_millis = (x * 5) + 200;
  // This gives us the milliseconds to wait for.  In 16 bit counting mode,
  // based on SYSCLK, there are exactly 337.5 overflows per second, or
  // 0.3375 overflows per millisecond.  We multiply by this constant
  // to convert from milliseconds to overflows.
  return (on_time_millis * 0.3375);
}

/*
 * Light the LED for on_time, then wait for off_time.  Returns 0 if successful, or 1 if PushButton4 was pressed.
 */
unsigned char light_LED(unsigned char LED_to_light, short on_time, short off_time) {
  switch(LED_to_light) {
    case 0:
      // Light LED0
      LED0 = 0;
      break;
    case 1:
      // Light LED1
      LED1 = 0;
      break;
    case 2:
      // Light LED2
      LED2 = 0;
      break;
    default:
      printf("Invalid LED input: %d%s", LED_to_light, newline());
  }

  // Wait for on_time
  Counts = 0;
  while (Counts < on_time) {
    if (CheckPushButton4()) {
      return 1;
    }
  }

  // Turn LEDs off again
  LED0 = 1;
  LED1 = 1;
  LED2 = 1;

  // Wait for off_time
  Counts = 0;
  while (Counts < off_time) {
    if (CheckPushButton4()) {
      return 1;
    }
  }

  return 0; // Pushbutton 4 not pressed
}

/*
 * Waits for a pushbutton to be pressed, then returns which button was pressed (an integer between 0 and 3).
 */
unsigned char ReadPushbuttons() {
  while (1) {
    if (CheckPushButton1()) {
      Counts = 0;
      while (Counts < 25); // Wait for PushButton to be completely pressed.
      if (CheckPushButton1()) { // Check again, to make sure it was no accident
        while (CheckPushButton1()); // Wait until the button is released.
        Counts = 0;
        while (Counts < 25); // Wait for PushButton to be completely released.
        return 0;
      }
    }
    if (CheckPushButton2()) {
      Counts = 0;
      while (Counts < 25); // Wait for PushButton to be completely pressed.
      if (CheckPushButton2()) { // Check again, to make sure it was no accident
        while (CheckPushButton2()); // Wait until the button is released.
        Counts = 0;
        while (Counts < 25); // Wait for PushButton to be completely released.
        return 1;
      }
    }
    if (CheckPushButton3()) {
      Counts = 0;
      while (Counts < 25); // Wait for PushButton to be completely pressed.
      if (CheckPushButton3()) { // Check again, to make sure it was no accident
        while (CheckPushButton3()); // Wait until the button is released.
        Counts = 0;
        while (Counts < 25); // Wait for PushButton to be completely released.
        return 2;
      }
    }
    if (CheckPushButton4()) {
      Counts = 0;
      while (Counts < 25); // Wait for PushButton to be completely pressed.
      if (CheckPushButton4()) { // Check again, to make sure it was no accident
        while (CheckPushButton4()); // Wait until the button is released.
        Counts = 0;
        while (Counts < 25); // Wait for PushButton to be completely released.
        return 3;
      }
    }
  }
}

/*
 * Returns a random number different from last_state.
 */
unsigned char unique_random(unsigned char last_state) {
  unsigned char return_value;
  do {
    return_value = random(3);
  } while(return_value == last_state);
  return return_value;
}

/*
 * Returns a random integer between 0 and N-1 (a range of N numbers).
 */
unsigned char random(unsigned char N) {
  return (rand() % N);
}

/*
 * Waits 1 second.
 */
void wait_one_second(void) {
  Counts = 0;
  while (Counts < 338);
}

/*
 * Light BiLED green.
 */
void light_green(void) {
  BILED0 = 1;
  BILED1 = 0;
}

/*
 * :Light BiLED red.
 */
void light_red(void) {
  BILED0 = 0;
  BILED1 = 1;
}

/*
 * Returns a 0 if CheckPushButton1 not activated
 * or a 1 if CheckPushButton1 is activated.
 * This code reads a single input only, associated with PB1
 */
int CheckPushButton1(void) {
  // !PB1 will evaluate to 0 if PB1 is off, or 1 if PB1 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB1;
}

/*
 * Returns a 0 if push button 2 not activated
 * or a 1 if push button 2 is activated.
 * This code reads a single input only, associated with PB2
 */
int CheckPushButton2(void) {
  // !PB2 will evaluate to 0 if PB2 is off, or 1 if PB2 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB2;
}

/*
 * Returns a 0 if push button 3 not activated
 * or a 1 if push button 3 is activated.
 * This code reads a single input only, associated with PB3
 */
int CheckPushButton3(void) {
  // !PB3 will evaluate to 0 if PB3 is off, or 1 if PB3 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB3;
}

/*
 * Returns a 0 if push button 4 not activated
 * or a 1 if push button 4 is activated.
 * This code reads a single input only, associated with PB4
 */
int CheckPushButton4(void) {
  // !PB4 will evaluate to 0 if PB4 is off, or 1 if PB4 is on.  These are the
  // desired return values, so we'll just return the statement.
  return !PB2;
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
    printf("Pushbuttons 1 and 2 activated.");
  } else if (push1) {
    // Only pushbutton 1 pressed.
    printf("Pushbutton 1 activated.");
  } else if (push2) {
    // Only pushbutton 2 pressed.
    printf("Pushbutton 2 activated.");
  } else {
    // No pushbuttons pressed.
    printf("Pushbuttons 1 and 2 not activated.");
  }
  printf("%s", newline());
}

/*
 * Helper function to return a string containing a newline character and return
 * character.
 */
char* newline() {
  char* retval = "\r\n";
  return retval;
}