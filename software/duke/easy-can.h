/*------------------------------------------------------------------------------
;
; Title:	`Hello world' demo for The Wellington Boot Loader
;
; Copyright:	Copyright (c) 2014 The Duke of Welling Town
;
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   This file is part of The Wellington Boot Loader.
;
;   The Wellington Boot Loader is free software: you can redistribute it and/or
;   modify it under the terms of the GNU General Public License as published
;   by the Free Software Foundation.
;
;   The Wellington Boot Loader is distributed in the hope that it will be
;   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License along
;   with The Wellington Boot Loader. If not, see http://www.gnu.org/licenses/
;-----------------------------------------------------------------------------*/

#ifndef _PROTON_MKII_H
#define _PROTON_MKII_H

#include <pic18fregs.h>

/*
 * WARNING: The target device seems to support XINST and no #pragma config XINST=OFF was found.
 *          The code generated by SDCC does probably not work when XINST is enabled (possibly by default).
 *          Please make sure to disable XINST.
 *          (If the target does not actually support XINST, please report this as a bug in SDCC.)
 */
#pragma config XINST=OFF

#pragma stack 0x0200 0x0100

#define LED_TRIS        (TRISAbits.TRISA0)
#define LED             (LATAbits.LATA0)

/* Serial UART */
#define BRG ((((FOSC / 500000) / 2) - 1) / 2)	/* BAUDRATE GENERATOR */

#define USE_STDIO (0)				/* printf to UART? */
#define USE_2STOP (0)				/* 2 stop bits?    */

/* Library routines */
#if USE_STDIO == 1
#include <stdio.h>
#include <stdlib.h>
#endif
#include <string.h>
#include <stdint.h>

/* Timing */
#define FOSC (64000000)				/* Oscillator frequency */
#define FCY  (FOSC / 4)				/* Instructions per second */

#define DELAY50U (FCY / 131147)			/* >50us delay for delay_tcy() */
#define DELAY10U (FCY / 727272)			/* >10us delay for delay_tcy() */

/* CAN Bus */
#define CAN_ID_ALL (1)				/* RX all messages, else   */
#define CAN_ID (0x666)		 		/* RX only this Message-Id */

/* #define CAN_BRG1 (0x0f)				 125kbaud @ 64 MHz */ 
#define CAN_BRG1 (0x07)				/* 250kbaud @ 64 MHz */
#define CAN_BRG2 (0xBC)
#define CAN_BRG3 (0x01)

#define CAN_LEN (8)				/* Message length */

#define CAN_WIN_TXB0 (0x08)			/* TX window */
#define CAN_WIN_TXB1 (0x06)
#define CAN_WIN_TXB2 (0x04)

#define CAN_WIN_RXB0 (0x00)			/* RX window */
#define CAN_WIN_RXB1 (0x0A)

#define CAN_RING_TX (3)				/* TX ring size */
#define CAN_RING_RX (2)				/* RX ring size */

#define CAN_DOUBLE_BUFFER_RX (1)		/* RXB0CON RB0DBEN */

#endif
