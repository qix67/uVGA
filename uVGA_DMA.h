/*
   This file is part of uVGA library.

   uVGA library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   uVGA library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with uVGA library.  If not, see <http://www.gnu.org/licenses/>.

   Copyright (C) 2017 Eric PREVOTEAU

   Original Author: Eric PREVOTEAU <digital.or@gmail.com>
*/

#ifndef _UVGA_DMA_H
#define _UVGA_DMA_H

#include <Arduino.h>
#include <avr_emulation.h>

typedef struct __attribute__((packed))
{
	volatile uint32_t CR;		// control
	volatile uint32_t ES;		// error status
	volatile uint32_t __empty0;	// empty area
	volatile uint32_t ERQ;		// enable request
	volatile uint32_t __empty1;	// empty area
	volatile uint32_t EEI;		// enable error interrupt

	volatile uint8_t CEEI;		// clear enable error interrupt
	volatile uint8_t SEEI;		// set enable error interrupt
	volatile uint8_t CERQ;		// clear enable request
	volatile uint8_t SERQ;		// set enable request
	volatile uint8_t CDNE;		// clear DONE status bit
	volatile uint8_t SSRT;		// set START bit
	volatile uint8_t CERR;		// clear error
	volatile uint8_t CINT;		// clear interrupt request

	volatile uint32_t __empty2;	// empty area
	volatile uint32_t INT;		// interrupt request
	volatile uint32_t __empty3;	// empty area
	volatile uint32_t ERR;		// error
	volatile uint32_t __empty4;	// empty area
	volatile uint32_t HRS;		// hardware request
	volatile uint32_t __empty5;	// empty area
	volatile uint32_t __empty6;	// empty area
	volatile uint32_t __empty7;	// empty area
	volatile uint32_t EARS;		// asynchronous request stop register
	
	volatile uint8_t __empty8[0x4008100-0x4008048];
	volatile uint8_t DCHPRI[16];		// channel n priority. WARNING: Order is 3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12
} EDMA_REGs;

#define EDMA_ADDR ((EDMA_REGs*)(&DMA_CR))

#endif
