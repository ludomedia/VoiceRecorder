/*
 * uart.h
 *
 * Created: 27.10.2013 19:03:45
 *  Author: marcha
 */ 
#ifndef _UART_H_
#define	_UART_H_ 1

#include <avr/io.h>
#include <stdio.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif

#include <util/setbaud.h>

void uart_init(void);
void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

#endif