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

// Objective:
// generates VGA signal fully in hardware without CPU help


// Principle:
// 1 FlexTimer (2 channels in complementary mode + 1 channel in PWM mode) && 1 (or 3) DMA channel

// FlexTimer uses 2 combined channels to create HSync signal at the correct time.
// on the same FlexTimer, a 3rd channel (X1) is used to start the dma at the correct time on each line.

// DMA generates both image and VSync signal and once started never stop.
// DMA uses a set of TCD (transfer control descriptor) linked together. Each describe a line to process.
// TCD is started by X1 FTM channel (the 3rd channel).
// On a normal line, DMA copies as fast as possible all pixels of the line + 1 (1 black pixel is added at the end of each line)
// After the image, 3 TCD are used to set VSYNC signal properly (1 before the vsync to wait it, 1 at the beginning of vsync to set it and 1 after the vsync to clear it)
// Vsync TCD uses nearly no ressource because they copy 4 bytes only on each line.

// It is not possible to fine tune DMA copy speed. Because it has the highest priority, it starts at the correct time (nearly every time :) )
// It tried to adjust speed of DMA using FTM and PIT but it slowed down far too much and is not accurate due to round error (PIT@60MHz to obtain a 45MHz signal gives a modulo of 1 with an error of 15Mhz).
// The 2 ways I found to slow down DMA is:
//  * the hard way: using DMA bandwith control to add wait states. Unfortunately, there is only 3 settings available: no wait state, 4 cycles wait state, 8 cycles wait state
//  * the extrem way: modifying CPU frequency
// Due to the fact VGA is an analog signal, the width of each pixel is no really "defined". Using a 800x600 resolution, I successfully packed more than 1000 pixels on each line

// To improve video stability, pixel DMA channel has the highest priority among other DMA channels
// moreover, the crossbar switch is configured to give DMA a maximal priority to SRAM backdoor and GPIO. Finally, to gain one more cycle, SRAM backdoor port and GPIO port are
// parked to DMA when they are not in used. This gives a huge boost in performance.

// Everything is not perfect. Due to the fact pixel duration is approximated, the monitor may or may not understand totally what it receives and with some colors, pixel may be blurry (VGA like :) )

// A last problem comes from SRAM. SRAM is splitted in 2, SRAM_L and SRAM_U. SRAM_L is accessed using CODE bus and has 0 wait state. SRAM_U is accessed using system bus and has at least 1 wait state.
// Unfortunately, the biggest part of the SRAM is SRAM_U.
// To fix this problem, a 2nd DMA channel is used. For all lines located in SRAM_U, a copy will be performed to bring them back in SRAM_L before displaying them. This channel will be started using
// TCD dma channel link of the previously displayed line
// However, this 2nd channel triggers a new problem. It is not possible for a TCD to modify destination address between TCD minor loop. This problem does not exist
// in the 1st channel because the destination address is always the same address. To bypass this problem, after a TCD minor loop of the 2nd channel is processed, it triggers a start on the 3rd channel.
// TCD of this 3rd channel will simply reset the value of destination address.

// All these copies waste a bit of RAM bandwidth but the 2nd dma copies are performed using burst mode and all these DMA TCD and DMA channel are automatically processed by the DMA engine without any CPU help

// Note: if all frame buffer lines are in SRAM_L, only the first DMA will be used

// Finally, these system works perfectly... as long as nothing disturb it.
// Hsync position is always correct because FTM channels cannot be bothered by anything.
// Vsync position is roughly correct. Even if DMA starts a bit late, due to signal duration, it does not seem to disturb monitor
// The main problem is pixel generation. If the DMA starts a bit late, minor line oscillation can be visible.
// Due to the high priority, DMA seems to always obtain access to SRAM before CPU. The only thing which seems to delay DMA channel start
// is... the DMA itself. If another DMA channel performing a "long" transfer, despite having a lower priority, it delays pixel DMA channel.
// Performing various data copies using CPU is OK. A simple Serial.print is not OK.

#define dump(v)		{DPRINT(#v ":"); DPRINTLN(v);}
#define dp(str,v)	{DPRINT(str ":"); DPRINTLN(v,HEX);}
#define dp_nonl(str,v)	{DPRINT(str ":"); DPRINT(v,HEX); DPRINT(" ");}

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
uVGA::uVGA(int dma_number, int sram_u_dma_number, int sram_u_dma_fix_number, int hsync_ftm_num, int hsync_ftm_channel_num, int x1_ftm_channel_num, int vsync_pin_num, int graphic_dma)
{
	// it is address of FTM0_SC, FTM1_SC, FTM2_SC, FTM3_SC
	static FTM_REGS_t *FTM_address[4] = {FTM0_ADDR, FTM1_ADDR, FTM2_ADDR,FTM3_ADDR};

	static volatile uint8_t *dma_chprio[DMA_NUM_CHANNELS] = {
																&DMA_DCHPRI0,  &DMA_DCHPRI1,  &DMA_DCHPRI2,  &DMA_DCHPRI3,
																&DMA_DCHPRI4,  &DMA_DCHPRI5,  &DMA_DCHPRI6,  &DMA_DCHPRI7,
																&DMA_DCHPRI8,  &DMA_DCHPRI9,  &DMA_DCHPRI10, &DMA_DCHPRI11,
																&DMA_DCHPRI12, &DMA_DCHPRI13, &DMA_DCHPRI14, &DMA_DCHPRI15
#if DMA_NUM_CHANNELS > 16
																,&DMA_DCHPRI16, &DMA_DCHPRI17, &DMA_DCHPRI18, &DMA_DCHPRI19,
																&DMA_DCHPRI20, &DMA_DCHPRI21, &DMA_DCHPRI22, &DMA_DCHPRI23,
																&DMA_DCHPRI24, &DMA_DCHPRI25, &DMA_DCHPRI26, &DMA_DCHPRI27,
																&DMA_DCHPRI28, &DMA_DCHPRI29, &DMA_DCHPRI30, &DMA_DCHPRI31
#endif
															};

	static int FTM_channel_to_dma_sources[4][8] = {
																	// FTM0
																	{
																		DMAMUX_SOURCE_FTM0_CH0, DMAMUX_SOURCE_FTM0_CH1,
																		DMAMUX_SOURCE_FTM0_CH2, DMAMUX_SOURCE_FTM0_CH3,
																		DMAMUX_SOURCE_FTM0_CH4, DMAMUX_SOURCE_FTM0_CH5,
																		DMAMUX_SOURCE_FTM0_CH6, DMAMUX_SOURCE_FTM0_CH7
																	},

																	// FTM1
																	{
																		DMAMUX_SOURCE_FTM1_CH0, DMAMUX_SOURCE_FTM1_CH1,
																		                     0,                      0,
																		                     0,                      0,
																		                     0,                      0
																	},

																	// FTM2
																	{
																		DMAMUX_SOURCE_FTM2_CH0, DMAMUX_SOURCE_FTM2_CH1,
																		                     0,                      0,
																		                     0,                      0,
																		                     0,                      0
																	},

																	// FTM3
																	{
#if FTM3_ADDR == NULL
																		                     0,                      0,
																		                     0,                      0,
																		                     0,                      0,
																		                     0,                      0
#else
																		DMAMUX_SOURCE_FTM3_CH0, DMAMUX_SOURCE_FTM3_CH1,
																		DMAMUX_SOURCE_FTM3_CH2, DMAMUX_SOURCE_FTM3_CH3,
																		DMAMUX_SOURCE_FTM3_CH4, DMAMUX_SOURCE_FTM3_CH5,
																		DMAMUX_SOURCE_FTM3_CH6, DMAMUX_SOURCE_FTM3_CH7
#endif
																	}
																};

	static int FTM_channel_to_gpio_pin[4][8] = {
														// FTM0
														{
															CORE_FTM0_CH0_PIN,	CORE_FTM0_CH1_PIN,
															CORE_FTM0_CH2_PIN,	CORE_FTM0_CH3_PIN,
															CORE_FTM0_CH4_PIN,	CORE_FTM0_CH5_PIN,
															CORE_FTM0_CH6_PIN,	CORE_FTM0_CH7_PIN
														},

														// FTM1
														{
															CORE_FTM1_CH0_PIN,	CORE_FTM1_CH1_PIN,
															                0,                   0,
															                0,                   0,
															                0,                   0
														},

														// FTM2
														{
															CORE_FTM2_CH0_PIN,	CORE_FTM2_CH1_PIN,
															                0,                   0,
															                0,                   0,
															                0,                   0
														},

														// FTM3
														{
#ifdef CORE_FTM3_CH0_PIN
															CORE_FTM3_CH0_PIN,	CORE_FTM3_CH1_PIN,
															CORE_FTM3_CH2_PIN,	CORE_FTM3_CH3_PIN,
															CORE_FTM3_CH4_PIN,	CORE_FTM3_CH5_PIN,
															CORE_FTM3_CH6_PIN,	CORE_FTM3_CH7_PIN
#else
															                0,                   0,
															                0,                   0,
															                0,                   0,
															                0,                   0
#endif
														}
													};

LED_INIT;
LED_ON;
	// select pixel DMA to use
	if((dma_number < 0) || (dma_number > 15) || (dma_number == graphic_dma))		// DMAx cannot be the same as gfx dma
		dma_number = 0;

	if((sram_u_dma_number < 0) || (sram_u_dma_number > 15) || (sram_u_dma_number == dma_number))
		sram_u_dma_number = 1;

	if((sram_u_dma_fix_number < 0) || (sram_u_dma_fix_number > 15) || (sram_u_dma_fix_number == sram_u_dma_number) || (sram_u_dma_fix_number == dma_number))
		sram_u_dma_fix_number = 2;

	dma_num = dma_number;
	sram_u_dma_num = sram_u_dma_number;
	sram_u_dma_fix_num = sram_u_dma_fix_number;

	// select Hsync FTM to use
	hsync_ftm = hsync_ftm_num;
	hsync_ftm_channel = hsync_ftm_channel_num;
	x1_ftm_channel = x1_ftm_channel_num;

	// hsync FTM from 0 to 3
	if((hsync_ftm < 0) || (hsync_ftm > 3))
		hsync_ftm = 0;

	// and only if it has an address on this CPU
	if( FTM_address[hsync_ftm] == NULL)
		hsync_ftm = 0;

	// hsync FTM channel must be even and from 0 to 7
	if( (hsync_ftm_channel & 1) || (hsync_ftm_channel < 0) || (hsync_ftm_channel > 7))
		hsync_ftm_channel = 0;

	// x1 FTM channel must be from 0 to 7 and neither hsync FTM channel nor hsync FTM channel +1
	if( (x1_ftm_channel < 0) || (x1_ftm_channel > 7) || (x1_ftm_channel == hsync_ftm_channel) || (x1_ftm_channel == (hsync_ftm_channel + 1)))
		x1_ftm_channel = 7;

	// x1 FTM channel must be a valid DMA request source
	if(FTM_channel_to_dma_sources[hsync_ftm][x1_ftm_channel] == 0)
		hsync_ftm = 0;

	hftm = FTM_address[hsync_ftm];

	// select Vsync FTM to use
	x1_ftm_channel = x1_ftm_channel_num;

	// share DMA settings
	edma = EDMA_ADDR;
	edma_TCD = (DMABaseClass::TCD_t*)&DMA_TCD0_SADDR;

	// select DMA to display image
	px_dma = &(edma_TCD[dma_num]);
	px_dmamux = (volatile uint8_t *)&(DMAMUX0_CHCFG0) + dma_num;
	px_dmaprio = dma_chprio[dma_num];

	px_dma_rq_src = FTM_channel_to_dma_sources[hsync_ftm][x1_ftm_channel];

	// select DMA to copy frame buffer line in SRAM_U to SRAM_L
	sram_u_dma = &(edma_TCD[sram_u_dma_num]);
	sram_u_dmamux = (volatile uint8_t *)&(DMAMUX0_CHCFG0) + sram_u_dma_num;
	sram_u_dmaprio = dma_chprio[sram_u_dma_num];

	sram_u_dma_fix = &(edma_TCD[sram_u_dma_fix_num]);
	sram_u_dma_fixmux = (volatile uint8_t *)&(DMAMUX0_CHCFG0) + sram_u_dma_fix_num;
	sram_u_dma_fixprio = dma_chprio[sram_u_dma_fix_num];

	// select DMA to perform video task (if possible)
	gfx_dma_num = graphic_dma;
	if( (gfx_dma_num > (DMA_NUM_CHANNELS - 1)) || (gfx_dma_num == dma_num) )
		gfx_dma_num = DMA_NUM_CHANNELS - 1;

	gfx_dma = &(edma_TCD[gfx_dma_num]);
	gfx_dmamux = (volatile uint8_t *)&(DMAMUX0_CHCFG0) + gfx_dma_num;
	gfx_dmaprio = dma_chprio[gfx_dma_num];

	// define pins sending *sync signal from calculed FTM and channel
	hsync_pin = FTM_channel_to_gpio_pin[hsync_ftm][hsync_ftm_channel];
	vsync_pin = vsync_pin_num;

	x1_pin = FTM_channel_to_gpio_pin[hsync_ftm][x1_ftm_channel];
}

// ============================================================================
// start video
uvga_error_t uVGA::begin(uVGAmodeline *modeline)
{
	static volatile uint32_t *pin_psor[] = {
															&CORE_PIN0_PORTSET, &CORE_PIN1_PORTSET, &CORE_PIN2_PORTSET, &CORE_PIN3_PORTSET,
															&CORE_PIN4_PORTSET, &CORE_PIN5_PORTSET, &CORE_PIN6_PORTSET, &CORE_PIN7_PORTSET,
															&CORE_PIN8_PORTSET, &CORE_PIN9_PORTSET, &CORE_PIN10_PORTSET, &CORE_PIN11_PORTSET,
															&CORE_PIN12_PORTSET, &CORE_PIN13_PORTSET, &CORE_PIN14_PORTSET, &CORE_PIN15_PORTSET,
															&CORE_PIN16_PORTSET, &CORE_PIN17_PORTSET, &CORE_PIN18_PORTSET, &CORE_PIN19_PORTSET,
															&CORE_PIN20_PORTSET, &CORE_PIN21_PORTSET, &CORE_PIN22_PORTSET, &CORE_PIN23_PORTSET,
															&CORE_PIN24_PORTSET, &CORE_PIN25_PORTSET, &CORE_PIN26_PORTSET, &CORE_PIN27_PORTSET,
															&CORE_PIN28_PORTSET, &CORE_PIN29_PORTSET, &CORE_PIN30_PORTSET, &CORE_PIN31_PORTSET,
															&CORE_PIN32_PORTSET, &CORE_PIN33_PORTSET
#if defined(CORE_PIN34_PORTSET)
															                                        , &CORE_PIN34_PORTSET, &CORE_PIN35_PORTSET,
															&CORE_PIN36_PORTSET, &CORE_PIN37_PORTSET, &CORE_PIN38_PORTSET, &CORE_PIN39_PORTSET,
															&CORE_PIN40_PORTSET, &CORE_PIN41_PORTSET, &CORE_PIN42_PORTSET, &CORE_PIN43_PORTSET,
															&CORE_PIN44_PORTSET, &CORE_PIN45_PORTSET, &CORE_PIN46_PORTSET, &CORE_PIN47_PORTSET,
															&CORE_PIN48_PORTSET, &CORE_PIN49_PORTSET, &CORE_PIN50_PORTSET, &CORE_PIN51_PORTSET,
															&CORE_PIN52_PORTSET, &CORE_PIN53_PORTSET, &CORE_PIN54_PORTSET, &CORE_PIN55_PORTSET,
															&CORE_PIN56_PORTSET, &CORE_PIN57_PORTSET, &CORE_PIN58_PORTSET, &CORE_PIN59_PORTSET,
															&CORE_PIN60_PORTSET, &CORE_PIN61_PORTSET, &CORE_PIN62_PORTSET, &CORE_PIN63_PORTSET
#endif
														};

	static volatile uint32_t *pin_pcor[] = {
															&CORE_PIN0_PORTCLEAR, &CORE_PIN1_PORTCLEAR, &CORE_PIN2_PORTCLEAR, &CORE_PIN3_PORTCLEAR,
															&CORE_PIN4_PORTCLEAR, &CORE_PIN5_PORTCLEAR, &CORE_PIN6_PORTCLEAR, &CORE_PIN7_PORTCLEAR,
															&CORE_PIN8_PORTCLEAR, &CORE_PIN9_PORTCLEAR, &CORE_PIN10_PORTCLEAR, &CORE_PIN11_PORTCLEAR,
															&CORE_PIN12_PORTCLEAR, &CORE_PIN13_PORTCLEAR, &CORE_PIN14_PORTCLEAR, &CORE_PIN15_PORTCLEAR,
															&CORE_PIN16_PORTCLEAR, &CORE_PIN17_PORTCLEAR, &CORE_PIN18_PORTCLEAR, &CORE_PIN19_PORTCLEAR,
															&CORE_PIN20_PORTCLEAR, &CORE_PIN21_PORTCLEAR, &CORE_PIN22_PORTCLEAR, &CORE_PIN23_PORTCLEAR,
															&CORE_PIN24_PORTCLEAR, &CORE_PIN25_PORTCLEAR, &CORE_PIN26_PORTCLEAR, &CORE_PIN27_PORTCLEAR,
															&CORE_PIN28_PORTCLEAR, &CORE_PIN29_PORTCLEAR, &CORE_PIN30_PORTCLEAR, &CORE_PIN31_PORTCLEAR,
															&CORE_PIN32_PORTCLEAR, &CORE_PIN33_PORTCLEAR
#ifdef CORE_PIN34_PORTCLEAR
															                                            , &CORE_PIN34_PORTCLEAR, &CORE_PIN35_PORTCLEAR,
															&CORE_PIN36_PORTCLEAR, &CORE_PIN37_PORTCLEAR, &CORE_PIN38_PORTCLEAR, &CORE_PIN39_PORTCLEAR,
															&CORE_PIN40_PORTCLEAR, &CORE_PIN41_PORTCLEAR, &CORE_PIN42_PORTCLEAR, &CORE_PIN43_PORTCLEAR,
															&CORE_PIN44_PORTCLEAR, &CORE_PIN45_PORTCLEAR, &CORE_PIN46_PORTCLEAR, &CORE_PIN47_PORTCLEAR,
															&CORE_PIN48_PORTCLEAR, &CORE_PIN49_PORTCLEAR, &CORE_PIN50_PORTCLEAR, &CORE_PIN51_PORTCLEAR,
															&CORE_PIN52_PORTCLEAR, &CORE_PIN53_PORTCLEAR, &CORE_PIN54_PORTCLEAR, &CORE_PIN55_PORTCLEAR,
															&CORE_PIN56_PORTCLEAR, &CORE_PIN57_PORTCLEAR, &CORE_PIN58_PORTCLEAR, &CORE_PIN59_PORTCLEAR,
															&CORE_PIN60_PORTCLEAR, &CORE_PIN61_PORTCLEAR, &CORE_PIN62_PORTCLEAR, &CORE_PIN63_PORTCLEAR
#endif
														};

	static uint32_t pin_bitmask[] = {
															CORE_PIN0_BITMASK, CORE_PIN1_BITMASK, CORE_PIN2_BITMASK, CORE_PIN3_BITMASK,
															CORE_PIN4_BITMASK, CORE_PIN5_BITMASK, CORE_PIN6_BITMASK, CORE_PIN7_BITMASK,
															CORE_PIN8_BITMASK, CORE_PIN9_BITMASK, CORE_PIN10_BITMASK, CORE_PIN11_BITMASK,
															CORE_PIN12_BITMASK, CORE_PIN13_BITMASK, CORE_PIN14_BITMASK, CORE_PIN15_BITMASK,
															CORE_PIN16_BITMASK, CORE_PIN17_BITMASK, CORE_PIN18_BITMASK, CORE_PIN19_BITMASK,
															CORE_PIN20_BITMASK, CORE_PIN21_BITMASK, CORE_PIN22_BITMASK, CORE_PIN23_BITMASK,
															CORE_PIN24_BITMASK, CORE_PIN25_BITMASK, CORE_PIN26_BITMASK, CORE_PIN27_BITMASK,
															CORE_PIN28_BITMASK, CORE_PIN29_BITMASK, CORE_PIN30_BITMASK, CORE_PIN31_BITMASK,
															CORE_PIN32_BITMASK, CORE_PIN33_BITMASK
#ifdef CORE_PIN34_BITMASK
															                                      , CORE_PIN34_BITMASK, CORE_PIN35_BITMASK,
															CORE_PIN36_BITMASK, CORE_PIN37_BITMASK, CORE_PIN38_BITMASK, CORE_PIN39_BITMASK,
															CORE_PIN40_BITMASK, CORE_PIN41_BITMASK, CORE_PIN42_BITMASK, CORE_PIN43_BITMASK,
															CORE_PIN44_BITMASK, CORE_PIN45_BITMASK, CORE_PIN46_BITMASK, CORE_PIN47_BITMASK,
															CORE_PIN48_BITMASK, CORE_PIN49_BITMASK, CORE_PIN50_BITMASK, CORE_PIN51_BITMASK,
															CORE_PIN52_BITMASK, CORE_PIN53_BITMASK, CORE_PIN54_BITMASK, CORE_PIN55_BITMASK,
															CORE_PIN56_BITMASK, CORE_PIN57_BITMASK, CORE_PIN58_BITMASK, CORE_PIN59_BITMASK,
															CORE_PIN60_BITMASK, CORE_PIN61_BITMASK, CORE_PIN62_BITMASK, CORE_PIN63_BITMASK
#endif
														};


	float exact_pxc_base_cnt;
	int y;
	uvga_error_t ret;

	if(modeline == NULL)
	{
		return UVGA_NO_VALID_VIDEO_MODE_FOUND;
	}

	// it is better to avoid approximate computation using modeline
	img_w = modeline->hres;
	img_h = modeline->vres;
	img_frame_rate = modeline->pixel_clock / modeline->htotal / modeline->vtotal;
	h_polarity = modeline->h_polarity;
	v_polarity = modeline->v_polarity;
	hpos_shift = modeline->horizontal_position_shift;

	// to find DMA BWC value, see Kinetis Reference Manual, 24.3.30 TCD Control and Status (DMA_TCDn_CSR)
	switch(modeline->pixel_h_stretch)
	{
		case UVGA_HSTRETCH_NORMAL:
											px_dma_bwc = 0;
											break;
   	case UVGA_HSTRETCH_WIDE:
											px_dma_bwc = 2;
											break;
   	case UVGA_HSTRETCH_ULTRA_WIDE:
											px_dma_bwc = 3;
											break;
	}

	dma_config_choice = modeline->dma_settings;

	vsync_bitmask = pin_bitmask[vsync_pin];
	if(v_polarity == UVGA_POSITIVE_POLARITY)
	{
		vsync_gpio_no_sync_level = pin_psor[vsync_pin];
		vsync_gpio_sync_level = pin_pcor[vsync_pin];
	}
	else
	{
		vsync_gpio_no_sync_level = pin_pcor[vsync_pin];
		vsync_gpio_sync_level = pin_psor[vsync_pin];
	}

	img_color_mode = modeline->img_color_mode;
	complex_mode_ydiv = modeline->repeat_line;

	scr_w = modeline->htotal;
	scr_h = modeline->vtotal;

	hsync_start_pix = modeline->hsync_start;
	hsync_end_pix = modeline->hsync_end;

	vsync_start_pix = modeline->vsync_start;
	vsync_end_pix = modeline->vsync_end;

	pxc_freq = modeline->pixel_clock;

	// FTM uses a fixed frequency
	pxc_base_freq = F_BUS;

	exact_pxc_base_cnt = (float)pxc_base_freq / (float)pxc_freq;

	// now, we need to compute value for FTM generating Hsync.
	// horizontal FTM is easy to compute... however modulo is a 16 bits number. 
	// using a prescaler reduce accuracy but also modulo
	hftm_base_freq = pxc_base_freq;
	for(hftm_prescaler = 1 ; hftm_prescaler < 256; hftm_prescaler <<= 1)
	{
		hftm_modulo = exact_pxc_base_cnt * scr_w / hftm_prescaler;

		// no compatible value between required frequency and modulo/prescaler combination
		if(hftm_modulo == 0)
			return UVGA_NO_VALID_VIDEO_MODE_FOUND;

		if(hftm_modulo < 65536)
			break;
	}

	if((hpos_shift <= 0) || (hpos_shift >= (hftm_modulo - 2)))
	{
		NPRINT("Invalid horizontal_position_shift. Assuming 1. Must be between 1 and ");
		NPRINTLN((hftm_modulo - 3));
		hpos_shift = 1;
	}

	// no compatible value between required frequency and modulo/prescaler combination
	if(hftm_modulo >= 65536)
		return UVGA_NO_VALID_VIDEO_MODE_FOUND;

	hftm_hsync_start = exact_pxc_base_cnt * hsync_start_pix / hftm_prescaler;
	hftm_hsync_end = exact_pxc_base_cnt * (hsync_end_pix - 1) / hftm_prescaler;

	fb_width = img_w;
	fb_row_stride = (fb_width + 1 + 15) & 0xFFF0;	// +1 to include a black pixel. then the result is rounded to the next multiple of 16 due to dma constraint

	switch(img_color_mode)
	{
		case UVGA_RGB332:
								// RGB 3:3:2 complex mode with possible line repeat
								fb_height = (img_h + complex_mode_ydiv - 1) / complex_mode_ydiv;

								// allocate all frame buffer rows + sram_u buffer as a single area, sram_u buffer at the beginning
								all_allocated_rows = (uint8_t*) malloc(fb_row_stride * (fb_height + 1) + 15);
								if(all_allocated_rows == NULL)
									return UVGA_FAIL_TO_ALLOCATE_FRAME_BUFFER;

								// round lines address to multiple of 16 bytes due to DMA burst constraint
								all_allocated_rows_aligned = (uint8_t *)(((int)all_allocated_rows + 15) & ~0xF);
								memset(all_allocated_rows_aligned, 0, fb_row_stride * (fb_height + 1));

								// allocate a 2 lines buffer in SRAM_L must be 16 bytes aligned due to DMA burst copy
								sram_l_dma_address = all_allocated_rows_aligned;

								if(((int)sram_l_dma_address) >= SRAM_U_START_ADDRESS)
									return UVGA_FAIL_TO_ALLOCATE_SRAM_L_BUFFER_IN_SRAM_L;

								// frame buffer has a reduced size
								frame_buffer = all_allocated_rows_aligned + fb_row_stride;

								if(((int)frame_buffer) >= SRAM_U_START_ADDRESS)
									return UVGA_FRAME_BUFFER_FIRST_LINE_NOT_IN_SRAM_L;

								// but not the frame buffer row pointer because it is used to display line
								fb_row_pointer = (uint8_t **) malloc(sizeof(uint8_t *) * img_h);
								if(fb_row_pointer == NULL)
									return UVGA_FAIL_TO_ALLOCATE_ROW_POINTER_ARRAY;

								sram_u_dma_required = false;

								switch(dma_config_choice)
								{
									case UVGA_DMA_AUTO:
																for(y = 0; y < img_h; y++)
																{
																	// prevent compiler to convert
																	//   floor(y / complex_mode_ydiv) * fb_row_stride
																	// into
																	//   floor(y * fb_row_stride / complex_mode_ydiv)
																	// using explicite parenthesis and cast.... just in case it has a stupid idea
																	// Note: yes, it is possible to write this faster but who cares, it is called only 1 time
																	fb_row_pointer[y] = frame_buffer + ((int)(y / complex_mode_ydiv)) * fb_row_stride;

																	// check if the last byte of the line is not in SRAM_U
																	if(((((int)fb_row_pointer[y]) + fb_width - 1) >= SRAM_U_START_ADDRESS) && (sram_u_dma_required == false))
																	{
																		DPRINT("y SRAM_U: ");
																		DPRINTLN(y);
																		sram_u_dma_required = true;
																		first_line_in_sram_u = y;
																	}
																}
																break;

									case UVGA_DMA_SINGLE:
																for(y = 0; y < img_h; y++)
																{
																	// prevent compiler to convert
																	//   floor(y / complex_mode_ydiv) * fb_row_stride
																	// into
																	//   floor(y * fb_row_stride / complex_mode_ydiv)
																	// using explicite parenthesis and cast.... just in case it has a stupid idea
																	// Note: yes, it is possible to write this faster but who cares, it is called only 1 time
																	fb_row_pointer[y] = frame_buffer + ((int)(y / complex_mode_ydiv)) * fb_row_stride;
																}
																break;
								}

								// if frame buffer lines are in SRAM_U, allocate a DMA redirection array
								if(sram_u_dma_required == true)
								{
									dma_row_pointer = (uint8_t **) malloc(sizeof(uint8_t *) * (img_h + complex_mode_ydiv));

									if(dma_row_pointer == NULL)
										return UVGA_FAIL_TO_ALLOCATE_DMA_ROW_POINTER_ARRAY;

									// row pointer of the line after the last line is the row pointer of the first line
									// (required to optimize TCD of the 3rd channel, instead of 1 per sram_l_dma_address change, only 1 globally)
									for(y = 0; y < img_h; y++)
									{
										dma_row_pointer[y] = fb_row_pointer[y];

										if(((int)dma_row_pointer[y] + fb_width - 1) >= SRAM_U_START_ADDRESS)
										{
											sram_u_dma_required = true;
											// 2 cases. If the previous line is the same as the current, reuse the same sram_l buffer
											// else use the other one
											if(fb_row_pointer[y] == fb_row_pointer[y - 1])
											{
												dma_row_pointer[y] = dma_row_pointer[y - 1];
											}
											else
											{
												dma_row_pointer[y] = sram_l_dma_address;
											}
										}
									}

									while(y < (img_h + complex_mode_ydiv))
									{
										dma_row_pointer[y++] = dma_row_pointer[0];
									}
								}
								else
								{
									sram_u_dma_required = false;
									dma_row_pointer = NULL;

									for(y = 0; y < img_h; y++)
									{
										DPRINT(y);
										DPRINT(":");
										DPRINTLN((int)fb_row_pointer[y], HEX);
									}
								}
								break;

		default:
								return UVGA_UNKNOWN_COLOR_MODE;
								break;
	}

	if(frame_buffer == NULL)
		return UVGA_FAIL_TO_ALLOCATE_FRAME_BUFFER;

	// not possible to initialize this earlier
	init_text_settings();

	clocks_init();

	if((ret = dma_init()) != UVGA_OK)
		return ret;

	clocks_start();

	return UVGA_OK;
}

// ============================================================================
// configure all clocks but keep them stopped
void uVGA::clocks_init()
{
	int channel_shift;

	SIM_SCGC6 |= SIM_SCGC6_FTM0 | SIM_SCGC6_FTM1;
	SIM_SCGC3 |= SIM_SCGC3_FTM2 | SIM_SCGC3_FTM3;

	uVGA::stop();

	switch(img_color_mode)
	{
		default:
					// Asymmetric PWM requires FTMEN=1 (MODE), QUADEN=0 (QDCTRL), DECAPEN=0 (COMBINE), COMBINE=1 (COMBINE), CPWMS=0 (SC), COMP=1 (COMBINE)
					// ELSB & A is 01 or 10 depending on the required polarity
					// MSB & A is 10. According to FlexTimerModule description of FTMx_CnSC, it is unused but AN5142 sets it to 10 and without it, it does not work

					// 2) hsync FTM settings
					hftm->SC = 0;			// before doing anything else
					hftm->CNT = 0;
					hftm->CNTIN = 0;

					hftm->MODE &= ~FTM_MODE_FTMEN;

					// set modulo frequency
					//hftm->MOD = hftm_modulo + 1;
					hftm->MOD = hftm_modulo;

					// set combine mode for hsync => asymmetric PWM
					if(h_polarity == UVGA_POSITIVE_POLARITY)
						hftm->C[hsync_ftm_channel].SC = 0b00100100;					// (start low, impulse is high)
					else
						hftm->C[hsync_ftm_channel].SC = 0b00101000;					// (start high, impulse is low)

					hftm->C[hsync_ftm_channel].V = hftm_hsync_start;		// hsync start on channel x
					hftm->C[hsync_ftm_channel+1].V = hftm_hsync_end;		// hsync end on channel x+1

					//hftm->CONF |= FTM_CONF_GTBEEN;								// perform a synchronized start with other FTM

					channel_shift = (hsync_ftm_channel >> 1) << 3;			// combine bits is at position (channel pair number (=channel number /2) * 8)
					hftm->COMBINE = ((hftm->COMBINE & ~(0x000000FF << channel_shift))
											| ((FTM_COMBINE_COMBINE0 | FTM_COMBINE_COMP0) << channel_shift));

					// set pwm mode for x1 (same FTM as hsync thus must be set before SC
					hftm->C[x1_ftm_channel].SC = 0b00100100 | FTM_CSC_DMA | FTM_CSC_CHIE;					// (start high until value then low), generate signal for DMAMUX (CHIE is required to have request routed to DMAMUX (see Reference Manual 43.5.14 DMA)
					hftm->C[x1_ftm_channel].V = hpos_shift;		// increase this value to move the screen to the right

					hftm->SC = FTM_SC_CLKS(1) | FTM_SC_PS(FTM_prescaler_to_selection(hftm_prescaler));
					// here, FTM is not started but read

					// enable all clock and signal pins
					pinMode(hsync_pin, OUTPUT);
					digitalWrite(hsync_pin, 0);
					set_pin_alternate_function_to_FTM(hsync_pin);

					pinMode(vsync_pin, OUTPUT);
					*vsync_gpio_no_sync_level = vsync_bitmask;

					// vga pin rgb pin: 2,14,7,8,6,20,21,5 (bit0-bit7 port D)
					GPIOD_PCOR = 0xFF;	// set all pins to LOW
					pinMode(2, OUTPUT);
					pinMode(14, OUTPUT);
					pinMode(7, OUTPUT);
					pinMode(8, OUTPUT);
					pinMode(6, OUTPUT);
					pinMode(20, OUTPUT);
					pinMode(21, OUTPUT);
					pinMode(5, OUTPUT);
					break;
	}
}

// ============================================================================
void uVGA::set_pin_alternate_function_to_FTM(int pin_num)
{
	static struct
	{
		volatile uint32_t *pin_config;
		int alt_func_mask;
	} config[] = {
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		// from K66 sub-family reference manual, K66 Signal Multiplexing and pin Assignments
											{&CORE_PIN0_CONFIG, 0x0000},		// PORTB_PCR16		x
											{&CORE_PIN1_CONFIG, 0x0000},		// PORTB_PCR17		x
											{&CORE_PIN2_CONFIG, 0x0400},		// PORTD_PCR0		FTM3_CH0
											{&CORE_PIN3_CONFIG, 0x0300},		// PORTA_PCR12		FTM1_CH0
											{&CORE_PIN4_CONFIG, 0x0300},		// PORTA_PCR13		FTM1_CH1
											{&CORE_PIN5_CONFIG, 0x0400},		// PORTD_PCR7		FTM0_CH7
											{&CORE_PIN6_CONFIG, 0x0400},		// PORTD_PCR4		FTM0_CH4
											{&CORE_PIN7_CONFIG, 0x0400},		// PORTD_PCR2		FTM3_CH2
											{&CORE_PIN8_CONFIG, 0x0400},		// PORTD_PCR3		FTM3_CH3
											{&CORE_PIN9_CONFIG, 0x0400},		// PORTC_PCR3		FTM0_CH2
											{&CORE_PIN10_CONFIG, 0x0400},		// PORTC_PCR4		FTM0_CH3
											{&CORE_PIN11_CONFIG, 0x0000},		// PORTC_PCR6		x
											{&CORE_PIN12_CONFIG, 0x0000},		// PORTC_PCR7		x
											{&CORE_PIN13_CONFIG, 0x0700},		// PORTC_PCR5		FTM0_CH2
											{&CORE_PIN14_CONFIG, 0x0400},		// PORTD_PCR1		FTM3_CH1
											{&CORE_PIN15_CONFIG, 0x0000},		// PORTC_PCR0		x
											{&CORE_PIN16_CONFIG, 0x0300},		// PORTB_PCR0		FTM1_CH0
											{&CORE_PIN17_CONFIG, 0x0300},		// PORTB_PCR1		FTM1_CH1
											{&CORE_PIN18_CONFIG, 0x0000},		// PORTB_PCR3		x
											{&CORE_PIN19_CONFIG, 0x0000},		// PORTB_PCR2		x
											{&CORE_PIN20_CONFIG, 0x0400},		// PORTD_PCR5		FTM0_CH5
											{&CORE_PIN21_CONFIG, 0x0400},		// PORTD_PCR6		FTM0_CH6
											{&CORE_PIN22_CONFIG, 0x0400},		// PORTC_PCR1		FTM0_CH0
											{&CORE_PIN23_CONFIG, 0x0400},		// PORTC_PCR2		FTM0_CH1
											{&CORE_PIN24_CONFIG, 0x0000},		// PORTE_PCR26
											{&CORE_PIN25_CONFIG, 0x0300},		// PORTA_PCR5		FTM0_CH2
											{&CORE_PIN26_CONFIG, 0x0000},		// PORTA_PCR14		x
											{&CORE_PIN27_CONFIG, 0x0000},		// PORTA_PCR15		x
											{&CORE_PIN28_CONFIG, 0x0000},		// PORTA_PCR16		x
											{&CORE_PIN29_CONFIG, 0x0300},		// PORTB_PCR18		FTM2_CH0
											{&CORE_PIN30_CONFIG, 0x0300},		// PORTB_PCR19		FTM2_CH1
											{&CORE_PIN31_CONFIG, 0x0000},		// PORTB_PCR10		x
											{&CORE_PIN32_CONFIG, 0x0000},		// PORTB_PCR11		x
											{&CORE_PIN33_CONFIG, 0x0000},		// PORTE_PCR24
											{&CORE_PIN34_CONFIG, 0x0000},		// PORTE_PCR25
											{&CORE_PIN35_CONFIG, 0x0300},		// PORTC_PCR8		FTM3_CH4
											{&CORE_PIN36_CONFIG, 0x0300},		// PORTC_PCR9		FTM3_CH5
											{&CORE_PIN37_CONFIG, 0x0300},		// PORTC_PCR10		FTM3_CH6
											{&CORE_PIN38_CONFIG, 0x0300},		// PORTC_PCR11		FTM3_CH7
											{&CORE_PIN39_CONFIG, 0x0000},		// PORTA_PCR17		x
											{&CORE_PIN40_CONFIG, 0x0000},		// PORTA_PCR28		x
											{&CORE_PIN41_CONFIG, 0x0000},		// PORTA_PCR29		x
											{&CORE_PIN42_CONFIG, 0x0000},		// PORTA_PCR26		x
											{&CORE_PIN43_CONFIG, 0x0000},		// PORTB_PCR20		x
											{&CORE_PIN44_CONFIG, 0x0000},		// PORTB_PCR22		x
											{&CORE_PIN45_CONFIG, 0x0000},		// PORTB_PCR23		x
											{&CORE_PIN46_CONFIG, 0x0000},		// PORTB_PCR21		x
											{&CORE_PIN47_CONFIG, 0x0000},		// PORTD_PCR8		x
											{&CORE_PIN48_CONFIG, 0x0000},		// PORTD_PCR9		x
											{&CORE_PIN49_CONFIG, 0x0000},		// PORTB_PCR4		x
											{&CORE_PIN50_CONFIG, 0x0000},		// PORTB_PCR5		x
											{&CORE_PIN51_CONFIG, 0x0000},		// PORTD_PCR14		x
											{&CORE_PIN52_CONFIG, 0x0000},		// PORTD_PCR13		x
											{&CORE_PIN53_CONFIG, 0x0000},		// PORTD_PCR12		x
											{&CORE_PIN54_CONFIG, 0x0000},		// PORTD_PCR15		x
											{&CORE_PIN55_CONFIG, 0x0000},		// PORTD_PCR11		x
											{&CORE_PIN56_CONFIG, 0x0000},		// PORTE_PCR10
											{&CORE_PIN57_CONFIG, 0x0000},		// PORTE_PCR11
											{&CORE_PIN58_CONFIG, 0x0000},		// PORTE_PCR0
											{&CORE_PIN59_CONFIG, 0x0000},		// PORTE_PCR1
											{&CORE_PIN60_CONFIG, 0x0000},		// PORTE_PCR2
											{&CORE_PIN61_CONFIG, 0x0000},		// PORTE_PCR3
											{&CORE_PIN62_CONFIG, 0x0000},		// PORTE_PCR4
											{&CORE_PIN63_CONFIG, 0x0000}		// PORTE_PCR5
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
		// from K20 sub-family reference manual, K20 Signal Multiplexing and pin Assignments
											{&CORE_PIN0_CONFIG, 0x0000},		// PORTB_PCR16		x
											{&CORE_PIN1_CONFIG, 0x0000},		// PORTB_PCR17		x
											{&CORE_PIN2_CONFIG, 0x0000},		// PORTD_PCR0		x
											{&CORE_PIN3_CONFIG, 0x0300},		// PORTA_PCR12		FTM1_CH0
											{&CORE_PIN4_CONFIG, 0x0300},		// PORTA_PCR13		FTM1_CH1
											{&CORE_PIN5_CONFIG, 0x0400},		// PORTD_PCR7		FTM0_CH7
											{&CORE_PIN6_CONFIG, 0x0400},		// PORTD_PCR4		FTM0_CH4
											{&CORE_PIN7_CONFIG, 0x0000},		// PORTD_PCR2		x
											{&CORE_PIN8_CONFIG, 0x0000},		// PORTD_PCR3		x
											{&CORE_PIN9_CONFIG, 0x0400},		// PORTC_PCR3		FTM0_CH2
											{&CORE_PIN10_CONFIG, 0x0400},		// PORTC_PCR4		FTM0_CH3
											{&CORE_PIN11_CONFIG, 0x0000},		// PORTC_PCR6		x
											{&CORE_PIN12_CONFIG, 0x0000},		// PORTC_PCR7		x
											{&CORE_PIN13_CONFIG, 0x0000},		// PORTC_PCR5		x
											{&CORE_PIN14_CONFIG, 0x0000},		// PORTD_PCR1		x
											{&CORE_PIN15_CONFIG, 0x0000},		// PORTC_PCR0		x
											{&CORE_PIN16_CONFIG, 0x0300},		// PORTB_PCR0		FTM1_CH0
											{&CORE_PIN17_CONFIG, 0x0300},		// PORTB_PCR1		FTM1_CH1
											{&CORE_PIN18_CONFIG, 0x0000},		// PORTB_PCR3		x
											{&CORE_PIN19_CONFIG, 0x0000},		// PORTB_PCR2		x
											{&CORE_PIN20_CONFIG, 0x0400},		// PORTD_PCR5		FTM0_CH5
											{&CORE_PIN21_CONFIG, 0x0400},		// PORTD_PCR6		FTM0_CH6
											{&CORE_PIN22_CONFIG, 0x0400},		// PORTC_PCR1		FTM0_CH0
											{&CORE_PIN23_CONFIG, 0x0400},		// PORTC_PCR2		FTM0_CH1
											{&CORE_PIN24_CONFIG, 0x0300},		// PORTA_PCR5     FTM0_CH2
											{&CORE_PIN25_CONFIG, 0x0300},		// PORTB_PCR19		FTM2_CH1
											{&CORE_PIN26_CONFIG, 0x0000},		// PORTE_PCR1		x
											{&CORE_PIN27_CONFIG, 0x0000},		// PORTC_PCR9		x
											{&CORE_PIN28_CONFIG, 0x0000},		// PORTC_PCR8		x
											{&CORE_PIN29_CONFIG, 0x0000},		// PORTC_PCR10		x
											{&CORE_PIN30_CONFIG, 0x0000},		// PORTC_PCR11		x
											{&CORE_PIN31_CONFIG, 0x0000},		// PORTE_PCR0		x
											{&CORE_PIN32_CONFIG, 0x0300},		// PORTB_PCR18		FTM2_CH0
											{&CORE_PIN33_CONFIG, 0x0300}		// PORTA_PCR4     FTM0_CH1
#endif
										};

	if(config[pin_num].alt_func_mask == 0)
	{
		NPRINT(pin_num);
		NPRINT(": no mask for FTM");
		return;
	}

	*(config[pin_num].pin_config) = ((*(config[pin_num].pin_config)) & 0xF800) | config[pin_num].alt_func_mask;
}

// ============================================================================
// allocate a bloc aligned on a 32 bytes address
uint8_t *uVGA::alloc_32B_align(int size)
{
	uint8_t *t;
	int base_align;

	int rsize = size + 31;

	t = (uint8_t *)malloc(rsize);
	if(t == NULL)
		return NULL;

	// current alignment
	base_align = ((unsigned int)t) & 0x1F;

	// already aligned ?
	if(base_align == 0)
		return t;

	// not aligned => move address to the next aligned address
	return t + (32 - base_align);
}

// ============================================================================
// append TCD generating VSYNC 
// input: first empty TCD entry to use
// output: new first empty TCD entry
DMABaseClass::TCD_t *uVGA::dma_append_vsync_tcds(DMABaseClass::TCD_t *cur_tcd)
{
	// address of the 2nd TCD of VSYNC. We don't use the first but the 2nd because waitVsync() will check DLASTSGA to identify TCD
	dma_sync_tcd_address = (int)(cur_tcd + 1);

	// Vblanking TCD configuration
	// after frame buffer and before sync
	if((vsync_start_pix - img_h) > 0)
	{
		cur_tcd->SADDR = &vsync_bitmask;	// source is the bitmask of the pin driving vsync signal
		cur_tcd->SOFF = 0;					// never change source adress, the pixel does not move :)
		cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// source data size = 32 bits
		cur_tcd->NBYTES = 4;					// each minor loop transfers 4 bytes
		cur_tcd->SLAST = 0;					// at end of major loop, don't change source address

		cur_tcd->DADDR = vsync_gpio_no_sync_level;		// destination is vsync port register
		cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
		cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// write data size = 8 bits
		cur_tcd->CITER = vsync_start_pix - img_h;			// repeat from end of framebuffer to start of vsync
		cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
		cur_tcd->CSR = DMA_TCD_CSR_ESG;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a vsync interrupt before image and after blanking time)
		cur_tcd->BITER = cur_tcd->CITER;
		cur_tcd++;
	}

	// during sync
	cur_tcd->SADDR = &vsync_bitmask;	// source is the bitmask of the pin driving vsync signal
	cur_tcd->SOFF = 0;					// never change source adress, the pixel does not move :)
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// source data size = 32 bits
	cur_tcd->NBYTES = 4;					// each minor loop transfers 4 bytes
	cur_tcd->SLAST = 0;					// at end of major loop, don't change source address

	cur_tcd->DADDR = vsync_gpio_sync_level;		// destination is vsync port register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// write data size = 8 bits
	cur_tcd->CITER = vsync_end_pix - vsync_start_pix;			// repeat from start of vsync to end of vsync
	cur_tcd->DLASTSGA = (int32_t)(cur_tcd+1);	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD
	cur_tcd->CSR = DMA_TCD_CSR_ESG;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a vsync interrupt before image and after blanking time)
	cur_tcd->BITER = cur_tcd->CITER;
	cur_tcd++;

	// and after sync
	cur_tcd->SADDR = &vsync_bitmask;	// source is the bitmask of the pin driving vsync signal
	cur_tcd->SOFF = 0;					// never change source adress, the pixel does not move :)
	cur_tcd->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// source data size = 32 bits
	cur_tcd->NBYTES = 4;					// each minor loop transfers 4 bytes
	cur_tcd->SLAST = 0;					// at end of major loop, don't change source address

	cur_tcd->DADDR = vsync_gpio_no_sync_level;		// destination is vsync port register
	cur_tcd->DOFF = 0;					// never change write destination, the register does not move :)
	cur_tcd->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);				// write data size = 8 bits
	cur_tcd->CITER = scr_h - vsync_end_pix;			// repeat from end of sync to end of screen
	cur_tcd->DLASTSGA = (uint32_t)px_dma_major_loop;	// scatter/gather mode enabled. At end of major loop of this TCD, switch to the next TCD... the first one
	cur_tcd->CSR = DMA_TCD_CSR_ESG;	// enable scatter/gather mode (add  "| DMA_TCD_CSR_INTMAJOR" have a vsync interrupt before image and after blanking time)
	cur_tcd->BITER = cur_tcd->CITER;
	cur_tcd++;

	return cur_tcd;
}

// ============================================================================
// DMA configuration when only 1 DMA channel is used
// optimized & OK
uvga_error_t uVGA::dma_init_dma_single_repeat_1()
{
	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("dma_init_dma_single_repeat_1");

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
// DMA configuration when only multiple DMA channels are used and repeat_line > 2
// optimized & OK
uvga_error_t uVGA::dma_init_dma_single_repeat_more_than_1()
{
	int t;
	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("dma_init_dma_single_repeat_more_than_1");

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
// DMA configuration when only multiple DMA channels are used and repeat_line = 1
// optimized & OK
uvga_error_t uVGA::dma_init_dma_multiple_repeat_1()
{
	int t;
	DMABaseClass::TCD_t *cur_tcd;
	int nb_sram_u_fb_lines;

	DPRINTLN("dma_init_dma_multiple_repeat_1");

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
// DMA configuration when only multiple DMA channels are used and repeat_line = 2
uvga_error_t uVGA::dma_init_dma_multiple_repeat_2()
{
	int t;
	bool sram_l_copy;
	int v;
	int nb_sram_u_fb_lines;


	DMABaseClass::TCD_t *cur_tcd;

	DPRINTLN("dma_init_dma_multiple_repeat_2");

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
// DMA configuration when only multiple DMA channels are used and repeat_line > 2
uvga_error_t uVGA::dma_init_dma_multiple_repeat_more_than_2()
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

// ============================================================================
// configure dma and enable it
uvga_error_t uVGA::dma_init()
{
	uvga_error_t ret = UVGA_UNKNOWN_ERROR;

	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;			// enable clock on DMA Mux module from SIM
	SIM_SCGC7 |= SIM_SCGC7_DMA;				// enable clock on DMA module from SIM

	switch(img_color_mode)
	{
		case UVGA_RGB332:
								if(!sram_u_dma_required)
								{
									switch(complex_mode_ydiv)
									{
										case 1:
													ret = dma_init_dma_single_repeat_1();
													break;

										default:
													ret = dma_init_dma_single_repeat_more_than_1();
													break;
									}
								}
								else
								{
									switch(complex_mode_ydiv)
									{
										case 1:
													ret = dma_init_dma_multiple_repeat_1();
													break;

										case 2:
													ret = dma_init_dma_multiple_repeat_2();
													break;

										default:
													ret = dma_init_dma_multiple_repeat_more_than_2();
													break;

									}
								}
								break;
	}

	if(ret != UVGA_OK)
		return ret;

	// px_dma channel must be configure to have the highest possible priority
	DMA_DCHPRI15 = DMA_DCHPRI_CHPRI(dma_num);
	*px_dmaprio = DMA_DCHPRI_CHPRI(15);		// give absolute priority for this DMA channel and disable preemption while running

	// let's also give the highest priority from DMA to RAM and GPIO in crossbar switch
	// master 2 = DMA
	// slave 1 = sram backdoor
	// slave 3 = GPIO
	// when only the CPU requests access, this has nearly no effects
	// Kinetis Reference manual says 6 is the highest priority but AXBS_PRSn register description says 0 is the highest
	// I assumed 6 is correct but wathever I chose, I see no difference during my test
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	AXBS_PRS1 = 0x05432610;	//0x06543021;
	AXBS_PRS3 = 0x05432610;	//0x06543021;
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
	AXBS_PRS1 = 0x00002310;	//0x00003021;
	AXBS_PRS3 = 0x00002310;	//0x00003021;
#endif

	// to gain 1 more cycle, when RAM and GPIO port is not used, attach them to DMA and for a fixed priority
	// with this tip, DMA gains a lot of time... really a lot due to fact the frame buffer must be copied byte by byte.
	// This type of copy waste a lot of bandwidth/time, so much it is not possible to trigger copy using a timer to obtain a more accurate pixel
	AXBS_CRS1 = AXBS_CRS_ARB_FIXED | AXBS_CRS_PARK_FIXED | AXBS_CRS_PARK(2);
	AXBS_CRS3 = AXBS_CRS_ARB_FIXED | AXBS_CRS_PARK_FIXED | AXBS_CRS_PARK(2);

	// when DMA uses RAM, it cannot be stopped... but it should already be the default settings.
	AXBS_MGPCR2 = 0x00000000;

	// all other masters must wait, even during undefined length burst
	AXBS_MGPCR0 = 0x00000001;
	AXBS_MGPCR1 = 0x00000001;
	AXBS_MGPCR3 = 0x00000001;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	AXBS_MGPCR4 = 0x00000001;
	AXBS_MGPCR5 = 0x00000001;
	AXBS_MGPCR6 = 0x00000001;
#endif

	// give absolute priority to DMA on SRAM_L and favor DMA on SRAM_U
	// this final settings greatly improve pixel sharpness and line stability under heavy graphic load
	MCM_CR = (MCM_CR & ~(MCM_CR_SRAMLAP(3) | MCM_CR_SRAMUAP(3)))
					| MCM_CR_SRAMLAP(3) | MCM_CR_SRAMUAP(1);
	
	// DMA trigger is Hsync FTM channel
	*px_dmamux = DMAMUX_ENABLE | px_dma_rq_src;

	edma->SERQ = dma_num;

	// enable MUX for gfx DMA channel
	// gfx DMA can be suspended any time. Without this, it disturb image generation
	*gfx_dmaprio = *gfx_dmaprio | DMA_DCHPRI_ECP;
	*gfx_dmamux = DMAMUX_ENABLE | DMAMUX_SOURCE_ALWAYS0;

	// here, dma is enabled and read to start as soon as X1 ftm channel goes up
	return UVGA_OK;
}

// ============================================================================
#if 0
void uVGA::init_gfx_dma()
{
	gfx_dma_draw_tcd.SADDR = gfx_dma_color;
	gfx_dma_draw_tcd.SOFF = 0;
	gfx_dma_draw_tcd.ATTR = ;
	//gfx_dma_draw_tcd.NBYTES = ;
	gfx_dma_draw_tcd.SLAST = 0;
	//gfx_dma_draw_tcd.DADDR = ;
	//gfx_dma_draw_tcd.DOFF = ;
	//gfx_dma_draw_tcd.CITER = ;
	gfx_dma_draw_tcd.DLASTSGA = 0
	gfx_dma_draw_tcd.CSR = DMA_TCD_CSR_DREQ;
	//gfx_dma_draw_tcd.BITER = ;

	
	edma->SERQ = gfx_dma_num;
}
#endif

// ============================================================================
void uVGA::dump_tcd(DMABaseClass::TCD_t *tcd)
{
	dp_nonl("SADDR", (int)tcd->SADDR);
	dp_nonl("SOFF", tcd->SOFF);
	dp_nonl("ATTR", tcd->ATTR);
	dp_nonl("NBYTES", tcd->NBYTES);
	dp_nonl("SLAST", tcd->SLAST);
	dp_nonl("DADDR", (int)tcd->DADDR);
	dp_nonl("DOFF", tcd->DOFF);
	dp_nonl("CITER", tcd->CITER);
	dp_nonl("DLASTSGA", tcd->DLASTSGA);
	dp_nonl("CSR", tcd->CSR);
	dp_nonl("BITER", tcd->BITER);
	DPRINTLN("");
}

// ============================================================================
// start clock and image production
void uVGA::clocks_start()
{
	int loop_delay= - 1;
	int frame_num;

	DPRINTLN("start clock");

	hftm->MODE |= FTM_MODE_FTMEN; 	// start hftm.

	// after starting the clock, check if the dma is not too slow to display the first 10 frames
	for(frame_num = 0; frame_num < 10; frame_num ++)
	{
		int start_micros = micros();
		int last_line = -1;
		int cur_num;
		int lp_delay;

		// wait until the DMA restart the first line
		while(1)
		{
			if(edma->ERR != 0)
			{
				int faulty_dma_channel;

				dp("HRS",(int)(edma->HRS));
				dp("ERR",(int)(edma->ERR));
				dp("INT",(int)(edma->INT));
				dp("ERQ",(int)(edma->ERQ));
				dp("ES",(int)(edma->ES));
				dp("CR",(int)(edma->CR));

				faulty_dma_channel = (edma->ES &0x00001F00) >> 8;
				dp("faulty DMA channel", faulty_dma_channel);
				dump_tcd(&(edma_TCD[((edma->ES &0x00001F00) >> 8)]));	// dump faulty DMA TCD

				dp("row pointer", (int)(fb_row_pointer));
				dp("DLASTSGA", px_dma->DLASTSGA);

				dp("last known TCD", (int)(last_tcd));

				cur_num = ((int)(px_dma->DLASTSGA) - (int)(px_dma_major_loop)) / sizeof(DMABaseClass::TCD_t);
				dp("next line", cur_num);

				switch(faulty_dma_channel)
				{
					case 0:
								{
									DMABaseClass::TCD_t *tcd  = px_dma_major_loop;
									while( tcd < last_tcd)
									{
										dump_tcd(tcd);
										tcd = (DMABaseClass::TCD_t *)(tcd->DLASTSGA);
									} 
								}
								break;

					case 1:
								{
									DMABaseClass::TCD_t *tcd  = sram_u_dma_major_loop;

									while( (tcd->DLASTSGA) > (int)tcd)
									{
										dump_tcd(tcd);
										tcd = (DMABaseClass::TCD_t *)(tcd->DLASTSGA);
									} 
								}
								break;

					case 2:
								break;
				}

				NPRINTLN("DMA crashed.");

				while(1);
			}

			cur_num = ((int)(px_dma->DLASTSGA) - (int)(px_dma_major_loop)) / sizeof(DMABaseClass::TCD_t);

			delay(1);	// don't know why but without this delay, the loop fails

			if(last_line <= cur_num)
				last_line = cur_num - 1;
			else
				break;
		}

		lp_delay = (micros() - start_micros);

		if(lp_delay > loop_delay)
			loop_delay = lp_delay;
	}

	// allow ~10% of error during computation
	if( (loop_delay - 1000000 / img_frame_rate) > ( 10 * 1000000 / (img_frame_rate * 100)))
	{
		NPRINTLN("==========================");
		NPRINTLN("==========================");
		NPRINTLN("==========================");
		NPRINTLN("CPU clock speed is too low");
		NPRINTLN("==========================");
		NPRINTLN("==========================");
		NPRINTLN("==========================");
	}
}

// ============================================================================
void uVGA::stop()
{
	// just in case stop is called before begin
	hftm->SC = 0;		// stop hFTM

	// set all clock and data pins as INPUT
	pinMode(hsync_pin, INPUT);
	pinMode(vsync_pin, INPUT);

	GPIOD_PDDR = 0x00;	// configure all pins of port D as input
}

// ============================================================================
// convert FTM to prescale factor selection
int uVGA::FTM_prescaler_to_selection(int prescaler)
{
	int selection = 0;

	prescaler >>= 1;

	while(prescaler)
	{
		selection++;
		prescaler >>= 1;
	}

	return selection;
}

// ============================================================================
// retrieve real size of frame buffer
void uVGA::get_frame_buffer_size(int *width, int *height)
{
	*width = img_w;
	*height = img_h;
}

// ============================================================================
// wait to be in Vsync
void uVGA::waitBeam()
{
	while(px_dma->DLASTSGA < dma_sync_tcd_address);
}

// ============================================================================
// wait for the next Vsync
void uVGA::waitSync()
{
	// already in sync ?
	while(px_dma->DLASTSGA >= dma_sync_tcd_address);

	while(px_dma->DLASTSGA < dma_sync_tcd_address);
}

