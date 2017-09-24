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

#ifndef UVGA_VALID_SETTINGS_120MHZ_H
#define UVGA_VALID_SETTINGS_120MHZ_H

#include <uVGA.h>

#if F_CPU == 120000000
extern "C"
{
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

#define UVGA_HREZ 426
#define UVGA_VREZ 480
#define UVGA_RPTL 2
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = UVGA_HREZ,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = UVGA_VREZ,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = UVGA_RPTL,
   .horizontal_position_shift = 16,
	.pixel_h_stretch = UVGA_HSTRETCH_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

#ifdef UVGA_120M_277X96
// 120Mhz, 640x480@60Hz, FB resolution: 277x96
#pragma message "120Mhz 277x96"

#define UVGA_HREZ 277
#define UVGA_VREZ 480
#define UVGA_RPTL 5
uVGAmodeline modeline = {
   .pixel_clock = 25180000, //25.18Mhz
   .hres = UVGA_HREZ,
   .hsync_start = 656,
   .hsync_end = 752,
   .htotal = 800,
   .vres = UVGA_VREZ,
   .vsync_start = 490,
   .vsync_end = 492,
   .vtotal = 525,
   .h_polarity = UVGA_NEGATIVE_POLARITY,
   .v_polarity = UVGA_NEGATIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = UVGA_RPTL,
   .horizontal_position_shift = 16,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

}
#endif
#endif
