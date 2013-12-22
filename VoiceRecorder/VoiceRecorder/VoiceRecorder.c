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
#include <avr/pgmspace.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "diskio.h"
#include "uart.h"
#include "mmc.h"
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
	printf("Dumping sector %ld\n", lba);
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

DWORD filesize;

DRESULT read_filesize ()
{
	DRESULT res;
	BYTE rc;
	WORD bc;
	BYTE b0 = 0;
	BYTE b1 = 0;
	BYTE b2 = 0;
	BYTE b3 = 0;
	DWORD sa = clust2sect(Fs.dirbase);
	if (!(CardType & CT_BLOCK)) sa *= 512;		/* Convert to byte address if needed */
	res = RES_ERROR;
	if (send_cmd(CMD17, sa) == 0) {		/* READ_SINGLE_BLOCK */
		bc = 40000;
		do {							/* Wait for data packet */
			rc = rcv_spi();
		} while (rc == 0xFF && --bc);

		if (rc == 0xFE) {				/* A data packet arrived */
			printf("packe arrived\n");
			bc = 0;
			do {
				rc = rcv_spi();
				if(bc<0x20) {					/* read only first directory entry */
					if(bc==0x1c) b0 = rc;
					else if(bc==0x1d) b1 = rc;
					else if(bc==0x1e) b2 = rc;
					else if(bc==0x1f) b3 = rc;
				}
				bc++;	
			} while(bc<514);
			
			filesize = (DWORD)b3<<24 | (DWORD)b2<<16 | (DWORD)b1<<8 | (DWORD)b0;
			
			res = RES_OK;
		}
	}

	DESELECT();
	rcv_spi();

	return res;
}

const BYTE direntry[] PROGMEM = {  'W',  'A',  'V',  'E',  'D',  'A',  'T',  'A',  'R',  'A', 'W', 0x20, 0x00, 0x7c, 0x88, 0x5e,
								  0x81, 0x43, 0x81, 0x43, 0x00, 0x00, 0xc7, 0xa8, 0x85, 0x42 };

DRESULT updateDirEntry(CLUST start_cluster, DWORD filesize) {
	DRESULT res;
	WORD bc;
	WORD wc;
	
	/* format first directory sector with one file named WAVEDATA.RAW */
	DWORD sa = clust2sect(Fs.dirbase);
	res = RES_ERROR;
	if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
	if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
		xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */
		wc = 512;							/* Set byte counter */

		for(BYTE i=0;i<sizeof(direntry);i++) {
			xmit_spi(pgm_read_byte(&direntry[i]));
			wc--;
		}
		
		/* write cluster# where file data starts. if FAT32, cluster high word is initialized to 0 in direntry array */
		xmit_spi(start_cluster&0xFF);		/* write cluster byte 0 */
		xmit_spi((start_cluster>>8)&0xFF);	/* write cluster byte 1 */

		/* write filesize */
		xmit_spi(filesize&0xFF);			/* filesize byte 0 */
		xmit_spi((filesize>>8)&0xFF);		/* filesize byte 1 */
		xmit_spi((filesize>>16)&0xFF);		/* filesize byte 2 */
		xmit_spi((filesize>>24)&0xFF);		/* filesize byte 3 */

		wc -= 6;

		bc = wc + 2;
		while (bc--) xmit_spi(0);	/* Fill left bytes and CRC with zeros */
		if ((rcv_spi() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 500ms */
			for (bc = 5000; rcv_spi() != 0xFF && bc; bc--) dly_100us();	/* Wait ready */
			if (bc) res = RES_OK;
		}
		DESELECT();
		rcv_spi();
	}

	return res;	
}

/* format a FAT sector by building the corresponding part of an ordered cluster list */
inline DRESULT formatFATsector(DWORD fatsector, CLUST last_cluster) {
	DRESULT res;
	WORD bc;

	puts("formatFATsector");
	printf("fatsector %ld\n", fatsector);
	printf("last_cluster %ld\n", last_cluster);

	DWORD sa = fatsector  + Fs.fatbase;
	CLUST index = fatsector * 128;
	last_cluster += 3; // skip two reserved cluster and root directory cluster
	CLUST c;

	res = RES_ERROR;
	if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
	printf("sector %ld\n", sa);
	if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
		xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */

		for(bc=0;bc<512;bc+=4) {
			index++;
			c = index;
			if(c==last_cluster) c = 0x0fffffff; /* end of cluster list */
			else if(c>last_cluster) c = 0;
			if(fatsector==0) {
				if(bc==0) c = 0x0ffffff8;
				else if(bc<12) c = 0x0fffffff;
			}
			xmit_spi(c&0xFF);		/* cluster byte 0 */
			xmit_spi((c>>8)&0xFF);	/* cluster byte 1 */
			xmit_spi((c>>16)&0xFF);	/* cluster byte 2 */
			xmit_spi((c>>24)&0xFF);	/* cluster byte 3 */		
		}
		printf("index %ld\n", index);

		xmit_spi(0); xmit_spi(0);	/* send CRC with 0 */
		if ((rcv_spi() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 500ms */
			for (bc = 5000; rcv_spi() != 0xFF && bc; bc--) dly_100us();	/* Wait ready */
			if (bc) res = RES_OK;
		}
		DESELECT();
		rcv_spi();
	}

	return res;
}

CLUST size2clusters(DWORD size) {
	//printf("-- size %ld\n", size);
	DWORD clust_byte_size = (DWORD)Fs.csize * 512;
	//printf("-- cbs %ld\n", clust_byte_size);
	DWORD rem = size % clust_byte_size;
	//printf("-- rem %ld\n", rem);
	CLUST c = size / clust_byte_size;
	if(rem>0) c++;
	//printf("-- c %ld\n", c);
	return c;
}

/* cluster# 0 is for root directory, file starts at cluster# 1 */
#define CLUSTER_START 3

DRESULT updateFAT(DWORD old_filesize, DWORD new_filesize) {
	
	puts("> updateFAT");

	if(updateDirEntry(CLUSTER_START, new_filesize)) return RES_ERROR;
	
	CLUST clusters = size2clusters(new_filesize);								/* number of clusters used by the file after recording */
	/* if(new_clusters >= Fs.n_fatent) return FR_DISK_ERR; */
	WORD fatsector = (size2clusters(old_filesize) + CLUSTER_START) / 128;		/* last fatsector formated during the previous recording */
	WORD last_fatsector = (clusters + CLUSTER_START) / 128;						/* last fatsector to format after recording */

	printf("clusters %ld\n", clusters);
	printf("first fatsectorr %d\n", fatsector);
	printf("last fatsector %d\n", last_fatsector);

	do {
		if(formatFATsector(fatsector, clusters)!=RES_OK) return RES_ERROR;
		// dump_sector(Fs.fatbase + fatsector);
		fatsector++;
	} while(fatsector<=last_fatsector);
	
	return RES_OK;
}

/* write analog data to current sector */
inline DRESULT writeData(DWORD sector) {
	DRESULT res;
	WORD bc;

	//puts("WriteData");
	//printf("sector %ld\n", sector);

	DWORD sa = sector + Fs.database;
	sa += Fs.csize; // skip 1 sector (root dir)

	res = RES_ERROR;
	if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
	if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
		xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */

		for(bc=0;bc<512;bc++) {				/* fill block with analog data */
			xmit_spi(adc_buffer_read());
		}		
		
		xmit_spi(0); xmit_spi(0);	/* send CRC with 0 */
		if ((rcv_spi() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 500ms */
			for (bc = 5000; rcv_spi() != 0xFF && bc; bc--) dly_100us();	/* Wait ready */
			if (bc) res = RES_OK;
		}
		DESELECT();
		rcv_spi();
	}

	return res;
}

int main(void)
{
	uart_init();
	
	stdout = &uart_output;
	stdin = &uart_input;
	
	WORD bw;
	
	/*while(1) {
		puts("Hello world 2!");
		input = getchar();
		printf("You wrote %c\n", input);
	}*/
	DDRD |= (1<<2);
	
	puts("Ready");
	getchar();
	
	/* Mount volume */
	FRESULT result = pf_mount(&Fs);
	if(result) die(result);
/*		
	printf("fs_type %d\n", Fs.fs_type);
	printf("csize %d\n", Fs.csize);
	printf("n_rootdir %d\n", Fs.n_rootdir);
	printf("n_fatent %ld\n", Fs.n_fatent);
	printf("fatbase %ld\n", Fs.fatbase);
	printf("dirbase %ld\n", Fs.dirbase);
	printf("database %ld\n", Fs.database);
	//printf("Fat\n");
	//dump_sector(Fs.fatbase);
	for(int s = 0;s<80;s+=16) {
		dump_sector(s + Fs.database);
	}			
	for(;;);
*/	
	if(read_filesize()) die(1);
	//filesize = 0; // TODO remove to append
	printf("current filesize %ld\n", filesize);
	DWORD old_filesize = filesize;
	DWORD sector = filesize / 512; // filesize % 512 is supposed to be 0

	adc_init();
	sei();
	for(int s=0;s<80;s++) { /* record 80 sectors = 5 s */
		if(writeData(sector)) die(1);
		sector++;
		filesize+=512;
	}
	cli();	
	//if (updateSize(3612)) die(1);
	//DWORD filesize = ((DWORD)1024*1024*15) + 1;
	if(updateFAT(old_filesize, filesize)) die(1);

	puts("Done");
//	dump_sector(clust2sect(Fs.dirbase));
	//if(formatFATsector(0, 7)) die(1);
	//dump_sector(Fs.fatbase);
	for(;;);

/*	if(updateFAT(453000, 16400800)) die(1);
	for(;;);
	if (updateSize(3612)) die(1);
	dump_sector(clust2sect(Fs.dirbase));
	for(;;);
*/	
/*	
	printf("Write and dump\n");
	if(write_sector(1000)) die(1);


	printf("Test\n");
	for(;;);
	
	//dump_sector(clust2sectFs.database + (Fs.dirbase-2) * Fs.csize);
	printf("Root Dir\n");
	dump_sector(clust2sect(Fs.dirbase));
	for(;;);
*/

	return 0;
		
}

