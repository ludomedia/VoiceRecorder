/*
 * SerialTest.c
 *
 * Created: 27.10.2013 18:17:32
 *  Author: marcha
 */ 

// -pattiny85 -C"C:\$(Device)Program Files (x86)\Arduino\hardware\tools\avr\etc/avrdude.conf" -v -v -carduino -P\\.\COM4 -b19200 -Uflash:w:"$(ProjectDir)Debug\$(ItemFileName).hex":i
// -patmega328p -P\\.\COM8 -C"C:\$(Device)Program Files (x86)\Arduino\hardware\tools\avr\etc/avrdude.conf" -v -v -carduino -b115200 -Uflash:w:"$(ProjectDir)Debug\$(ItemFileName).hex":i
#define F_CPU 16000000

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "uart.h"

#define LED_PIN PB5

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

int main(void)
{
	uart_init();
	stdout = &uart_output;
	stdin = &uart_input;
	    
	char input;

	while(1) {
		puts("Hello world!");
		input = getchar();
		printf("You wrote %c\n", input);
	}
	    
	return 0;
		
	DDRB = (1<<LED_PIN);
    while(1)
    {
		PORTB ^= (1<<LED_PIN);
		_delay_ms(500);
        //TODO:: Please write your application code 
    }
}