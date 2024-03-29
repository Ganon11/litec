/*
 * Names: Michael Stark and David Melecio-Vazquez
 * Section: 4 (Side A)
 * Date: 8 October 2010
 * File name: worksheet6.c
 * Program description:
 * This code produces a pulse width modulated (PWM) on Port 1, Pin 1.
 * Capture/Compare Module 1 is used to produce the signal. The Crossbar
 * setting XBR0 = 0x27 sets the comparator output, CEX1, to Port 1, Pin 1.
 * The frequency of the signal is within the acoustic range of the human
 * ear, enabling a listener to hear the tone. The BILED will cycle between
 * red and green. However, if the pulse width is large or small, one color
 * will be present for a larger amount of time and the human eye will 
 * have an easier time discerning that color.
 */

#include<c8051_SDCC.h>
#include<stdio.h>

#define pw_min 1000
#define pw_max 65000

void PCA_Init (void);
void XBR0_Init (void);

unsigned int PW = 5000; // pulsewidth for speed controller
unsigned int PCA_start = 10000; // start count for PCA

void main() {
  unsigned char input;
  Sys_Init(); // initial functions in the blimp_init.h file
  putchar(' ');
  XBR0_Init(); // initialize XBAR
  PCA_Init (); // initialize PCA
  printf("\r\n Start    ");
  printf("\r\n a - increases PCA start count by 1000");
  printf("\r\n s - decreases PCA start count by 1000");
  printf("\r\n k - increases pulse width by 1000");
  printf("\r\n l - decreases pulse width by 1000");
  PCA0CPL1 = 65535 - PW; // set initial pulse width
  PCA0CPH1 = (65535 - PW) >> 8;
  while(1) {
    input = getchar();
    if (input == 'a') if (PCA_start < pw_max) PCA_start += 1000;
    if (input == 's') if (PCA_start > pw_min) PCA_start -= 1000;
    if (input == 'k') {
      if (PW < pw_max) {
        PW += 1000;
        PCA0CPL1 = 65535 - PW; // change pulse width
        PCA0CPH1 = (65535 - PW) >> 8;
      }
    }
    if (input == 'l') {
      if (PW > pw_min) {
        PW -= 1000;
        PCA0CPL1 = 65535 - PW; // change pulse width
        PCA0CPH1 = (65535 - PW) >> 8;
      }
    }
    printf(" PCA_start = %u  PW = %u      \r\n", PCA_start,PW);
  }
}

void XBR0_Init() {
  XBR0 = 0x27; // set up URART0, SPI, SMB, and CEX 0-3
}

void PCA_Init() {
  PCA0MD = 0x81; // SYSCLK/12, enable CF interrupts, suspend when idle
  PCA0CPM1 = 0xC2; // 16 bit, enable compare, enable PWM
  EIE1 |= 0x08; // enable PCA interrupts
  PCA0CN |= 0x40; // enable PCA
  EA = 1; // enable all interrupts
}

void PCA_ISR (void) interrupt 9 {
  if (CF) {
    PCA0L = PCA_start; // low byte of start count
    PCA0H = PCA_start>>8; // high byte of start count
    CF = 0; // Very important - clear interrupt flag
  }
  else PCA0CN &= 0xC0; // all other type 9 interrupts
}