uVGA library
by Eric PREVOTEAU

1) Quick start
==============

Place this folder in your libraries directory
Start or restart the Arduino IDE

Wiring
------

Hsync pin (default: 22) -> 82R resistor -> VGA 13		// depend on HSync FTM channel 

Vsync pin (default: 29) -> 82R resistor -> VGA 14		// can be changed to any pin except pin of port D

   on teensy 3.2, default Vsync pin is pin 10.



Teensy pin 5  (port D, bit 7) -> 2k2 resistor -> VGA pin 2 (green)
Teensy pin 21 (port D, bit 6) -> 1k resistor -> VGA pin 2 (green)
Teensy pin 20 (port D, bit 5) -> 470R resistor -> VGA pin 2 (green)

Teensy pin 6  (port D, bit 4) -> 2k2 resistor -> VGA pin 1 (red)
Teensy pin 8  (port D, bit 3) -> 1k resistor -> VGA pin 1 (red)
Teensy pin 7  (port D, bit 2) -> 470R resistor -> VGA pin 1(red)

Teensy pin 14 (port D, bit 1) -> VGA pin 14 (VSync)
Teensy pin 2  (port D, bit 0) -> VGA pin 13 (HSync)

Teensy pin GND -> VGA pins 5,6,7,8,10

for more accurate colors, replace 2k2 by 2k and 470R by 510R


2) Basic usage
==============

#include <uVGA.h>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>

void setup()
{
	uvga.begin(&modeline);
}

This will use the default resolution given for your CPU frequency.

You can choose a different modeline by changing the define. Accepted values are
in uVGA_valid_settings.h

Then call any drawing or text functions.

For text, use uvga.print(...) and uvga.println(...) just like Serial.print(...)
and Serial.println(...)

3) Colours
==========

color format is RGB332 (RRRGGGBB)


4) API
======

class instantiation
uVGA(int dma_number = 0, int sram_u_dma_number = 1, int sram_u_dma_fix_number = 2, int hsync_ftm_num = 0, int hsync_ftm_channel_num = 0, int x1_ftm_channel_num = 7,int vsync_pin = 29, int graphic_dma = DMA_NUM_CHANNELS - 1)

  Initialize class internal parameters.
  Class requirements:
    - 4 DMA channels (0-15), can be any
    - 1 FTM with 3 channels (on teensy 3.6, only FTM0 and FTM3 are possible).
      All others channels will be free.

  On the chosen FTM, the class will use channels hsync_ftm_channel_num,
  hsync_ftm_channel_num+1 and x1_ftm_channel_num.

  hsync_ftm_channel_num MUST be even (0, 2, 4, 6)

  The pair (hsync_ftm_num, hsync_ftm_channel_num) defines the pin generating
  Hsync signal. Look at teensyduino's teensy3/core_pins.h file, CORE_FTM*_CH*_PIN
  On teensy 3.6, valid pairs (not on port D) are:
    - (0,0) => pin 22
    - (0,2) => pin 9
    - (3,0) => pin 2
    - (3,4) => pin 35
    - (3,6) => pin 37

  On teensy 3.2, valid pairs (not on port D) are:
    - (0,0) => pin 22
    - (0,2) => pin 9. This FTM can use pin 22 but you will have to configure it
                      yourself.

  1 or 3 DMA will be used to generate video signal, the 4th one is (will be)
  used to accelerate some drawing functions. Currently, only 2 functions support
  DMA acceleration (HLine & VLine) however, unlike the first 3 DMA channels
  which never runs at the same time, this one can run at any time and drawing
  a long line greatly disturb the other DMA channels thus DMA acceleration is
  currently disabled

  If any parameter is invalid, the library will fallback to its default value.


uvga_error_t  uvga.begin(uVGAmodeline *modeline)

  Initializes the display
  Returns: 0 on success, uvga_error_t code on failure
  Not all resolutions work on all monitors.

  see below for uVGAmodeline description


void uvga.end();

  Stops the display. NOT TESTED


void uvga.clear(int col=0);

  Fills screen with color col, or black if col is not specified.


void get_frame_buffer_size(int *width, int *height);

  Retrieves the width and the height of the frame buffer.
  Width and height of the frame buffer are computed from modeline settings.


int getPixel(int x, int y)

  Returns the color of the pixel at (x,y)


void uvga.drawPixel(int x, int y, int col);

  Draws pixel at (x,y) in colour col


void uvga.drawLine(int x0, int y0, int x1, int y1, int col);

  Draws line from (x0,y0) to (x1,y1) in colour col


void uvga.drawTri(int x0,int y0,int x1,int y1,int x2,int y2,int col);
void uvga.fillTri(int x0,int y0,int x1,int y1,int x2,int y2,int col);

  Draws or fills triangle (x0,y0),(x1,y1),(x2,y2) in colour col


void uvga.drawRect(int x0, int y0, int x1, int y1, int col);
void uvga.fillRect(int x0, int y0, int x1, int y1, int col);

  Draws or fills rectangle with corners (x0,y0),(x1,y1) in colour col


void uvga.drawCircle(int x, int y, int r, int col);
void uvga.fillCircle(int x, int y, int r, int col);

  Draws or fills circle center (x,y) radius r in colour col


void uvga.drawEllipse(int x0, int y0, int x1, int y1, int col);
void uvga.fillEllipse(int x0, int y0, int x1, int y1, int col);

  Draws or fills ellipse bounded by rectangle (x0,y0),(x1,y1) in colour col


void uvga.drawText(const char *text, int x, int y, int fgcol, int bgcol= -1, uvga_text_direction dir = UVGA_DIR_RIGHT);

  Draws text at any pixel position. 
  (x,y) is the top-left corner of the text before rotation.
  fgcol is the colour of the text. bgcol is the colour of the text background
  or -1 for a transparent background.
  dir is the direction of the text. 
    UVGA_DIR_RIGHT is left to right,
    UVGA_DIR_TOP is bottom to top,
    UVGA_DIR_LEFT is right to left,
    UVGA_DIR_BOTTOM is top to bottom,


void uvga.scroll(int x, int y, int w, int h, int dx, int dy,int col=0);

  Scrolls an area of the screen, top left corner (x,y), width w, height h
  by (dx,dy) pixels. If dx>0 scrolling is right, dx<0 is left. dy>0 is down,
  dy<0 is up. Empty area is filled with color col (only in horizontal (dy=0)
  and vertical scroll (dx=0))


void uvga.copy(int s_x, int s_y, int d_x, int d_y, int w, int h);

  Copy image area from position (s_x, s_y) to position (d_x, d_y).
  Area is w pixels width and h pixels height.

  The function supports overlapping area and off-screen. If source area is
  fully off-screen, nothing occurs. If source area is partially off-screen,
  w and h will be automatically adjusted to fit fully in screen.


void uvga.moveCursor(int column, int line);

  Moves the print position to (column, line)  


void uvga.setPrintWindow(int x, int y, int width, int height);

  Restricts the printing window to an area of width x height characters, at a 
  position (x,y) (in pixel).


void uvga.unsetPrintWindow()

  Restores the print window to be the whole screen.


void uvga.clearPrintWindow();

  Clears the print window to the current text paper colour.


void uvga.scrollPrintWindow();

  Scrolls the print window up one line and moves the print position to the bottom.


void uvga.setForegroundColor(uint8_t fg_color);

  Sets the text colour to fg_color (RGB332).


void uvga.setBackgroundColor(int bg_color);

  Sets the text background colour to bg_color (RGB332) or -1 for transparent background


virtual size_t uvga.write(const uint8_t *buffer, size_t size);
virtual size_t uvga.write(uint8_t c);

  These functions are similar to Serial.write, except output gets printed to
  the screen. These functions enable print and println to work correctly.


void uvga.waitBeam()
void uvga.waitSync()

  These functions wait for the beam position to be off-screen. waitBeam will return 
  immediately if the beam is already off-screen, waitSync will always wait for the 
  next frame. These can be used to reduce flicker.


5) modeline
===========

Used resolution is described using a modeline stored in an uVGAmodeline
structure. Most of the data can be obtain from a standard modeline for Xorg
server.

Standard modeline format is:
ModeLine "640x480" 25.20 640 656 752 800 480 490 492 525 -HSync -VSync

Pixel clock in MHz ^^^^^ |   |   |   |   |   |   |   |   |      |
horizontal resolution    ^^^ |   |   |   |   |   |   |   |      |
HSync start                  ^^^ |   |   |   |   |   |   |      |
HSync end                        ^^^ |   |   |   |   |   |      |
Total pixels in line (including sync)^^^ |   |   |   |   |      |
vertical resolution                      ^^^ |   |   |   |      |
VSync start                                  ^^^ |   |   |      |
VSync end                                        ^^^ |   |      |
Total line per image (including sync)                ^^^ |      |
Polarity of horizontal signal                            ^^^^^^ |
Polarity of vertical signal                                     ^^^^^^

The modeline structure contains the following fields:

 * int pixel_clock: 
    it is the pixel clock. Its value cannot be higher than F_BUS frequency
    which depends on CPU frequency. The pixel clock is in Hz, not in MHZ.


 * int hres:
    number of pixels per line. Unlike all other modelines values and because
    VGA is an analog signal, the number of horizontal pixels can be freely
    defined. For example. @180MHz and UVGA_HSTRETCH_WIDE, in 800x600@56, the
    maximum number of pixel is 445. @144Mhz, in 640x480@56Hz, you can fit 724
    pixels per line :)


 * int hsync_start:

    position of the start of the horizontal sync signal.


 * int hsync_end:

    position of the end of the horizontal sync signal.


 * int htotal:

    number of pixels per line including sync signal.


 * int vres:

    number of lines per frame.


 * int vsync_start:

    position of the start of the vertical sync signal.


 * int vsync_end:

    position of the end of the vertical sync signal.


 * int vtotal:

    number of lines per frame including sync signal.


 * uvga_signal_polarity_t h_polarity:
 * uvga_signal_polarity_t v_polarity:

    polarity of horizontal and vertical signal. Possible values are:
     - UVGA_POSITIVE_POLARITY
     - UVGA_NEGATIVE_POLARITY


 * uvga_color_mode_t img_color_mode:

    video mode to use. The only possible value is UVGA_RGB332.


 * int repeat_line:

    number of times to display each line of the frame buffer. The height of the
    frame buffer is vrez / repeat_line.
     - 1 means 640x480 (hres x vres) has a frame buffer of 640x480 pixels
     - with 2, frame buffer is 640x240 pixels
     - with 3, frame buffer is 640x160 pixels
    frame buffer height is always rounded to immediate next integer


Some additionnal settings allow fine tuning of the video mode

 * int horizontal_position_shift:

    Delay before displaying the start of line. Default value is 1. Increase to
    move image to the right if first pixels are not visible.


 * uvga_pixel_hstretch pixel_h_stretch:

    increase width of pixels on monitor. This works by throttling DMA and
    inserting wait state. Default is no wait state. The other settings insert
    4 and 8 wait states. Possible values are:
     - UVGA_HSTRETCH_NORMAL
     - UVGA_HSTRETCH_WIDE
     - UVGA_HSTRETCH_ULTRA_WIDE


 * uvga_dma_settings dma_settings:

    The library can display images using either 1 or 3 DMA channels. With 3 DMA
    channels, the image is more stable but requires roughly 
    hres + vres * 4 more bytes. However, if the frame buffer is small enough
    to fit in SRAM_L (lower SRAM), there is no need to use 3 DMA channels to
    obtain stable image. Possible values are
     - UVGA_DMA_AUTO  (let library decide if multiple DMA channels are required)
     - UVGA_DMA_SINGLE  (force library to use only one DMA channel)


6) Miscellanous informations
============================

* To enable debug mode in the library, edit uVGA.cpp and uncomment the line
   #define DEBUG

  This will dump a huge amount of data regarding FTM and TCD.

* If you create your own modeline structure from a standard modeline and the
  monitor fails to sync properly, it is probably because the library send
  image pixels during HSync signal. Try to lower hres value and if it is not
  enough, you can also increase horizontal_position_shift value a little.

* Frame buffer size is not width * height bytes because all lines address and
  size must be aligned on a multiple of 16.

  A line uses (hres + 1 + 0xF) & ~0xF bytes. +1 comes from the black pixel
  added at the end of each line. 


7) How it works
===============

A lot of magic...  1 FlexTimer (2 channels in complementary mode + 1 channel in
PWM mode) and 1 (or 3) DMA channel

The FlexTimer uses 2 combined channels in complementary mode (asymetric PWM) to
create HSync signal at the correct time. On the same FlexTimer, a 3rd channel
(named X1 here) is used to start the DMA at the correct time on each line.

The DMA generates both image and VSync signal and once started never stops. The
DMA uses a set of TCD (transfer control descriptor) linked together using DMA
scatter/gather mode. The last TCD is linked to the first one thus the DMA
never has nothing to do. Each TCD describes a line to process and is started by X1
FTM channel (the 3rd channel).

On a normal line, DMA copies as fast as possible all pixels of the line + 1
(1 black pixel is added at the end of each line to power of RGB pins).
After the image, 3 TCD are used to set VSYNC signal properly, 1 before the vsync
to wait it (may not exist depending on modeline), 1 at the beginning of vsync to
set it and 1 after the vsync to clear it. Vsync TCD uses nearly no ressource
because they copy 4 bytes only on each line.

It is not possible to fine tune DMA copy speed. Because it has the highest
priority, it starts at the correct time (nearly every time :) )

I tried to adjust speed of DMA using FTM and PIT but it slowed down far too much
and is not accurate due to round error (PIT@60MHz to obtain a 45MHz signal gives
a FTM modulo of 1 with an error of 15Mhz). Moreover, additionnal wait cycles are
added to start and stop DMA.

The 2 ways I found to slow down DMA is:
 * the hard way: using DMA bandwith control to add wait states. Unfortunately,
   there is only 3 settings available: no wait state, 4 cycles wait state,
   8 cycles wait state
 * the extreme way: modifying CPU frequency

Due to the fact VGA is an analog signal, the width of each pixel is no really
"defined". Using a 800x600 resolution, I successfully packed more than 1000
pixels on each line.

To improve video stability, pixel DMA channel has the highest priority among
other DMA channels. Moreover, the crossbar switch is configured to give DMA a
maximal priority to SRAM backdoor and GPIO. To gain one more cycle, SRAM
backdoor port and GPIO port are parked to DMA when they are not in used.
This gives a huge boost in performance. Finally, SRAM backdoor is configured
to give absolute priority to DMA on SRAM_L and favor DMA on SRAM_U.

Everything is not perfect. Due to the fact pixel duration is approximated, the
monitor may or may not understand totally what it receives and with some colors,
pixels may be blurry (VGA like :) )

A last problem comes from SRAM. SRAM is splitted in 2, SRAM_L and SRAM_U. SRAM_L
is accessed using CODE bus and has 0 wait state. SRAM_U is accessed using system
bus and has at least 1 wait state. Unfortunately, the biggest part of the SRAM
is SRAM_U.

To fix this problem, a 2nd DMA channel is used. For all lines located in SRAM_U,
a copy will be performed to bring them back in SRAM_L before displaying them.
This channel will be started using TCD dma channel link of the previously
displayed line. In case repeat line factor is bigger than 1, to reduce
bandwidth usage, the copy will happen only on new line, not on its duplicates.

However, this 2nd channel triggers a new problem. It is not possible for a TCD
to modify destination address between TCD minor loop. This problem does not exist
in the 1st channel because the destination address is always the same address.
To bypass this problem, after TCD minor loop of the 2nd channel is processed,
it triggers a start on the 3rd channel. TCD of this 3rd channel will simply reset
the value of destination address (yes, the DMA reprograms one of its register
itself :) ).

All these copies waste a bit of RAM bandwidth but the 2nd DMA channel copies are
performed using burst mode and all these DMA TCD and DMA channel are
automatically processed by the DMA engine without any CPU help.

Note: if all frame buffer lines are in SRAM_L, only the first DMA will be used

Finally, these system works perfectly... as long as nothing disturb it.

Hsync position is always correct because FTM channels cannot be bothered by
anything.

Vsync position is roughly correct. Even if DMA starts a bit late, due to signal
duration, it does not seem to disturb monitor (at least my old LCD 17").

The main problem is pixel generation. If the DMA is delayed, minor line
oscillation can be visible. Due to the high priority, DMA seems to always
obtain access to SRAM before CPU. The only thing which seems to delay DMA
channel start is... the DMA itself. If another DMA channel performing a "long"
transfer, despite having a lower priority, it delays pixel DMA channel.
Performing various data copies using CPU is OK. A simple Serial.print is not OK.

