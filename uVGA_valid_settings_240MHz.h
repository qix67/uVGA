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

#ifndef UVGA_VALID_SETTINGS_240MHZ_H
#define UVGA_VALID_SETTINGS_240MHZ_H

#include <uVGA.h>


#if F_CPU == 240000000

extern "C"
{
#ifdef UVGA_DEFAULT_REZ
#define UVGA_240M_703X300
#endif

// ======================================================
// valid video settings for 240MHz CPU
// * UVGA_240M_703X300  (800x600@60Hz, FB resolution: 703x300), very stable but used nearly all memory
// * UVGA_240M_452X300  (800x600@60Hz, FB resolution: 452x300), very stable
// * UVGA_240M_452X200  (800x600@60Hz, FB resolution: 452x200), very stable
// * UVGA_240M_560X240  (640x480@60Hz, FB resolution: 560x240), very stable

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_240M_703X300
// 240Mhz, 800x600@60Hz, FB resolution: 703x300
// very stable but used nearly all memory
#pragma message "240Mhz 703x300"

#define UVGA_HREZ 703
#define UVGA_VREZ 600
#define UVGA_RPTL 2
uVGAmodeline modeline = {
   .pixel_clock = 40000000, //40MHz
   .hres = UVGA_HREZ,		// don't know why but sometimes, 710 works here
   .hsync_start = 840,
   .hsync_end = 968,
   .htotal = 1056,
   .vres = UVGA_VREZ,
   .vsync_start = 601,
   .vsync_end = 605,
   .vtotal = 628,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = UVGA_RPTL,
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

#define UVGA_HREZ 452
#define UVGA_VREZ 600
#define UVGA_RPTL 2
uVGAmodeline modeline = {
   .pixel_clock = 40000000, //40MHz
   .hres = UVGA_HREZ,
   .hsync_start = 840,
   .hsync_end = 968,
   .htotal = 1056,
   .vres = UVGA_VREZ,
   .vsync_start = 601,
   .vsync_end = 605,
   .vtotal = 628,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = UVGA_RPTL,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_240M_452X200
// 240Mhz, 800x600@60Hz, FB resolution: 452x200
// very stable
#pragma message "240Mhz 452x200"

#define UVGA_HREZ 452
#define UVGA_VREZ 600
#define UVGA_RPTL 3
uVGAmodeline modeline = {
   .pixel_clock = 40000000, //40MHz
   .hres = UVGA_HREZ,
   .hsync_start = 840,
   .hsync_end = 968,
   .htotal = 1056,
   .vres = UVGA_VREZ,
   .vsync_start = 601,
   .vsync_end = 605,
   .vtotal = 628,
   .h_polarity = UVGA_POSITIVE_POLARITY,
   .v_polarity = UVGA_POSITIVE_POLARITY,
   .img_color_mode = UVGA_RGB332,
   .repeat_line = UVGA_RPTL,
   .horizontal_position_shift = 14,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};
#endif

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef UVGA_240M_560X240
// 240Mhz, 640x480@60Hz, FB resolution: 560x240
// image is stable but pixels slightly flicker on my LCD
#pragma message "240Mhz 560x240"

#define UVGA_HREZ 560
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
   .horizontal_position_shift = 10,
	.pixel_h_stretch = UVGA_HSTRETCH_ULTRA_WIDE,
	.dma_settings = UVGA_DMA_AUTO,
	};

#endif

}
#endif
#endif
