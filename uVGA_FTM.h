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

#ifndef _UVGA_FTM_H
#define _UVGA_FTM_H

#include <Arduino.h>
#include <avr_emulation.h>

typedef struct __attribute__((packed))
{
	volatile uint32_t SC;	// Channel Status And Control
	volatile uint32_t V;		// Channel Value
} FTM_CHANNEL_REGS_t;

typedef struct __attribute__((packed))
{
	volatile uint32_t SC;		// status and control
	volatile uint32_t CNT;		// Counter
	volatile uint32_t MOD;     // Modulo
	FTM_CHANNEL_REGS_t C[8];	// Channels Status and Control
	volatile uint32_t CNTIN;	// Counter Initial Value
	volatile uint32_t STATUS;	// Capture And Compare Status
	volatile uint32_t MODE;		// Features Mode Selection
	volatile uint32_t SYNC;		// Synchronization
	volatile uint32_t OUTINIT;	// Initial State For Channels Output
	volatile uint32_t OUTMASK;	// Output Mask
	volatile uint32_t COMBINE;	// Function For Linked Channels
	volatile uint32_t DEADTIME;// Deadtime Insertion Control
	volatile uint32_t EXTTRIG;	// FTM External Trigger
	volatile uint32_t POL;		// Channels Polarity
	volatile uint32_t FMS;		// Fault Mode Status
	volatile uint32_t FILTER;	// Input Capture Filter Control
	volatile uint32_t FLTCTRL;	// Fault Control
	volatile uint32_t QDCTRL;	// Quadrature Decoder Control And Status
	volatile uint32_t CONF;		// Configuration
	volatile uint32_t FLTPOL;	// FTM Fault Input Polarity
	volatile uint32_t SYNCONF;	// Synchronization Configuration
	volatile uint32_t INVCTRL;	// FTM Inverting Control
	volatile uint32_t SWOCTRL;	// FTM Software Output Control
	volatile uint32_t PWMLOAD;	// FTM PWM Load
} FTM_REGS_t;

// values taken from kinetis.h of teensyduino
#define FTM0_ADDR ((FTM_REGS_t*)(&FTM0_SC))
#define FTM1_ADDR ((FTM_REGS_t*)(&FTM1_SC))

#if defined(FTM2_SC)
#define FTM2_ADDR ((FTM_REGS_t*)(&FTM2_SC))
#else
#define FTM2_ADDR NULL
#endif

#if (defined(FTM3_SC) && defined(DMAMUX_SOURCE_FTM3_CH0))
#define FTM3_ADDR ((FTM_REGS_t*)(&FTM3_SC))
#define HAVE_FTM3_ADDR
#else
#define FTM3_ADDR NULL
#endif

#endif
