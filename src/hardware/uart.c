/*
 * uart.c
 *
 *  Created on: 30.04.2017
 *      Author: andre
 */

#include "uart.h"

#include <hardware/custom.h>
#include <stdio.h>
#include <stdarg.h>
#include <hardware/intbits.h>

extern volatile struct Custom custom;

int timeoutCnt=0;

void uart_printChar(char c)
{
	custom.serdat = c;

	timeoutCnt=0;
	while ((custom.intreqr & INTF_TBE)==0)
	{
		//Einfach warten
		timeoutCnt++;
		if (timeoutCnt > 5000)
		{

			/*
			asm (	"ILLEGAL\n"
				 : //no outputs
				 : //no inputs
				 : //no effects
				   );
			*/
		}
	}

	custom.intreq = INTF_TBE;
}

void uart_printf ( const char * format, ... )
{
	char buffer[256];
	va_list args;
	va_start (args, format);
	vsprintf (buffer,format, args);

	char *str=buffer;

	custom.serper = 100;
	while (*str)
	{
		uart_printChar(*str);

		str++;
	}

	va_end (args);

}

void uart_puts(char *str)
{
	while (*str)
	{
		uart_printChar(*str);

		str++;
	}
}

void uart_init()
{

}

