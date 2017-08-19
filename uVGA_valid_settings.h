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
#pragma message "240Mhz 703x300"

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
#pragma message "240Mhz 452x300"

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
#pragma message "192Mhz 602x300"

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
#pragma message "180Mhz 360x300"

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
#pragma message "180Mhz 566x300"

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
#pragma message "168Mhz 598x240"

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
#pragma message "144Mhz 512x240"

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
#pragma message "144Mhz 326x240"

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
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
// Teensy 3.6 & 3.5
#define UVGA_120M_426X240
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
// Teensy 3.2
#define UVGA_120M_277X96
#endif
#endif

// ======================================================
// valid video settings for 144MHz CPU
// * UVGA_120M_426X240  (640x480@60Hz, FB resolution: 426x240), stable
// * UVGA_120M_277X96  (640x480@60Hz, FB resolution: 277x96), stable (compatible w/ teensy 3.2)

#ifdef UVGA_120M_426X240
// 120Mhz, 640x480@60Hz, FB resolution: 426x240
#pragma message "120Mhz 426x240"

uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 222,
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

#ifdef UVGA_120M_277X96
// 120Mhz, 640x480@60Hz, FB resolution: 277x96
#pragma message "120Mhz 277x96"

uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 277,
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
   .repeat_line = 5,
   .horizontal_position_shift = 16,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#elif F_CPU == 96000000

#ifdef UVGA_DEFAULT_REZ

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
// Teensy 3.6 & 3.5
#define UVGA_96M_340X240
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
// Teensy 3.2
#define UVGA_96M_222X120
#endif

#endif


// ======================================================
// valid video settings for 96MHz CPU
// * UVGA_96M_340X240  (640x480@60Hz, FB resolution: 340x240), stable, very minor glitch
// * UVGA_96M_804X240  (640x480@60Hz, FB resolution: 804x240), fairly stable, very minor glitch. Very weird resolution, it has really more pixels per line than possible :) 
// * UVGA_96M_222X120  (640x480@60Hz, FB resolution: 222x120), stable, very minor glitch. (compatible w/ teensy 3.2)

#ifdef UVGA_96M_340X240
// 96Mhz, 640x480@60Hz, FB resolution: 340x240
#pragma message "96Mhz 340x240"

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
#pragma message "96Mhz 804x240"

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

#ifdef UVGA_96M_222X120
// 96Mhz, 640x480@60Hz, FB resolution: 222x240
#pragma message "96Mhz 222x240"

uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 222,
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
   .repeat_line = 4,
   .horizontal_position_shift = 8,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_SINGLE,
	};
#endif

#elif F_CPU == 72000000

#ifdef UVGA_DEFAULT_REZ

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
// Teensy 3.6 & 3.5
#define UVGA_72M_602X240
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
// Teensy 3.2
#define UVGA_72M_255X120
#endif

#endif

// ======================================================
// valid video settings for 72MHz CPU
// * UVGA_72M_602X240 (640x480@60Hz, FB resolution: 602x240), fairly stable, very minor glitch. DMA starts to fail to compensate clock slowness
// * UVGA_72M_255X120 (640x480@60Hz, FB resolution: 255x120), fairly stable, very minor glitch. DMA starts to fail to compensate clock slowness. Use nearly all RAM on teensy 3.2.
// * UVGA_72M_166X120 (640x480@60Hz, FB resolution: 166x120), fairly stable, very minor glitch. DMA starts to fail to compensate clock slowness. compatible with teensy 3.2

#ifdef UVGA_72M_602X240
// 72Mhz, 640x480@60Hz, FB resolution: 602x240
#pragma message "72Mhz 602x240"

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

#ifdef UVGA_72M_255X120
// 72Mhz, 640x480@60Hz, FB resolution: 255x120
#pragma message "72Mhz 255x120"

uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 255,
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
   .repeat_line = 4,
   .horizontal_position_shift = 8,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_SINGLE,
	};
#endif

#ifdef UVGA_72M_166X120
// 72Mhz, 640x480@60Hz, FB resolution: 166x120
#pragma message "72Mhz 166x120"

uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = 166,
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
   .repeat_line = 4,
   .horizontal_position_shift = 8,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_SINGLE,
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
#pragma message "48Mhz 404x240"

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

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
// Teensy 3.6 & 3.5
#define UVGA_24M_200X240
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
// Teensy 3.2
#define UVGA_24M_200X120
#endif

#endif

// at this frequency, it is more a proof of concept as pixel clock is higher than CPU frequency

// ======================================================
// valid video settings for 24MHz CPU
// * UVGA_24M_200X240 (640x480@60Hz, FB resolution: 202x240), nearly stable, lot of glitch. Frequency is too low to let time to DMA to copy from SRAM_U to SRAM_L, single DMA forced
// * UVGA_24M_200X120 (640x480@60Hz, FB resolution: 202x120), nearly stable, lot of glitch. Frequency is too low to let time to DMA to copy from SRAM_U to SRAM_L, single DMA forced (compatible w/ teensy 3.2)

#ifdef UVGA_24M_200X240
// 24Mhz, 640x480@60Hz, FB resolution: 200x240, single DMA
#pragma message "24Mhz 200x240"

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

#ifdef UVGA_24M_200X120
// 24Mhz, 640x480@60Hz, FB resolution: 200x160, single DMA
#pragma message "24Mhz 200x160"

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
   .repeat_line = 4,
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
