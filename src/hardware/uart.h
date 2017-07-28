/*
 * uart.h
 *
 *  Created on: 30.04.2017
 *      Author: andre
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_


void uart_printf ( const char * format, ... );
void uart_printChar(char c);
void uart_puts(const char *str);

#define uart_assert(cond) \
	if (!(cond)) \
	{ \
		uart_printf("ASSERT failed at %s:%d %s\n",__FILE__,__LINE__,__func__); \
		for(;;); \
	}

#endif /* SRC_UART_H_ */
