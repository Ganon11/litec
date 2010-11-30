/*
	Section:
	Date:
	File name:
	Program description:
*/

#include <c8051_SDCC.h>
#include <stdio.h>

/* Global Variables */

unsigned int desired = 1350;			// set this value
unsigned int kp = 12;
unsigned int kd = 0;
int pw_neut = 2750;					// set this value
int previous_error = 20;				// set this value
int error = 0;
long temp_motorpw_alg6 = 0;
int corrected_motorpw;

void main()
{
  unsigned int i;
	Sys_Init();
	putchar(' ');
  for (i = 0; i <= 3150; i += 450) {
  	error = desired - i;
	  temp_motorpw_alg6 = (long)pw_neut + (long)kp * (long)error + (long)kd * (long)(error - previous_error);
    if (temp_motorpw_alg6 > 3500) {
      corrected_motorpw = 3500;
    } else if (temp_motorpw_alg6 < 2000) {
      corrected_motorpw = 2000;
    } else {
      corrected_motorpw = temp_motorpw_alg6;
    }
  	printf("\rHeading %d, motorpw %ld, corrected %d\n", i, temp_motorpw_alg6, corrected_motorpw);
  }
}