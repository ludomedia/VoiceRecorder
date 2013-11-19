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

#include "diskio.h"
#include "uart.h"
#include "pff.h"
#include "analog.h"

// #define LED_PIN PB5

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

FATFS Fs;			/* File system object */
DIR Dir;			/* Directory object */
FILINFO Fno;		/* File information */
const char * dir;

BYTE buff[64];

void die (FRESULT rc) {
	printf("Failed with rc=%u.\n", rc);
	for (;;) ;
}

void dump_sector(DWORD lba) {
	FRESULT result;
	for(int o=0;o<512;o+=16) {
		printf("%06x: ", o);
		result = disk_readp(buff, lba, o, 16);
		if(result) die(result);
		for(int i=0;i<16;i++) printf("%02x ", buff[i]);
		printf(" ");
		for (int i=0; i<16; i++) printf("%c", isprint(buff[i]) ? buff[i] : '.');
		printf("\n");
	}
}

int main(void)
{
	uart_init();
	//adc_init();
	//sei();
	
	stdout = &uart_output;
	stdin = &uart_input;
	
	WORD bw;
	
	/*while(1) {
		puts("Hello world 2!");
		input = getchar();
		printf("You wrote %c\n", input);
	}*/

	puts("Start");




	/* Mount volume */
	FRESULT result = pf_mount(&Fs);
	if(result) die(result);
	
	
	printf("fs_type %d\n", Fs.fs_type);
	printf("csize %d\n", Fs.csize);
	printf("n_rootdir %d\n", Fs.n_rootdir);
	printf("n_fatent %ld\n", Fs.n_fatent);
	printf("fatbase %ld\n", Fs.fatbase);
	printf("dirbase %ld\n", Fs.dirbase);
	printf("database %ld\n", Fs.database);
	
	printf("Fat\n");
	dump_sector(Fs.fatbase);
	//dump_sector(clust2sectFs.database + (Fs.dirbase-2) * Fs.csize);
	printf("Root Dir\n");
	dump_sector(clust2sect(Fs.dirbase));
	for(;;);

//typedef struct {
	//BYTE	fs_type;	/* FAT sub type */
	//BYTE	flag;		/* File status flags */
	//BYTE	csize;		/* Number of sectors per cluster */
	//BYTE	pad1;
	//WORD	n_rootdir;	/* Number of root directory entries (0 on FAT32) */
	//CLUST	n_fatent;	/* Number of FAT entries (= number of clusters + 2) */
	//DWORD	fatbase;	/* FAT start sector */
	//DWORD	dirbase;	/* Root directory start sector (Cluster# on FAT32) */
	//DWORD	database;	/* Data start sector */
	//DWORD	fptr;		/* File R/W pointer */
	//DWORD	fsize;		/* File size */
	//CLUST	org_clust;	/* File start cluster */
	//CLUST	curr_clust;	/* File current cluster */
	//DWORD	dsect;		/* File current data sector */
//} FATFS;
//

	for(;;);

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
	for (;;) {
		//result = pf_write("Bonjour, ceci est un test\r\n", 27, &bw);
		result = pf_write("Bonjour, ceci est un test\r\n", 27, &bw);
		if (result || !bw) break;
	}
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

