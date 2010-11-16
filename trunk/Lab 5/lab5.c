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

//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void); // Initialize input/output ports.
void Interrupt_Init(void); // Set up PCA0 interrupts.
void XBR0_Init(void); // Initialize crossbar.
void SMB_Init(void); // Initialize system bus.
void PCA_Init (void); // Initialize the PCA counter.
void ADC_Init(void); // Initialize A/D Conversion