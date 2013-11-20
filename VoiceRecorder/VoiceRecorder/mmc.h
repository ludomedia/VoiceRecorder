/*
 * mmc.h
 *
 * Created: 20.11.2013 20:01:14
 *  Author: marcha
 */ 

#include "integer.h"
#include "diskio.h"

#ifndef MMC_H_
#define MMC_H_

#define SELECT()	PORTB &= ~_BV(3)	/* CS = L */
#define	DESELECT()	PORTB |=  _BV(3)	/* CS = H */
#define	MMC_SEL		!(PORTB &  _BV(3))	/* CS status (true:CS == L) */
#define	FORWARD(d)	xmit_spi(d)			/* Data forwarding function (Console out in this example) */

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_BLOCK			0x08	/* Block addressing */

void init_spi (void);
BYTE xmit_spi(BYTE ch);
BYTE rcv_spi(void);
void dly_100us();
BYTE send_cmd(BYTE cmd, DWORD arg);
DRESULT write_sector (DWORD sa);

BYTE CardType;

#endif /* MMC_H_ */