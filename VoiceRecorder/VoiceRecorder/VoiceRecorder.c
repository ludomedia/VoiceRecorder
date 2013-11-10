/*
 * VoiceRecorder.c
 *
 * Created: 27.10.2013 18:17:32
 * Author: marcha
 */ 

// -pattiny85 -C"C:\$(Device)Program Files (x86)\Arduino\hardware\tools\avr\etc/avrdude.conf" -v -v -carduino -P\\.\COM4 -b19200 -Uflash:w:"$(ProjectDir)Debug\$(ItemFileName).hex":i
// -patmega328p -P\\.\COM8 -C"C:\$(Device)Program Files (x86)\Arduino\hardware\tools\avr\etc/avrdude.conf" -v -v -carduino -b115200 -Uflash:w:"$(ProjectDir)Debug\$(ItemFileName).hex":i
#define F_CPU 16000000

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#include "uart.h"
#include "pff.h"

#define LED_PIN PB5

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

FATFS Fs;			/* File system object */
DIR Dir;			/* Directory object */
FILINFO Fno;		/* File information */
const char * dir;

void die (FRESULT rc) {
	printf("Failed with rc=%u.\n", rc);
	for (;;) ;
}

int main(void)
{
	uart_init();
	stdout = &uart_output;
	stdin = &uart_input;
	
	char input;
	BYTE buff[64];
	WORD bw, br, i;
	
	/*while(1) {
		puts("Hello world 2!");
		input = getchar();
		printf("You wrote %c\n", input);
	}*/

	puts("Start");

	/* Mount volume */
	FRESULT result = pf_mount(&Fs);
	if(result) die(result);

	/*
	printf("\nOpen a test file (ACCESS.LOG).\n");
	result = pf_open("ACCESS.LOG");
	if (result) die(result);

	printf("\nType the file content.\n");
	for (;;) {
		result = pf_read(buff, sizeof(buff), &br);	// Read a chunk of file
		if (result || !br) break;					// Error or end of file
		for (i = 0; i < br; i++) putchar(buff[i]);	// Type the data
	}
	if (result) die(result);
	*/
	
	printf("\nOpen a file to write (write.txt).\n");
	result = pf_open("ACCESS.LOG");
	if (result) die(result);

	printf("\nWrite a text data. (Hello world!)\n");
	//for (;;) {
		result = pf_write("Bonjour, ceci est un test\r\n", 27, &bw);
		//if (result || !bw) break;
	//}
	if (result) die(result);

	printf("\nTerminate the file write process.\n");
	result = pf_write(0, 0, &bw);
	if (result) die(result);
	
	for(;;);
	
		/*
		result = pf_opendir(&Dir, dir = "");
		printf("Result %d", result);
		if(result == FR_OK) {				// Repeat in the dir
			result = pf_readdir(&Dir, 0);			// Rewind dir
			while (result == FR_OK) {				// Play all wav files in the dir
				result = pf_readdir(&Dir, &Fno);	// Get a dir entry
				if (result || !Fno.fname[0]) break;	// Break on error or end of dir
				//if (!(Fno.fattrib & (AM_DIR|AM_HID)) && strstr(Fno.fname, ".WAV"))
				//res = play(dir, Fno.fname);		// Play file
				printf("Dir %s", Fno.fname);
				puts("OK");
			}
			puts("End Of Dir");
		}		
		*/
		
	return 0;
		
}
