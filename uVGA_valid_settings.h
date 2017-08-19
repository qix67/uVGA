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

#ifndef UVGA_VALID_SETTINGS_H
#define UVGA_VALID_SETTINGS_H

#include <uVGA.h>

extern "C"
{

#if F_CPU == 240000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_240M_703X300
#endif

// ======================================================
// valid video settings for 240MHz CPU
// * UVGA_240M_703X300  (800x600@60Hz, FB resolution: 703x300), very stable but used nearly all memory
// * UVGA_240M_452X240  (800x600@60Hz, FB resolution: 452x300), very stable

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_240M_703X300
// 240Mhz, 800x600@60Hz, FB resolution: 703x300
// very stable but used nearly all memory
uVGAmodeline modeline = {
   .pixel_clock = 40000000, //40MHz
   .hres = 703,		// don't know why but sometimes, 710 works here
   .hsync_start = 840,
   .hsync_end = 968,
   .htotal = 1056,
   .vres = 600,
   .vsync_start = 601,
   .vsync_end = 605,
   .vtotal = 628,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_240M_452X300
// 240Mhz, 800x600@60Hz, FB resolution: 452x300
// very stable
uVGAmodeline modeline = {
   .pixel_clock = 40000000, //40MHz
   .hres = 452,
   .hsync_start = 840,
   .hsync_end = 968,
   .htotal = 1056,
   .vres = 600,
   .vsync_start = 601,
   .vsync_end = 605,
   .vtotal = 628,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 192000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_192M_602X300
#endif

// ======================================================
// valid video settings for 192MHz CPU
// * UVGA_192M_602X300  (800x600@56Hz, FB resolution: 602x300), very stable

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_192M_602X300
// 192Mhz, 800x600@56Hz, FB resolution: 602x300
// very stable
uVGAmodeline modeline = {
   .pixel_clock = 36000000, //36MHz
   .hres = 602,
   .hsync_start = 824,
   .hsync_end = 896,
   .htotal = 1024,
   .vres = 600,
   .vsync_start = 601,
   .vsync_end = 603,
   .vtotal = 625,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 180000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_180M_566X300
#endif

// ======================================================
// valid video settings for 180MHz CPU
// * UVGA_180M_360X300  (800x600@56Hz, FB resolution: 360x300), very stable
// * UVGA_180M_566X300  (800x600@56Hz, FB resolution: 566x300), very stable

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_180M_360X300
// 180Mhz, 800x600@56Hz, FB resolution: 360x300
// very stable
uVGAmodeline modeline = {
   .pixel_clock = 36000000, //36MHz
   .hres = 360,
   .hsync_start = 824,
   .hsync_end = 896,
   .htotal = 1024,
   .vres = 600,
   .vsync_start = 601,
   .vsync_end = 603,
   .vtotal = 625,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_180M_566X300
// 180Mhz, 800x600@56Hz, FB resolution: 566x300
// very stable
uVGAmodeline modeline = {
   .pixel_clock = 36000000, //36MHz
   .hres = 566,
   .hsync_start = 824,
   .hsync_end = 896,
   .htotal = 1024,
   .vres = 600,
   .vsync_start = 601,
   .vsync_end = 603,
   .vtotal = 625,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 168000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_168M_598X240
#endif

// ======================================================
// valid video settings for 180MHz CPU
// * UVGA_168M_598X240  (640x480@60Hz, FB resolution: 598x240), end of line not very sharp

#ifdef UVGA_168M_598X240
// 168Mhz, 640x480@60Hz, FB resolution: 598x240
// end of line not very sharp
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 598,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 4,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 144000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_144M_512X240
#endif

// ======================================================
// valid video settings for 144MHz CPU
// * UVGA_144M_512X240  (640x480@60Hz, FB resolution: 512x240), stable
// * UVGA_144M_326X240  (640x480@60Hz, FB resolution: 326x240), stable, nearly a standard VGA resolution

#ifdef UVGA_144M_512X240
// 144Mhz, 640x480@60Hz, FB resolution: 512x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 512,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 12,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#ifdef UVGA_144M_326X240
// 144Mhz, 640x480@60Hz, FB resolution: 326x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 326,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 12,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 120000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_120M_426X240
#endif

// ======================================================
// valid video settings for 144MHz CPU
// * UVGA_120M_426X240  (640x480@60Hz, FB resolution: 426x240), stable

#ifdef UVGA_120M_426X240
// 120Mhz, 640x480@60Hz, FB resolution: 426x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 426,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 16,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 96000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_96M_340X240
#endif

// ======================================================
// valid video settings for 96MHz CPU
// * UVGA_96M_340X240  (640x480@60Hz, FB resolution: 340x240), stable, very minor glitch
// * UVGA_96M_804X240  (640x480@60Hz, FB resolution: 804x240), fairly stable, very minor glitch. Very weird resolution, it has really more pixels per line than possible :) 

#ifdef UVGA_96M_340X240
// 96Mhz, 640x480@60Hz, FB resolution: 340x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 340,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 16,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#ifdef UVGA_96M_804X240
// 96Mhz, 640x480@60Hz, FB resolution: 804x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 804,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 8,
	.pixel_h_stretch = UVGA_HSTRETCH_NORMAL,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 72000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_72M_602X240
#endif

// ======================================================
// valid video settings for 72MHz CPU
// * UVGA_72M_602X240 (640x480@60Hz, FB resolution: 602x240), fairly stable, very minor glitch. DMA starts to fail to compensate clock slowness

#ifdef UVGA_72M_602X240
// 72Mhz, 640x480@60Hz, FB resolution: 602x240
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 602,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 8,
	.pixel_h_stretch = UVGA_HSTRETCH_NORMAL,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif


#elif F_CPU == 48000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_48M_404X240
#endif

// ======================================================
// valid video settings for 48MHz CPU
// * UVGA_48M_404X240 (640x480@60Hz, FB resolution: 404x240), nearly stable, some glitch. Frequency is too low to let time to DMA to copy from SRAM_U to SRAM_L, single DMA forced

#ifdef UVGA_48M_404X240
// 48Mhz, 640x480@60Hz, FB resolution: 404x240, single DMA
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 404,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 4,
	.pixel_h_stretch = UVGA_HSTRETCH_NORMAL,
	.dma_settings = UVGA_DMA_SINGLE,
	};
#endif

#elif F_CPU == 24000000

#ifdef UVGA_DEFAULT_REZ
#define UVGA_24M_200X240
#endif

// at this frequency, it is more a proof of concept as pixel clock is higher than CPU frequency

// ======================================================
// valid video settings for 24MHz CPU
// * UVGA_24M_200X240 (640x480@60Hz, FB resolution: 202x240), nearly stable, lot of glitch. Frequency is too low to let time to DMA to copy from SRAM_U to SRAM_L, single DMA forced

#ifdef UVGA_24M_200X240
// 24Mhz, 640x480@60Hz, FB resolution: 200x240, single DMA
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 200,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = 480,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = 2,
   .horizontal_position_shift = 2,
	.pixel_h_stretch = UVGA_HSTRETCH_NORMAL,
	.dma_settings = UVGA_DMA_SINGLE,
	};
#endif

#else

#pragma message "No resolution defined for this CPU frequency. Known CPU frequency: 240Mhz, 192MHz, 180Mhz, 168Mhz, 144Mhz, 96Mhz, 72Mhz, 48Mhz"

#endif
}
#endif
