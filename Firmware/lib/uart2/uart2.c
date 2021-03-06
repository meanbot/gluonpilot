/*! 
 *  @file     uart1.c
 *  @brief    Hardware functions for using the UART1 module
 *  @detailed This implementation uses a generic PID controller with some
 *            simplifications. For more information on PID controllers, please
 *            check wikipedia: http://en.wikipedia.org/wiki/PID_controller
 *  @author   Tom Pycke
 *  @date     23-nov-2008
 *  @since    0.1
 */
 
#include "microcontroller/microcontroller.h"
 
 
void uart2_open(long baud)
{
	// configure U2MODE
	U2MODEbits.UARTEN = 0;	// Bit15 TX, RX DISABLED, ENABLE at end of func
	U2MODEbits.USIDL = 0;	// Bit13 Continue in Idle
	U2MODEbits.IREN = 0;	// Bit12 No IR translation
	U2MODEbits.RTSMD = 1;	// Bit11 Simplex Mode
	U2MODEbits.UEN = 0;		// Bits8,9 TX,RX enabled, CTS,RTS not
	U2MODEbits.WAKE = 0;	// Bit7 No Wake up (since we don't sleep here)
	U2MODEbits.LPBACK = 0;	// Bit6 No Loop Back
	U2MODEbits.ABAUD = 0;	// Bit5 No Autobaud (would require sending '55')
	U2MODEbits.URXINV = 0;	// Bit4 IdleState = 1  (for dsPIC)
	U2MODEbits.BRGH = 0;	// Bit3 16 clocks per bit period
	U2MODEbits.PDSEL = 0;	// Bits1,2 8bit, No Parity
	U2MODEbits.STSEL = 0;	// Bit0 One Stop Bit
	
	// Load a value into Baud Rate Generator.  Example is for 9600.
	// See section 19.3.1 of datasheet.
	//  U2BRG = (Fcy/(16*BaudRate))-1
	//  U2BRG = (40MHz/(16*38400))-1
	//  U2BRG = 65
	
	U2BRG = (int)(FCY / (16*baud) - 1);	
	
	// Load all values in for U1STA SFR
	U2STAbits.UTXISEL1 = 0;	//Bit15 Int when Char is transferred (1/2 config!)
	
	U2STAbits.UTXINV = 0;	//Bit14 N/A, IRDA config
	U2STAbits.UTXISEL0 = 0;	//Bit13 Other half of Bit15
	U2STAbits.UTXBRK = 0;	//Bit11 Disabled
	U2STAbits.UTXEN = 1;	//Bit10 TX pins controlled by periph
	U2STAbits.URXISEL = 0;	//Bits6,7 Int. on character recieved
	U2STAbits.ADDEN = 0;	//Bit5 Address Detect Disabled

	IPC7 = 0x4400;	// Mid Range Interrupt Priority level, no urgent reason

//	IFS1bits.U1TXIF = 0;	// Clear the Transmit Interrupt Flag
	//IEC1bits.U1TXIE = 1;	// Enable Transmit Interrupts
//  IFS1bits.U1RXIF = 0;	// Clear the Recieve Interrupt Flag
	//IEC1bits.U1RXIE = 1;	// Enable Recieve Interrupts

	U2MODEbits.UARTEN = 1;	// And turn the peripheral on

	U2STAbits.UTXEN = 1;
	// I think I have the thing working now.
	
	TRISFbits.TRISF5 = 0;	// just make this an output
}


char uart2_dataready()
{
	return U2STAbits.URXDA;
}

	
void uart2_puts(char *str)
{
	while(*str != '\0')
	{
		while(U2STAbits.UTXBF)
			;  /* wait if the buffer is full */
		U2TXREG = *str++;   /* transfer data byte to TX reg */
	}
	while(U2STAbits.UTXBF)
		;
}

void uart2_putc(char c)
{
	U2TXREG = c;
}

char uart2_getc()
{
	return U2RXREG;
}
