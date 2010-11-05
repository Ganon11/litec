#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init();
void interrupt_init(void);
void PCA_ISR (void) interrupt 9;
unsigned int ReadCompass (void);
void SMB_Init(void);
void Steer(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int PW_CENTER = 0xF550;	// 62800
unsigned int PW_MIN = 0xF985;		// 63877
unsigned int PW_MAX = 0xF0E1;		// 61665
unsigned int PW = 0;
unsigned int PCA_start = 36864;	
unsigned int Counts;
unsigned int current_heading;
unsigned char new_heading = 0;
unsigned int desired_heading = 2700;
sbit at 0xB7 SS;					//SS declared in Port 3 Pin 7

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
	// initialize board
	Sys_Init();
	putchar(' '); //the quotes in this line may not format correctly
	Port_Init();
	XBR0_Init();
	PCA_Init();
	SMB_Init();
	interrupt_init();

	printf("\rLab 3.3 start\r\n");

	while (1)							//The code counts 40 ms then reads the compass and prints out the heading
	{
		unsigned char Straight = 1;

		while (!SS)						//While the slide switch is off, the wheels turn to face forward
		{
			if (new_heading == 1)
			{
				PCA0CPL0 = PW_CENTER;
				PCA0CPH0 = PW_CENTER >> 8;
				new_heading = 0;
			}

			if (Straight == 1)			//Print "straight" only once per time the slide switch is off
			{
				printf("Straight\r\n");
				Straight = 0;
			}
		}

		while (SS)						//while the slide switch is on
		{
			if (new_heading == 1)		//every time new_heading = 1, read the compass and adjust the wheels accordingly
			{
				printf("Desired heading is %d\r\n", desired_heading);
				current_heading = ReadCompass();
				Steer();
				new_heading = 0;		//reset new_heading
			}
		}
	}
}


unsigned int ReadCompass()										//Fuction to read the electronic compass
{
	unsigned char addr = 0xC0;									//address of the sensor
	unsigned char Data[2];										//array with length of 2
	unsigned int heading;										//the heading returned in degrees between 0 and 3599
	i2c_read_data(addr, 2, Data, 2);							//reads 2 bytes into Data[]
	heading =(((unsigned int)Data[0] << 8) | Data[1]);			//combines the two numbers into degrees accurate to 1/10 of a degree
	printf("Current heading is %d\r\n", current_heading);		//prints current heading
	return heading;												//return heading (in degrees)
}

void Steer(void)						//Turn wheels so they point towards desired heading
{
	unsigned int k = 2;
	signed int error = desired_heading - current_heading;

	if (error < -1800)					//If error is too low (car spun around past 1 cycle), add 360 degrees
	{
		error += 3600;
	}

	else if (error > 1800)				//If error is too high, add 360 degrees
	{
		error -= 3600;
	}

	if (error <= 1800 && error > 0)		//If error is above 0, set PW based off of k factor
	{
		PW = PW_CENTER - k*(error)/3;
	}

	else if (error <= 0 && error >= -1800)	//If error is below 0, set PW based off of k factor
	{
		PW = k*(-error)/3 + PW_CENTER;
	}

	//printf("Error is %d\r\n", error);
	
	printf("Pulsewidth (PW) is %u\r\n\n", PW);				//Prints PW
	PCA0CPL0 = PW;
	PCA0CPH0 = PW >> 8;
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Port_Init()
{
	P1MDOUT |= 0x01;//set output pin for CEX0 in push-pull mode
	P3MDOUT &= ~0x80;
	P3 |= 0x80;
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init()
{
	XBR0 = 0x27; 	//configure crossbar as directed in the laboratory
						 		
}
 
//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void)
{
	PCA0CN = 0x40;
	PCA0CPM0 = 0xC2;
	PCA0MD = 0x81;

	// reference to the sample code in Example 4.5 -Pulse Width Modulation 
	// implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
}

void SMB_Init(void)
{
	SMB0CR=0x93;
	ENSMB = 1;
}

void interrupt_init(void)
{
	EA = 1;
	EIE1 |= 0x08;
}

//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR (void) interrupt 9
{
	if (CF) {
		PCA0L = 65536 - PCA_start;			// low byte of start count
		PCA0H = (65536 - PCA_start) >> 8;	// high byte of start count
		CF = 0;

		Counts++;

		if (Counts > 1)						//Every 2 counts (40ms), new_heading = 1
		{
			new_heading = 1;
			Counts = 0;
		}
	}

	PCA0CN &= 0xC0;
}