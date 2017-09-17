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

#include "uVGA.h"

#define dump(v)      {DPRINT(#v ":"); DPRINTLN(v);}
#define dp(str,v) {DPRINT(str ":"); DPRINTLN(v,HEX);}
#define dp_nonl(str,v)  {DPRINT(str ":"); DPRINT(v,HEX); DPRINT(" ");}

#define NPRINT(args...)    Serial.print(args)
#define NPRINTLN(args...)  Serial.println(args)

//#define DEBUG
#ifdef DEBUG

#define DPRINT(args...)    Serial.print(args)
#define DPRINTLN(args...)  Serial.println(args)

#define LED_PIN 13
#define LED_INIT           pinMode(LED_PIN, OUTPUT);

#define LED_ON             digitalWrite(LED_PIN, HIGH);
#define LED_OFF            digitalWrite(LED_PIN, LOW);

#define LED_BLINK(duration)                              \
                           digitalWrite(LED_PIN, HIGH);  \
                           delay(duration);              \
                           digitalWrite(LED_PIN, LOW);

#else

#define DPRINT(args...)
#define DPRINTLN(args...)

#define LED_INIT
#define LED_ON
#define LED_OFF
#define LED_BLINK(duration)
#endif

// ============================================================================
// DMA configuration when only 1 DMA channel is used
uvga_error_t uVGA::rgb332_dma_init_dma_single_repeat_1()
{
	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("rgb332_dma_init_dma_single_repeat_1");

	// the number of major loop of the first DMA channel is:
	// 1 major loop for image containing img_h minor loop copying fb_row_stride bytes + 3 major loop for VBlanking (1 before sync, 1 during sync and 1 after sync)
	// some modeline has no delay between end of image en begin of vsync. The library supports this and discard the first vblank TCD
	px_dma_nb_major_loop = 1 + 3;

	sram_u_dma_nb_major_loop = 0;

	px_dma_major_loop = (DMABaseClass::TCD_t*)alloc_32B_align(sizeof(DMABaseClass::TCD_t) * px_dma_nb_major_loop);

	if(px_dma_major_loop == NULL)
		return UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER;

	cur_tcd = px_dma_major_loop;

	// 1) build TCD to display lines and do Vsync

	// here, 1 TCD exists per image, minor loop displays 1 line, major loop repeats for all image lines. Then, scatter/gather mode switch to the next TCD

	// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
	cur_tcd->SADDR = fb_row_pointer[0];	// source is line 't' of frame buffer or DMA indirection
	cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
	cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
	cur_tcd->SLAST = -img_h * fb_row_stride;	// at end of major loop, move start address back to its initial position

	cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
	cur_tcd->CITER = img_h;					// major loop should transfer all lines
	cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
	cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
	cur_tcd->BITER = cur_tcd->CITER;

	cur_tcd++;

	// Vblanking TCD configuration
	last_tcd = dma_append_vsync_tcds(cur_tcd);

	// with this configuration, the DMA will permanently running between screen TCD and VBlanking TCD due to scatter/gather list looping itself
	// the number of bytes sent during 1 frame time is img_h * fb_row_stride + (scr_h - img_h) * sizeof(uint32_t)

	*px_dmamux = 0;								// disable DMA channel

	memcpy((void*)px_dma, px_dma_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA
	dump_tcd((DMABaseClass::TCD_t*)px_dma);

	return UVGA_OK;
}

// ============================================================================
// DMA configuration when only 1 DMA channel is used and repeat_line > 1
uvga_error_t uVGA::rgb332_dma_init_dma_single_repeat_more_than_1()
{
	int t;
	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("rgb332_dma_init_dma_single_repeat_more_than_1");

	// the number of major loop of the first DMA channel is:
	// 1 major loop per line + 3 major loop for VBlanking (1 before sync, 1 during sync and 1 after sync)
	// some modeline has no delay between end of image en begin of vsync. The library supports this and discard the first vblank TCD
	px_dma_nb_major_loop = img_h + 3;

	sram_u_dma_nb_major_loop = 0;

	// to reduce memory waste due to data alignment, both pixel, sram_u and sram_u_fix TCD are allocated simultaneously
	px_dma_major_loop = (DMABaseClass::TCD_t*)alloc_32B_align(sizeof(DMABaseClass::TCD_t) * (px_dma_nb_major_loop));

	if(px_dma_major_loop == NULL)
		return UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER;

	cur_tcd = px_dma_major_loop;

	// 1) build TCD to display lines and do Vsync

	// here, 1 TCD exists per line, minor loop displays 1 line. Then, scatter/gather mode switch to the next TCD
	for(t = 0; t < img_h ; t++)
	{
		// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
		cur_tcd->SADDR = fb_row_pointer[t];	// source is line 't' of frame buffer or DMA indirection
		cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
		cur_tcd->CITER = 1;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
		cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
		cur_tcd->BITER = cur_tcd->CITER;

		cur_tcd++;
	}

	// Vblanking TCD configuration
	last_tcd = dma_append_vsync_tcds(cur_tcd);

	// with this configuration, the DMA will permanently running between screen TCD and VBlanking TCD due to scatter/gather list looping itself
	// the number of bytes sent during 1 frame time is img_h * fb_row_stride + (scr_h - img_h) * sizeof(uint32_t)
	*px_dmamux = 0;								// disable DMA channel

	memcpy((void*)px_dma, px_dma_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA
	dump_tcd((DMABaseClass::TCD_t*)px_dma);

	return UVGA_OK;
}

// ============================================================================
// DMA configuration when multiple DMA channels are used and repeat_line = 1
uvga_error_t uVGA::rgb332_dma_init_dma_multiple_repeat_1()
{
	int t;
	DMABaseClass::TCD_t *cur_tcd;
	int nb_sram_u_fb_lines;

	DPRINTLN("rgb332_dma_init_dma_multiple_repeat_1");

	nb_sram_u_fb_lines = (img_h - first_line_in_sram_u);		// number of lines of frame buffer in SRAM_U

	// the number of major loop of the first DMA channel is... complex to compute.
	// ... or not :)
	// the frame requires 1+N major loops + 3 major loops for VBlanking (1 before sync, 1 during sync and 1 after sync)
	// the image uses 1+N major loops:
	// The 1st major loop copies all lines located in SRAM_L. When this major loop ends, it starts the 2nd DMA to copy 1 line from SRAM_U to SRAM_L buffer.
	// The N major loop copies 1 line located in SRAM_L buffer. N is the number of line in SRAM_U. When this major loop ends, it starts then 2nd DMA to copy 1 line from SRAM_U to SRAM_L buffer
	px_dma_nb_major_loop = 1 + (img_h - first_line_in_sram_u) + 3;

	// In this case, 2nd and 3rd DMA channel have only 1 TCD, it is not necessary to allocated them in RAM
	// at least 1 line is in SRAM_U else we would be in this function
	sram_u_dma_nb_major_loop = 0;
	sram_u_dma_major_loop = NULL;
	sram_u_dma_fix_major_loop = NULL;

	// to reduce memory waste due to data alignment, both pixel, sram_u and sram_u_fix TCD are allocated simultaneously
	px_dma_major_loop = (DMABaseClass::TCD_t*)alloc_32B_align(sizeof(DMABaseClass::TCD_t) * (px_dma_nb_major_loop));

	if(px_dma_major_loop == NULL)
		return UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER;

	cur_tcd = px_dma_major_loop;

	// 1) build TCD to display lines and do Vsync

	// here, 1 TCD exists per line, minor loop displays 1 line. Then, scatter/gather mode switch to the next TCD

	// first TCD copies SRAM_L lines
	cur_tcd->SADDR = dma_row_pointer[0];	// source is line 't' of frame buffer or DMA indirection
	cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
	cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
	cur_tcd->SLAST = -first_line_in_sram_u * fb_row_stride;	// at end of major loop, move start address back to its initial position

	cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
	cur_tcd->CITER = first_line_in_sram_u;					// major loop should transfer all lines
	cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
	cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) | DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
	cur_tcd->BITER = cur_tcd->CITER;

	DPRINT("0 line@");
	DPRINT((int)fb_row_pointer[0], HEX);
	DPRINT(" ");
	dump_tcd(cur_tcd);

	cur_tcd++;

	dump(first_line_in_sram_u);

	// 2nd TCD copies SRAM_L buffer
	for(t = first_line_in_sram_u; t < img_h ; t++)
	{
		// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
		cur_tcd->SADDR = sram_l_dma_address; // source is line 't' of frame buffer or DMA indirection
		cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
		cur_tcd->CITER = 1;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD

		// the last line does not trigger copy of the next line because it does not exist
		if(t != (img_h - 1))
			cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) | DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
		else
			cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc);	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)

		cur_tcd->BITER = cur_tcd->CITER;

		DPRINT(t);
		DPRINT(" line@");
		DPRINT((int)fb_row_pointer[t], HEX);
		DPRINT(" ");
		dump_tcd(cur_tcd);

		cur_tcd++;
	}

	// Vblanking TCD configuration
	last_tcd = dma_append_vsync_tcds(cur_tcd);

	// 2) 2nd DMA has to relocate SRAM_U lines to SRAM_L buffer, build its TCDs
	// WARNING: 2nd DMA only has 1 TCD and it is stored directly in eDMA engine

	// disable 2nd DMA channel
	*sram_u_dmamux = 0;

	// and create 2nd DMA TCD
	sram_u_dma->SADDR = fb_row_pointer[first_line_in_sram_u];		// source is first line of frame buffer partially in SRAM_U
	sram_u_dma->SOFF = 16;														// after each read, move source address 16 bytes forward
	sram_u_dma->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// source data size = 16 bytes
	sram_u_dma->NBYTES = fb_row_stride;										// each minor loop transfers 1 framebuffer line
	sram_u_dma->SLAST = -nb_sram_u_fb_lines * fb_row_stride;					// at end of major loop, move start address back to its initial position

	sram_u_dma->DADDR = sram_l_dma_address;								// destination is the SRAM_L buffer
	sram_u_dma->DOFF = 16;														// after each write, move destination address 16 bytes forward
	sram_u_dma->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// write data size = 16 bytes
	sram_u_dma->CITER = nb_sram_u_fb_lines | DMA_TCD_CITER_ELINKYES_ELINK | DMA_TCD_CITER_ELINKYES_LINKCH(sram_u_dma_fix_num);	// after each minor loop (except last one), start the 3rd DMA channel to fix DADDR of this TCD
	sram_u_dma->DLASTSGA = -fb_row_stride;									// at end of major loop, let this TCD fixes itself instead of starting the 3rd DMA channel
	sram_u_dma->CSR = 0;
	sram_u_dma->BITER = sram_u_dma->CITER;

	DPRINTLN("DMA 2 TCD");
	dump_tcd((DMABaseClass::TCD_t*)sram_u_dma);

	// now, time to create TCD for 3rd DMA
	// it will reset DADDR of 2nd DMA TCD to sram_l_dma_addressw_pointer. The minor loop will copy 1 pointer at each trigger
	// and the major loop will end when all pointers were copied
	// WARNING: 3rd DMA only has 1 TCD and it is stored directly in eDMA engine

	// disable 3rd DMA channel
	*sram_u_dma_fixmux = 0;

	// and create 3nd DMA TCD
	sram_u_dma_fix->SADDR = &sram_l_dma_address;
	sram_u_dma_fix->SOFF = 0;
	sram_u_dma_fix->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
	sram_u_dma_fix->NBYTES = sizeof(uint8_t *);
	sram_u_dma_fix->SLAST = 0;

	sram_u_dma_fix->DADDR = &(sram_u_dma->DADDR);		// believe it (or not) but the 3rd DMA channel modifies DADDR register of the 2nd DMA channel between each minor loop :)
	sram_u_dma_fix->DOFF = 0;
	sram_u_dma_fix->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
	sram_u_dma_fix->CITER = 1;
	sram_u_dma_fix->DLASTSGA = 0;
	sram_u_dma_fix->CSR = 0;
	sram_u_dma_fix->BITER = sram_u_dma_fix->CITER;

	DPRINTLN("DMA 3 TCD");
	dump_tcd((DMABaseClass::TCD_t*)sram_u_dma_fix);

	// with this configuration, the DMA will permanently running between screen TCD and VBlanking TCD due to scatter/gather list looping itself
	// the number of bytes sent during 1 frame time is img_h * fb_row_stride + (scr_h - img_h) * sizeof(uint32_t)
	*px_dmamux = 0;								// disable DMA channel

	memcpy((void*)px_dma, px_dma_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA
	dump_tcd((DMABaseClass::TCD_t*)px_dma);

	return UVGA_OK;
}

// ============================================================================
// DMA configuration when multiple DMA channels are used and repeat_line = 2
uvga_error_t uVGA::rgb332_dma_init_dma_multiple_repeat_2()
{
	int t;
	bool sram_l_copy;
	int v;
	int nb_sram_u_fb_lines;


	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("rgb332_dma_init_dma_multiple_repeat_2");

	nb_sram_u_fb_lines = (img_h - first_line_in_sram_u) / 2;		// number of lines of frame buffer in SRAM_U

	// the number of major loop of the first DMA channel is... complex to compute.
	// ... or not :)
	// the frame requires M+N major loops + 3 major loops for VBlanking (1 before sync, 1 during sync and 1 after sync)
	// the image uses M+N major loops:
	// The first M major loops copy all lines located in SRAM_L. When the last major loop ends, it starts the 2nd DMA to copy 1 line from SRAM_U to SRAM_L buffer.
	// M = 1 + V, V = first_line_in_sram_u / 2 - 1.
	//  * The first major loop contains a minor loop copying the line 0
	//  * The V - 1 the next major loops contains 2 minors loop copying line V and V+1
	//  * The last major loop (V) contains 1 minor loop copying the line V and then start the 2nd DMA to copy 1 line from SRAM_U to SRAM_L buffer
	// With this settings, if first_line_in_sram_u = 6 then V = 2.
	// LOOP 0 => line 0
	// LOOP 1 (V=0)=> line 0, line 1
	// LOOP 2 (V=1)=> line 1, line 2
	// LOOP 3 (V=2)=> line 2
	v = first_line_in_sram_u / 2 - 1;
	px_dma_nb_major_loop = 1 + v + (img_h - first_line_in_sram_u) + 3;

	// In this case, 2nd and 3rd DMA channel have only 1 TCD, it is not necessary to allocated them in RAM
	// at least 1 line is in SRAM_U else we would be in this function
	sram_u_dma_nb_major_loop = 0;
	sram_u_dma_major_loop = NULL;
	sram_u_dma_fix_major_loop = NULL;

	// to reduce memory waste due to data alignment, both pixel, sram_u and sram_u_fix TCD are allocated simultaneously
	px_dma_major_loop = (DMABaseClass::TCD_t*)alloc_32B_align(sizeof(DMABaseClass::TCD_t) * (px_dma_nb_major_loop));

	if(px_dma_major_loop == NULL)
		return UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER;

	cur_tcd = px_dma_major_loop;

	// 1) build TCD to display lines and do Vsync

	// here, 1 TCD exists to copy the first line of frame buffer, 1 to copy the last in SRAM_U, and N existr1 TCD exists per line, minor loop displays 1 line. Then, scatter/gather mode switch to the next TCD
	// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
	// copy line 0
	cur_tcd->SADDR = dma_row_pointer[0];	// source is line 't' of frame buffer or DMA indirection
	cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
	cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
	cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

	cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
	cur_tcd->CITER = 1;					// major loop should transfer one line
	cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
	cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
	cur_tcd->BITER = cur_tcd->CITER;

	DPRINT("0 line@");
	DPRINT((int)fb_row_pointer[0], HEX);
	DPRINT(" ");
	dump_tcd(cur_tcd);

	cur_tcd++;

	// copy line V and V + 1
	for(t = 0; t < (v - 1) ; t++)
	{
		// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
		cur_tcd->SADDR = dma_row_pointer[2 * t];	// source is line 't' of frame buffer or DMA indirection. DMA row pointer is also duplicated thus multiplication by 2
		cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = - 2 * fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
		cur_tcd->CITER = 2;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
		cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
		cur_tcd->BITER = cur_tcd->CITER;

		DPRINT(2*t);
		DPRINT(" line@");
		DPRINT((int)fb_row_pointer[2*t], HEX);
		DPRINT(" ");
		dump_tcd(cur_tcd);

		cur_tcd++;
	}

	cur_tcd->SADDR = dma_row_pointer[v * 2];	// source is line 't' of frame buffer or DMA indirection
	cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
	cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
	cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

	cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
	cur_tcd->CITER = 1;					// major loop should transfer one line
	cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
	cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) | DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
	cur_tcd->BITER = cur_tcd->CITER;

	DPRINT(2*t);
	DPRINT(" line@");
	DPRINT((int)fb_row_pointer[2*v], HEX);
	DPRINT(" ");
	dump_tcd(cur_tcd);

	cur_tcd++;
	
	DPRINTLN("=== SRAM_U");

	// 2nd TCD copies SRAM_L buffer
	sram_l_copy = false;
	for(t = first_line_in_sram_u; t < img_h ; t++)
	{
		// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
		cur_tcd->SADDR = sram_l_dma_address; // source is line 't' of frame buffer or DMA indirection
		cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
		cur_tcd->CITER = 1;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD

		// the last line does not trigger copy of the next line because it does not exist
		// the copy only occurs every 2 frames buffer line because line are replicated
		if( (t != (img_h - 1)) && (sram_l_copy))
			cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) | DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
		else
			cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc);	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)

		sram_l_copy = !sram_l_copy;

		cur_tcd->BITER = cur_tcd->CITER;

		DPRINT(t);
		DPRINT(" line@");
		DPRINT((int)fb_row_pointer[t], HEX);
		DPRINT(" ");
		dump_tcd(cur_tcd);

		cur_tcd++;
	}

	// Vblanking TCD configuration
	last_tcd = dma_append_vsync_tcds(cur_tcd);

	// with this configuration, the DMA will permanently running between screen TCD and VBlanking TCD due to scatter/gather list looping itself
	// the number of bytes sent during 1 frame time is img_h * fb_row_stride + (scr_h - img_h) * sizeof(uint32_t)

	// disable 1st DMA channel
	*px_dmamux = 0;

	// load initial TCD in 1st DMA channel
	memcpy((void*)px_dma, px_dma_major_loop, sizeof(DMABaseClass::TCD_t));
	dump_tcd((DMABaseClass::TCD_t*)px_dma);


	// 2) 2nd DMA has to relocate SRAM_U lines to SRAM_L buffer, build its TCDs
	// WARNING: 2nd DMA only has 1 TCD and it is stored directly in eDMA engine

	// disable 2nd DMA channel
	*sram_u_dmamux = 0;

	// and create 2nd DMA TCD
	sram_u_dma->SADDR = fb_row_pointer[first_line_in_sram_u];		// source is first line of frame buffer partially in SRAM_U
	sram_u_dma->SOFF = 16;														// after each read, move source address 16 bytes forward
	sram_u_dma->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// source data size = 16 bytes
	sram_u_dma->NBYTES = fb_row_stride;										// each minor loop transfers 1 framebuffer line
	sram_u_dma->SLAST = -nb_sram_u_fb_lines * fb_row_stride;					// at end of major loop, move start address back to its initial position

	sram_u_dma->DADDR = sram_l_dma_address;								// destination is the SRAM_L buffer
	sram_u_dma->DOFF = 16;														// after each write, move destination address 16 bytes forward
	sram_u_dma->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// write data size = 16 bytes
	sram_u_dma->CITER = nb_sram_u_fb_lines | DMA_TCD_CITER_ELINKYES_ELINK | DMA_TCD_CITER_ELINKYES_LINKCH(sram_u_dma_fix_num);	// after each minor loop (except last one), start the 3rd DMA channel to fix DADDR of this TCD
	sram_u_dma->DLASTSGA = -fb_row_stride;									// at end of major loop, let this TCD fixes itself instead of starting the 3rd DMA channel
	sram_u_dma->CSR = 0;
	sram_u_dma->BITER = sram_u_dma->CITER;

	DPRINTLN("DMA 2 TCD");
	dump_tcd((DMABaseClass::TCD_t*)sram_u_dma);

	// now, time to create TCD for 3rd DMA
	// it will reset DADDR of 2nd DMA TCD to sram_l_dma_addressw_pointer. The minor loop will copy 1 pointer at each trigger
	// and the major loop will end when all pointers were copied
	// WARNING: 3rd DMA only has 1 TCD and it is stored directly in eDMA engine

	// disable 3rd DMA channel
	*sram_u_dma_fixmux = 0;

	// and create 3nd DMA TCD
	sram_u_dma_fix->SADDR = &sram_l_dma_address;
	sram_u_dma_fix->SOFF = 0;
	sram_u_dma_fix->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
	sram_u_dma_fix->NBYTES = sizeof(uint8_t *);
	sram_u_dma_fix->SLAST = 0;

	sram_u_dma_fix->DADDR = &(sram_u_dma->DADDR);		// believe it (or not) but the 3rd DMA channel modifies DADDR register of the 2nd DMA channel between each minor loop :)
	sram_u_dma_fix->DOFF = 0;
	sram_u_dma_fix->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
	sram_u_dma_fix->CITER = 1;
	sram_u_dma_fix->DLASTSGA = 0;
	sram_u_dma_fix->CSR = 0;
	sram_u_dma_fix->BITER = sram_u_dma_fix->CITER;

	DPRINTLN("DMA 3 TCD");
	dump_tcd((DMABaseClass::TCD_t*)sram_u_dma_fix);

	return UVGA_OK;
}

// ============================================================================
// DMA configuration when multiple DMA channels are used and repeat_line > 2
uvga_error_t uVGA::rgb332_dma_init_dma_multiple_repeat_more_than_2()
{
	int t;
	uint8_t **row_pointer;

	DMABaseClass::TCD_t *cur_tcd;

	// the number of major loop of the first DMA channel is:
	// 1 major loop per line + 3 major loop for VBlanking (1 before sync, 1 during sync and 1 after sync)
	// some modeline has no delay between end of image en begin of vsync. The library supports this and discard the first vblank TCD
	px_dma_nb_major_loop = img_h + 3;

	sram_u_dma_nb_major_loop = 0;

	if(fb_row_pointer[0] != dma_row_pointer[0])
		sram_u_dma_nb_major_loop++;

	// a line must be copied to SRAM_L before being display if its dma address is not in SRAM_L and it differs from the previous line 
	// (no need to copy multiple times the same line in SRAM_L buffer
	for(t = 0; t < img_h; t++)
	{
		if( (fb_row_pointer[t] != dma_row_pointer[t])
			&& (fb_row_pointer[t] != fb_row_pointer[t - 1])
			)
		{
			sram_u_dma_nb_major_loop++;
		}
	}

	// to reduce memory waste due to data alignment, both pixel, sram_u and sram_u_fix TCD are allocated simultaneously
	px_dma_major_loop = (DMABaseClass::TCD_t*)alloc_32B_align(sizeof(DMABaseClass::TCD_t) * (px_dma_nb_major_loop + sram_u_dma_nb_major_loop + 1));

	if(px_dma_major_loop == NULL)
		return UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER;

	sram_u_dma_major_loop = &px_dma_major_loop[px_dma_nb_major_loop];
	sram_u_dma_fix_major_loop = &px_dma_major_loop[px_dma_nb_major_loop + sram_u_dma_nb_major_loop];

	row_pointer = dma_row_pointer;

	cur_tcd = px_dma_major_loop;

	// 1) build TCD to display lines and do Vsync

	// here, 1 TCD exists per line, minor loop displays 1 line. Then, scatter/gather mode switch to the next TCD
	for(t = 0; t < img_h ; t++)
	{
		// line TCD configuration. each byte of the write buffer is written as a 32 bits value inside GPIO port D
		cur_tcd->SADDR = row_pointer[t];	// source is line 't' of frame buffer or DMA indirection
		cur_tcd->SOFF = 1;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// source data size = 1 byte
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = (volatile void*)&GPIOD_PDOR;		// destination is port D register. It is a 32 bits register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);				// write data size = 8 bits
		cur_tcd->CITER = 1;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
		cur_tcd->CSR = DMA_TCD_CSR_ESG | DMA_TCD_CSR_BWC(px_dma_bwc) ;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a hsync interrupt after image and before blanking time)
		cur_tcd->BITER = cur_tcd->CITER;

		cur_tcd++;
	}

	// Vblanking TCD configuration
	last_tcd = dma_append_vsync_tcds(cur_tcd);

	// 2) if a 2nd DMA has to rellocate SRAM_U lines to SRAM_L buffer, build its TCDs
	DMABaseClass::TCD_t *sram_u_dma_fix_tcd;
	DMABaseClass::TCD_t *sram_u_first_tcd = NULL;

	cur_tcd = NULL;
	int sram_u_tcd_num = 0;
	int nb_dma_fix = 0;

	// the first frame buffer line is processed after the last image line
	for(t = 1; t < img_h ; t++)
	{
		if( (fb_row_pointer[t] != dma_row_pointer[t])
			&& (fb_row_pointer[t] != fb_row_pointer[t - 1])
			)
		{
			cur_tcd = &sram_u_dma_major_loop[sram_u_tcd_num];

			if(sram_u_first_tcd == NULL)
				sram_u_first_tcd = cur_tcd;

			cur_tcd->SADDR = fb_row_pointer[t];	// source is line 't' of frame buffer
			cur_tcd->SOFF = 16;					// after each read, move source address 16 bytes forward
			cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// source data size = 16 bytes
			cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
			cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

			cur_tcd->DADDR = dma_row_pointer[t];	// destination is the 2 SRAM_L buffer
			cur_tcd->DOFF = 16;					// after each write, move destination address 16 bytes forward
			cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// write data size = 16 bytes
			cur_tcd->CITER = 1;					// major loop should transfer one line
			cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
			cur_tcd->CSR = DMA_TCD_CSR_ESG;	// enable scatter/gather mode
			cur_tcd->BITER = cur_tcd->CITER;

			// modify pixel DMA TCD from previous line to trigger a start of sram_u dma
			px_dma_major_loop[t - 1].CSR |= DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK;

			// here, the line frame is properly computed but... it does not work for the next frame
			// because it is not possible to have simultaneously scatter/gather mode enabled
			// and destination address correction, DLASTSGA is either next TCD or destination address correction

			// it can be fixed using the 3rd DMA channel to reset DADDR to its initial value :)
			// this 3rd DMA is trigger by the second one
			cur_tcd->CSR |= DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_fix_num) | DMA_TCD_CSR_MAJORELINK;
											
			sram_u_tcd_num++;
			nb_dma_fix++;
		}
	}

	// finally the first frame line
	if(fb_row_pointer[0] == dma_row_pointer[0])
	{
		if(sram_u_tcd_num < 1)
		{
			// this case should never occurs because it means the first frame buffer line is in SRAM_U but all other lines are in SRAM_L
			NPRINTLN("Error during TCD building. sram_u_tcd_num should never be smaller than 1 here. Stopping here");
			while(1);
		}

		cur_tcd = &sram_u_dma_major_loop[sram_u_tcd_num - 1];

		// if the first line is correct, correct the last sram_u TCD next TCD pointer to the first
		cur_tcd->DLASTSGA = (int32_t)sram_u_dma_major_loop;
	}
	else
	{
		cur_tcd = &sram_u_dma_major_loop[sram_u_tcd_num];

		cur_tcd->SADDR = fb_row_pointer[0];	// source is line 't' of frame buffer
		cur_tcd->SOFF = 16;					// after each read, move source address 16 byte forward
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// source data size = 16 bytes
		cur_tcd->NBYTES = fb_row_stride;	// each minor loop transfers 1 framebuffer line
		cur_tcd->SLAST = -fb_row_stride;	// at end of major loop, move start address back to its initial position

		cur_tcd->DADDR = dma_row_pointer[t];	// destination is one of the 2 SRAM_L buffer
		cur_tcd->DOFF = 16;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);				// write data size = 16 bytes
		cur_tcd->CITER = 1;					// major loop should transfer one line
		cur_tcd->DLASTSGA = (int32_t)sram_u_dma_major_loop;	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
		cur_tcd->CSR = DMA_TCD_CSR_ESG;	// enable scatter/gather mode
		cur_tcd->BITER = cur_tcd->CITER;

		// modify pixel DMA TCD from last line to trigger a start of sram_u dma
		px_dma_major_loop[img_h - 1].CSR |= DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_num) | DMA_TCD_CSR_MAJORELINK;

		cur_tcd->CSR |= DMA_TCD_CSR_MAJORLINKCH(sram_u_dma_fix_num) | DMA_TCD_CSR_MAJORELINK;

		nb_dma_fix++;
	}

	// now, time to create TCD for 3rd DMA
	// it will recopy dma_row_pointer to 2nd DMA TCD. The minor loop will copy 1 pointer at each trigger
	// and the major loop will end when all pointers were copied
	for(t = 1; t < img_h; t++)
	{
		if( (fb_row_pointer[t] != dma_row_pointer[t])
			&& (fb_row_pointer[t] != fb_row_pointer[t - 1])
			)
		{
			sram_u_dma_fix_tcd = &sram_u_dma_fix_major_loop[0];

			sram_u_dma_fix_tcd->SADDR = &dma_row_pointer[t];
			sram_u_dma_fix_tcd->SOFF = sizeof(uint32_t *) * complex_mode_ydiv;						// after each read go to the next pointer (4 bytes after * the repeat line factor)
			sram_u_dma_fix_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
			sram_u_dma_fix_tcd->NBYTES = sizeof(uint32_t *);
			sram_u_dma_fix_tcd->SLAST = -nb_dma_fix * sizeof(uint32_t *) * complex_mode_ydiv;

			sram_u_dma_fix_tcd->DADDR = &(sram_u_first_tcd->DADDR);		// after each write go to the DADDR pointer of the next TCD
			sram_u_dma_fix_tcd->DOFF = sizeof(DMABaseClass::TCD_t);
			sram_u_dma_fix_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
			sram_u_dma_fix_tcd->CITER = nb_dma_fix;
			sram_u_dma_fix_tcd->DLASTSGA = -nb_dma_fix * sizeof(DMABaseClass::TCD_t);
			sram_u_dma_fix_tcd->CSR = 0;
			sram_u_dma_fix_tcd->BITER = sram_u_dma_fix_tcd->CITER;
			break;
		}
	}

	// with this configuration, the DMA will permanently running between screen TCD and VBlanking TCD due to scatter/gather list looping itself
	// the number of bytes sent during 1 frame time is img_h * fb_row_stride + (scr_h - img_h) * sizeof(uint32_t)
	*px_dmamux = 0;								// disable DMA channel

	memcpy((void*)px_dma, px_dma_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA
	dump_tcd((DMABaseClass::TCD_t*)px_dma);

	*sram_u_dmamux = 0;																		// disable DMA channel
	memcpy((void*)sram_u_dma, sram_u_dma_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA

	*sram_u_dma_fixmux = 0;																		// disable DMA channel
	memcpy((void*)sram_u_dma_fix, sram_u_dma_fix_major_loop, sizeof(DMABaseClass::TCD_t));	// load initial TCD in DMA

	return UVGA_OK;
}

