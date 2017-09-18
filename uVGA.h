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

#ifndef _UVGA_H
#define _UVGA_H

#include <Arduino.h>
#include <avr_emulation.h>
#include <DMAChannel.h>
#include "Print.h"

#include <uVGA_FTM.h>
#include <uVGA_DMA.h>

typedef enum uvga_error_t
{
	UVGA_OK = 0,
	UVGA_NO_VALID_VIDEO_MODE_FOUND = -1,
	UVGA_FAIL_TO_ALLOCATE_FRAME_BUFFER = -2,
	UVGA_FAIL_TO_ALLOCATE_DMA_BUFFER = -3,
	UVGA_UNKNOWN_COLOR_MODE = -4,
	UVGA_FAIL_TO_ALLOCATE_ROW_POINTER_ARRAY = -5,
	UVGA_FAIL_TO_ALLOCATE_DMA_ROW_POINTER_ARRAY = -6,
	UVGA_FAIL_TO_ALLOCATE_SRAM_L_BUFFER = -7,
	UVGA_FAIL_TO_ALLOCATE_SRAM_L_BUFFER_IN_SRAM_L = -8,
	UVGA_UNKNOWN_ERROR = -9,
	UVGA_FRAME_BUFFER_FIRST_LINE_NOT_IN_SRAM_L = 10,
} uvga_error_t;

typedef enum uvga_signal_polarity_t
{
	UVGA_POSITIVE_POLARITY,
	UVGA_NEGATIVE_POLARITY
} uvga_signal_polarity_t;

typedef enum uvga_color_mode_t
{
	UVGA_RGB332,
} uvga_color_mode_t;

typedef enum uvga_pixel_hstretch
{
	UVGA_HSTRETCH_NORMAL,
	UVGA_HSTRETCH_WIDE,
	UVGA_HSTRETCH_ULTRA_WIDE,
} uvga_pixel_hstretch;

typedef enum uvga_dma_settings
{
	// let library decide if multiple DMA channels are required
	UVGA_DMA_AUTO,				// RGB signal on GPIO

	// force library to use only one DMA channel
	UVGA_DMA_SINGLE,			// RGB signal on GPIO
} uvga_dma_settings;

typedef enum uvga_text_direction
{
	UVGA_DIR_RIGHT,
	UVGA_DIR_TOP,
	UVGA_DIR_LEFT,
	UVGA_DIR_BOTTOM,
} uvga_text_direction;

#define SRAM_U_START_ADDRESS				0x20000000

// to provide value from EDID or Modeline
typedef struct
{
	int pixel_clock;			// in Hz
	short hres;					// horizontal resolution. Unlike all other modelines values and because VGA is an analog signal, the number of horizontal pixels can be freely defined
									// For example. @180MHz and UVGA_HSTRETCH_WIDE, in 800x600@56, the maximum number of pixel is 445.
									// @144Mhz, in 640x480@56Hz, you can fit 724 pixels per line :)
	short hsync_start;
	short hsync_end;
	short htotal;

	short vres;					// vertical resolution
	short vsync_start;
	short vsync_end;
	short vtotal;

	uvga_signal_polarity_t h_polarity;
	uvga_signal_polarity_t v_polarity;

	uvga_color_mode_t img_color_mode;
	short repeat_line;			// number of times to display each line of the frame buffer.
									// frame buffer height = vres / repeat_line;
									// 1 means 640x480 (hres x vres) has a frame buffer of 640x480 pixels
									// with 2, frame buffer is 640x240 pixels
									// with 3, frame buffer is 640x160 pixels
									// frame buffer height is always rounded to immediate next integer

	// custom settings;
	// image start position. Default: 1. Increase to move image to the right if first pixels are not visible
	short horizontal_position_shift;

	// increase width of pixels. This works by throttling DMA and inserting wait state. Default is no wait state. The other settings insert 4 and 8 wait states
	uvga_pixel_hstretch pixel_h_stretch;

	// choose dma settings
	uvga_dma_settings dma_settings;
} uVGAmodeline;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define DEFAULT_VSYNC_PIN 29
#else
#define DEFAULT_VSYNC_PIN 10
#endif

class uVGA : public Print
{
public:
	// =========================================================
	// video settings
	// =========================================================
	// Default: DMA0, DMA1, DMA2, HSYNC on FTM0_CH0 (uses FTM0_CH0, CH1, CH7)
	//          VSYNC on pin 29 (teensy 3.5, 3.6), 10 (teensy 3.2)
	//          gfx_dma = last dma channel available
	// =========================================================
	uVGA(int dma_number = 0, int sram_u_dma_number = 1, int sram_u_dma_fix_number = 2, int hsync_ftm_num = 0, int hsync_ftm_channel_num = 0, int x1_ftm_channel_num = 7,int vsync_pin = DEFAULT_VSYNC_PIN, int graphic_dma = DMA_NUM_CHANNELS - 1);

	// display VGA image
	uvga_error_t begin(uVGAmodeline *modeline = NULL);
	void end();

	// retrieve real size of the frame buffer
	void get_frame_buffer_size(int *width, int *height);

	// wait to be in Vsync
	void waitBeam();

	// wait next Vsync
	void waitSync();

	// =========================================================
	// graphic primitives
	// =========================================================

	void clear(int color = 0);
	int getPixel(int x, int y);
	void drawPixel(int x, int y, int color);
	void drawRect(int x0, int y0, int x1, int y1, int color);
	void fillRect(int x0, int y0, int x1, int y1, int color);
	void drawLine(int x0, int y0, int x1, int y1, int color, bool no_last_pixel = false);
	void drawHLine(int y, int x1, int x2, int color);
	void drawVLine(int y, int x1, int x2, int color);
	void drawTri(int x0, int y0, int x1, int y1, int x2, int y2, int color);
	void fillTri(int x0, int y0, int x1, int y1, int x2, int y2, int color);
	void drawCircle(int xm, int ym, int r, int color);
	void fillCircle(int xm, int ym, int r, int color);
	void drawEllipse(int x0, int y0, int x1, int y1, int color);
	void fillEllipse(int x0, int y0, int x1, int y1, int color);
	void scroll(int x, int y, int w, int h, int dx, int dy,int col);

	void copy(int s_x, int s_y, int d_x, int d_y, int w, int h);
	void drawBitmap(int16_t x_pos, int16_t y_pos, uint8_t *bitmap, int16_t bitmap_width, int16_t bitmap_height);

	void drawText(const char *text, int x, int y, int fgcol, int bgcol= -1, uvga_text_direction dir = UVGA_DIR_RIGHT);
	void moveCursor(int column, int line);

	// define text print window. Width and height are in pixels and cannot be smaller than font width and height
	void setPrintWindow(int x, int y, int width, int height);
	void unsetPrintWindow();
	void clearPrintWindow();
	void scrollPrintWindow();

	void setForegroundColor(uint8_t fg_color);	// RGB332 format
	void setBackgroundColor(int bg_color);			// RGB332 format or -1 for transparent background
	virtual size_t write(const uint8_t *buffer, size_t size);
	virtual size_t write(uint8_t c);

private:
	// width and height of the image (comes from begin() call)
	short img_w;
	short img_h;
	short img_frame_rate;
	uvga_signal_polarity_t h_polarity;
	uvga_signal_polarity_t v_polarity;
	uvga_color_mode_t img_color_mode;
	short complex_mode_ydiv;				// number of times a line from the frame buffer must be displayed consecutively on the screen (UVGA_RGB332_COMPLEX_YDIV only)

	// width and height of the screen (include blanking time)
	short scr_w;
	short scr_h;

	// shift start position of each line on the screen
	short hpos_shift;

	// HSync position in pixel
	short hsync_start_pix;
	short hsync_end_pix;

	// VSync position in pixel
	short vsync_start_pix;
	short vsync_end_pix;

	// pixel clock = at least (PIT frequency / PIT overflow value)
	int pxc_freq;			// frequency of the pixel clock
	int pxc_base_freq;	// FTM base frequency

	short dma_num;
	short sram_u_dma_num;
	short sram_u_dma_fix_num;
	short gfx_dma_num;
	uvga_dma_settings dma_config_choice;

	// hsync FTM settings
	int hftm_base_freq;	// same as PIT frequency
	int hftm_prescaler;	// FTM prescaler	(1,2,4,...128)
	int hftm_modulo;		// max FTM value
	int hftm_hsync_start;// FTM first channel duty => start of Hsync pulse
	int hftm_hsync_end;	// FTM second channel duty => end of Hsync pulse

	short hsync_ftm;			// FTM to use to generate Hsync
	short hsync_ftm_channel;	// FTM channel to use on hsync_ftm. MUST BE EVEN. due to combine mode, this channel and the next will be used
								// the digital pin associated to this channel will output VGA hsync signal

	short hsync_pin;			// pin sending Hsync signal

	short x1_ftm_channel;	// FTM channel to use on hsync_ftm to general horizontal visible signal (used to mitigate PIT trigger on DMA)

	short x1_pin;

	FTM_REGS_t *hftm;
	
	// vsync settings
	short vsync_pin;			// pin sending Vsync signal
	uint32_t vsync_bitmask;	// bit mask to use in GPIOx_P[SC]OR to set vsync signal
	volatile uint32_t *vsync_gpio_no_sync_level;	// these 2 pointers are address of GPIOx_PSOR and GPIOx_PCOR
	volatile uint32_t *vsync_gpio_sync_level;		// there ordre depends on the polarity of the vsync signal
	

	FTM_REGS_t *vftm;

	// pixel pin address
	volatile void *pixel_pin_address;	// on GPIO mode, it is GPIOD_PDOR

	// DMA settings
	EDMA_REGs *edma;								// address of eDMA registers

	DMABaseClass::TCD_t *edma_TCD;			// address of eDMA TCD registers

	// DMA used to display image
	volatile DMABaseClass::TCD_t *px_dma;				// address of DMA channel registers
	volatile uint8_t *px_dmamux;				// address of DMA channel multiplexer
	volatile uint8_t *px_dmaprio;				// address of DMA channel priority
	int px_dma_rq_src;							// DMA request source (cf. K20 reference manual, 3.3.8.1 DMA MUX request sources

	int px_dma_nb_major_loop;					// number of minor loops in the major loop
	DMABaseClass::TCD_t *px_dma_major_loop;// array of DMA transfer (minor loop) to process to build screen. array MUST BE 32 bytes aligned (eDMA requirement)

	DMABaseClass::TCD_t *last_tcd;
	int dma_sync_tcd_address;					// address of the TCD containing the start of sync. It is only used by waitSync()

	// stretch pixel horizontaly using DMA bandwidth control
	short px_dma_bwc;

	// DMA used to copy frame buffer line in SRAM_U to SRAM_L
	bool sram_u_dma_required;
	short first_line_in_sram_u;
	volatile DMABaseClass::TCD_t *sram_u_dma;				// address of DMA channel registers
	volatile uint8_t *sram_u_dmamux;				// address of DMA channel multiplexer
	volatile uint8_t *sram_u_dmaprio;				// address of DMA channel priority

	short sram_u_dma_nb_major_loop;					// number of minor loops in the major loop
	DMABaseClass::TCD_t *sram_u_dma_major_loop;// array of DMA transfer (minor loop) to process to build screen. array MUST BE 32 bytes aligned (eDMA requirement)

	DMABaseClass::TCD_t *sram_u_dma_fix;				// address of DMA channel registers
	volatile uint8_t *sram_u_dma_fixmux;				// address of DMA channel multiplexer
	volatile uint8_t *sram_u_dma_fixprio;				// address of DMA channel priority
	DMABaseClass::TCD_t *sram_u_dma_fix_major_loop;// array of DMA transfer (minor loop) to process to build screen. array MUST BE 32 bytes aligned (eDMA requirement)
	
	// to reduce memory usage, all rows (any where in the code) are stored in this array
	uint8_t *all_allocated_rows;				// it is the real address of all allocated rows
	uint8_t *all_allocated_rows_aligned;	// this address is 16 bytes aligned
														// address of all allocated rows (fb_row_stride bytes per row).
														// its contains fb_height+1 rows. The first one is the SRAM_L buffer.

	// SRAM_L buffer
	uint8_t *sram_l_dma_address;				// address used by DMA

	// DMA used to perform graphic task
	volatile DMABaseClass::TCD_t *gfx_dma;				// address of DMA channel registers
	volatile uint8_t *gfx_dmamux;				// address of DMA channel multiplexer
	volatile uint8_t *gfx_dmaprio;				// address of DMA channel priority

	byte gfx_dma_color[4];						// color copied by DMA during graphic task

	// video buffer
	uint8_t *frame_buffer;							// frame buffer address is the address used to perform drawing
															// most of the time, these 3 address are identical
															// however, they can be diffrent if frame buffer must be align on an address (DMA requirement) or if DMA uses a buffer containing video synchro
															// 
	short fb_row_stride;								// number of bytes per line of frame buffer
	short fb_width;
	short fb_height;

	uint8_t **fb_row_pointer;					// pointer on start of each line of the frame buffer
														// array contains fb_height_entries pointing on first pixel off each line
														// only used in complex color mode
														// in complex color mode, fb_row_pointer[y] = frame_buffer + z * fb_row_stride, z is between 0 and fb_height

	uint8_t **dma_row_pointer;					// pointer on start of each line used by the DMA
														// if a line is in SRAM_U and a 2nd DMA 
	// GFX settings
	uint8_t foreground_color;
	uint8_t background_color;
	bool transparent_background;

	short cursor_x;			// cursor x position in print window in CHARACTER
	short cursor_y;			// cursor y position in print window in CHARACTER
	short font_width;
	short font_height;
	short print_window_x;	// x position in pixel of text window 
	short print_window_y;	// y position in pixel of text window
	short print_window_w;	// text window width in CHARACTER
	short print_window_h;	// text window height in CHARACTER

	void clocks_init();
	void signal_pins_init();
	uvga_error_t dma_init();
	uvga_error_t rgb332_dma_init_dma_single_repeat_1();
	uvga_error_t rgb332_dma_init_dma_single_repeat_more_than_1();
	uvga_error_t rgb332_dma_init_dma_multiple_repeat_1();
	uvga_error_t rgb332_dma_init_dma_multiple_repeat_2();
	uvga_error_t rgb332_dma_init_dma_multiple_repeat_more_than_2();
	uvga_error_t monochrome_dma_init_repeat_1();

	DMABaseClass::TCD_t *dma_append_vsync_tcds(DMABaseClass::TCD_t *cur_tcd);
	
	void clocks_start();
	void stop();
	void set_pin_alternate_function_to_FTM(int pin_num);
	uvga_error_t compute_ntsc_video_mode();

	inline void wait_idle_gfx_dma();
	void init_text_settings();

	int FTM_prescaler_to_selection(int prescaler);
	uint8_t *alloc_32B_align(int size);

	// utility function
	inline int clip_x(int x);
	inline int clip_y(int y);

	// delta = abs(v2-v1); sign = sign of (v2-v1);
	inline void delta_and_sign(int v1, int v2, int *delta, int *sign);

	// =========================================================
	// internal graphic primitives
	// =========================================================
	inline int _getPixel(int x, int y);
	inline int getPixelFast(int x, int y);
	inline void drawPixelFast(int x, int y, int color);
	inline void drawHLineFast(int y, int x1, int x2, int color);
	inline void drawVLineFast(int x, int y1, int y2, int color);

	inline void drawLinex(int x0, int y0, int x1, int y1, int color)
	{
		drawLine(x0, y0, x1, y1, color, true);
	}
	
	inline void Vscroll(int x, int y, int w, int h, int dy ,int col);
	inline void Hscroll(int x, int y, int w, int h, int dx ,int col);

	void dump_tcd(DMABaseClass::TCD_t *tcd);
};

extern uVGA uvga;
extern unsigned char _vga_font8x8[];

#endif

