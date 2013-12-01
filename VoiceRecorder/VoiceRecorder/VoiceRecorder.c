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

//DRESULT write_sector(DWORD lba) {
	//DRESULT res;
	//WORD bc;
	//res = RES_ERROR;
	//if (!(CardType & CT_BLOCK)) lba *= 512;	/* Convert to byte address if needed */
	//if (send_cmd(CMD24, lba) == 0) {		/* WRITE_SINGLE_BLOCK */
		//xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */
		//for(WORD i=0;i<512;i++) {			/* Data */
			//xmit_spi(1);
		//}
		//xmit_spi(0);						/* CRC */
		//xmit_spi(0);
		//if ((rcv_spi() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 500ms */
			//for (bc = 5000; rcv_spi() != 0xFF && bc; bc--) dly_100us();	/* Wait ready */
			//if (bc) res = RES_OK;
		//}
	//}		
	//DESELECT();
	//rcv_spi();
	//return res;
//}
//

DRESULT write_sector (DWORD sa) {			/* Sector number (LBA) */
	DRESULT res;
	WORD bc;
	static WORD wc;

	res = RES_ERROR;

	if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
	if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
		xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */
		wc = 512;							/* Set byte counter */

		bc = 12;
		while (bc && wc) {		/* Send data bytes to the card */
			xmit_spi('y');
			wc--; bc--;
		}

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

const BYTE direntry[] PROGMEM = {  'W',  'A',  'V',  'E',  'D',  'A',  'T',  'A',  'R',  'A', 'W', 0x20, 0x00, 0x7c, 0x88, 0x5e,
								  0x81, 0x43, 0x81, 0x43, 0x00, 0x00, 0xc7, 0xa8, 0x85, 0x42 };

DRESULT updateSize(DWORD filesize) {
	DRESULT res;
	WORD bc;
	static WORD wc;
	CLUST cluster = 0x00000708;
	
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
		xmit_spi(cluster&0xFF);				/* write cluster byte 0 */
		xmit_spi((cluster>>8)&0xFF);		/* write cluster byte 1 */

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
inline DRESULT formatFATsector(DWORD sa, CLUST * cluster_ptr, CLUST last_cluster) {
	DRESULT res;
	WORD bc;
	WORD wc;
	puts("formatFATsector");
	printf("cluster %ld\n", *cluster_ptr);
	printf("last_cluster %ld\n", last_cluster);
	
	res = RES_ERROR;
	sa += Fs.fatbase;
	if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
//	if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
//		xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */
		wc = 512;							/* Set byte counter */

		while(*cluster_ptr<last_cluster && wc) {
			CLUST c = * cluster_ptr;
			//xmit_spi(c&0xFF);			/* cluster byte 0 */
			//xmit_spi((c>>8)&0xFF);	/* cluster byte 1 */
			//xmit_spi((c>>16)&0xFF);	/* cluster byte 2 */
			//xmit_spi((c>>24)&0xFF);	/* cluster byte 3 */
			wc -= 4;
			(*cluster_ptr)++;
		}
		return RES_OK;
		///* end cluster 0x0fffffff */
		//xmit_spi(0xff);						
		//xmit_spi(0xff);						
		//xmit_spi(0xff);						
		//xmit_spi(0x0f);						
		//wc -= 4;
				//
		//bc = wc + 2;
		//while (bc--) xmit_spi(0);	/* fill left bytes and crc with zeros */
		//if ((rcv_spi() & 0x1f) == 0x05) {	/* receive data resp and wait for end of write process in timeout of 500ms */
			//for (bc = 5000; rcv_spi() != 0xff && bc; bc--) dly_100us();	/* wait ready */
			//if (bc) res = res_ok;
		//}
		//deselect();
		//rcv_spi();
	//}
	//
//
	return res;
}

CLUST size2cluster(DWORD size) {
	DWORD clust_byte_size = (DWORD)Fs.csize * 512;
	// printf("cbs %ld\n", clust_byte_size);
	DWORD rem = size % clust_byte_size;
	// printf("rem %ld\n", rem);
	CLUST c = size / clust_byte_size;
	if(rem>0) c++;
	return c;
}

/* cluster# 0 is for root directory, file starts at cluster# 1 */
#define CLUSTER_OFFSET 1

DRESULT updateFAT(DWORD old_filesize, DWORD new_filesize) {
	/* if(new_clusters >= Fs.n_fatent) return FR_DISK_ERR; */
	WORD fat_sector = (size2cluster(old_filesize)+2) / 128;					/* first FAT sector to format */
	CLUST cluster = (fat_sector * 128) + CLUSTER_OFFSET;					/* first cluster index to format */
	CLUST last_cluster = size2cluster(new_filesize) + CLUSTER_OFFSET;		/* last cluster index used by the file after recording */

	
	puts("updateFAT");
	printf("cluster %ld\n", cluster);
	printf("last_cluster %ld\n", last_cluster);
	printf("fat_sector %d\n", fat_sector);
	printf("last_fat_sector %d\n", last_fat_sector);

	do {
		if(formatFATsector(fat_sector, &cluster, last_cluster)!=RES_OK) return RES_ERROR;
		fat_sector++;
	} while(fat_sector<last_fat_sector);
	
	return RES_OK;
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

	if(updateFAT(453000, 16400800)) die(1);
	for(;;);
	if (updateSize(3612)) die(1);
	dump_sector(clust2sect(Fs.dirbase));
	for(;;);
	
	BYTE bf[4] = {'a','b','c','d'};
	
	if (disk_writep(0, 512)) die(1);	/* Initiate a sector write operation */
	if (disk_writep(bf, 4)) die(1);	/* Send data to the sector */
	if (disk_writep(0, 0)) die(1);	/* Finalize the currtent secter write operation */

	write_sector(512);
	dump_sector(512);
	for(;;);
	
/*	
	printf("Write and dump\n");
	if(write_sector(1000)) die(1);

	printf("Fat\n");
	dump_sector(Fs.fatbase);

	printf("Test\n");
	for(;;);
	
	//dump_sector(clust2sectFs.database + (Fs.dirbase-2) * Fs.csize);
	printf("Root Dir\n");
	dump_sector(clust2sect(Fs.dirbase));
	for(;;);
*/

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
		result = pf_write("Salut, ceci est amusant\r\n", 27, &bw);
		if (result || !bw) break;
	}
	if (result) die(result);

	printf("\nTerminate the file write process.\n");
	result = pf_write(0, 0, &bw);
	if (result) die(result);

	printf("Write and dump\n");
	if(write_sector(16448)) die(1);

	dump_sector(16448);
	
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

