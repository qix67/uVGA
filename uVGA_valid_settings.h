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

#if F_CPU == 240000000
#include "uVGA_valid_settings_240MHz.h"

#elif F_CPU == 192000000
#include "uVGA_valid_settings_192MHz.h"

#elif F_CPU == 180000000
#include "uVGA_valid_settings_180MHz.h"

#elif F_CPU == 168000000
#include "uVGA_valid_settings_168MHz.h"

#elif F_CPU == 144000000
#include "uVGA_valid_settings_144MHz.h"

#elif F_CPU == 120000000
#include "uVGA_valid_settings_120MHz.h"

#elif F_CPU == 96000000
#include "uVGA_valid_settings_96MHz.h"

#elif F_CPU == 72000000
#include "uVGA_valid_settings_72MHz.h"

#elif F_CPU == 48000000
#include "uVGA_valid_settings_48MHz.h"

#elif F_CPU == 24000000
#include "uVGA_valid_settings_24MHz.h"

#else

#pragma message "No resolution defined for this CPU frequency. Known CPU frequency: 240Mhz, 192MHz, 180Mhz, 168Mhz, 144Mhz, 96Mhz, 72Mhz, 48Mhz"

#endif
#endif
