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

extern struct Custom custom;

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

		custom.serdat = *str;

		while ((custom.intreqr & INTF_TBE)==0)
		{
			//Einfach warten
		}

		custom.intreq = INTF_TBE;

		str++;
	}

	va_end (args);
}

void uart_init()
{

}

