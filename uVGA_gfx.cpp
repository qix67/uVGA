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

#define dump(v)      {Serial.print(#v ":"); Serial.println(v);}

#define NO_DMA_GFX

// clip X to inside horizontal range
inline int uVGA::clip_x(int x)
{
	if(x < 0)
		return 0;
	if(x >= fb_width)
		return fb_width - 1;
	return x;
}

// clip Y to inside vertical range
inline int uVGA::clip_y(int y)
{
	if(y < 0)
		return 0;
	if(y >= fb_height)
		return fb_height - 1;
	return y;
}

// wait for GFX dma to become free
inline void uVGA::wait_idle_gfx_dma()
{
#ifndef NO_DMA_GFX
	while(edma->ERQ & (1 << gfx_dma_num));
#endif
}

// draw a single pixel. If the pixel is out of screen, it is not displayed
void uVGA::drawPixel(int x, int y, int color)
{
	// pixel outside of visible area ?
	if( (clip_x(x) != x)
		|| (clip_y(y) != y)
		)
		return;

	wait_idle_gfx_dma();

	drawPixelFast(x, y, color);
}

// draw a single pixel WITHOUT performing any clipping test
inline void uVGA::drawPixelFast(int x, int y, int color)
{
	frame_buffer[y * fb_row_stride + x] = color;
}

int uVGA::getPixel(int x, int y)
{
	return _getPixel(x, y);
}

inline int uVGA::_getPixel(int x, int y)
{
	// pixel outside of visible area ?
	if( (clip_x(x) != x)
		|| (clip_y(y) != y)
		)
		return 0;

	wait_idle_gfx_dma();

	return getPixelFast(x, y);
}

inline int uVGA::getPixelFast(int x, int y)
{
	return *(frame_buffer + y * fb_row_stride + x);
}

// draw a horizontal line pixel with clipping
void uVGA::drawHLine(int y, int x1, int x2, int color)
{
	int nx1;
	int nx2;

	// line out of screen ?
	if(clip_y(y) != y)
		return;

	nx1 = clip_x(x1);
	nx2 = clip_x(x2);

	wait_idle_gfx_dma();

	if(x1 <= x2)
		drawHLineFast(y, nx1, nx2, color);
	else
		drawHLineFast(y, nx2, nx1, color);
}

// draw a horizontal line pixel WITHOUT performing any clipping test
// x1 always <= x2
inline void uVGA::drawHLineFast(int y, int x1, int x2, int color)
{
#ifdef NO_DMA_GFX
	uint8_t *ptr = frame_buffer + y * fb_row_stride + x1;

	while(x1 <= x2)
	{
		*ptr++ = color;
		x1++;
	}
#else
	gfx_dma_color[0] = color;

	gfx_dma->SADDR = gfx_dma_color;
	gfx_dma->SOFF = 0;
	gfx_dma->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_8BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);
	gfx_dma->NBYTES = x2 - x1 + 1;
	gfx_dma->SLAST = 0;
	gfx_dma->DADDR = frame_buffer + y * fb_row_stride + x1;
	gfx_dma->DOFF = 1;
	gfx_dma->CITER = 1;
	gfx_dma->BITER = 1;
	gfx_dma->CSR = DMA_TCD_CSR_DREQ;

	edma->SERQ = gfx_dma_num;
#endif
}

// draw a vertical line pixel with clipping
void uVGA::drawVLine(int x, int y1, int y2, int color)
{
	int ny1;
	int ny2;

	// line out of screen ?
	if(clip_x(x) != x)
		return;

	ny1 = clip_y(y1);
	ny2 = clip_y(y2);

	wait_idle_gfx_dma();

	if(y1 <= y2)
		drawVLineFast(x, ny1, ny2, color);
	else
		drawVLineFast(x, ny2, ny1, color);
}

// draw a vertical line pixel WITHOUT performing any clipping test
// y1 always <= y2
inline void uVGA::drawVLineFast(int x, int y1, int y2, int color)
{
#ifdef NO_DMA_GFX
	uint8_t *ptr = frame_buffer + y1 * fb_row_stride + x;

	while(y1 <= y2)
	{
		*ptr = color;
		ptr += fb_row_stride;
		y1++;
	}
#else
	gfx_dma_color[0] = color;

	gfx_dma->SADDR = gfx_dma_color;
	gfx_dma->SOFF = 0;
	gfx_dma->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_8BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);
	gfx_dma->NBYTES = y2 - y1 + 1;
	gfx_dma->SLAST = 0;
	gfx_dma->DADDR = frame_buffer + y1 * fb_row_stride + x;
	gfx_dma->DOFF = fb_row_stride;
	gfx_dma->CITER = 1;
	gfx_dma->BITER = 1;
	gfx_dma->CSR = DMA_TCD_CSR_DREQ;

	edma->SERQ = gfx_dma_num;
#endif
}

void uVGA::fillRect(int x0, int y0, int x1, int y1, int color)
{
/*
	int width;
	int height;
*/
	int t;

	x0 = clip_x(x0);
	y0 = clip_y(y0);

	x1 = clip_x(x1);
	y1 = clip_y(y1);

	// increase speed if the rectangle is a single pixel, horizontal or vertical line
	if( (x0 == x1) )
	{
		if(y0 == y1)
		{
			wait_idle_gfx_dma();

			return drawPixelFast(x0, y0, color);
		}
		else
		{
			wait_idle_gfx_dma();

			if(y0 < y1)
				return drawVLineFast(x0, y0, y1, color);
			else
				return drawVLineFast(x0, y1, y0, color);
		}
	}
	else if(y0 == y1)
	{
		wait_idle_gfx_dma();

		if(x0 < x1)
				return drawHLineFast(y0, x0, x1, color);
			else
				return drawHLineFast(y0, x1, x0, color);
	}

	if( x0 > x1 )
	{
		t = x0;
		x0 = x1;
		x1 = t;
	}

	if( y0 > y1 )
	{
		t = y0;
		y0 = y1;
		y1 = t;
	}

	while(y0 <= y1)
	{
		wait_idle_gfx_dma();

		drawHLineFast(y0, x0, x1, color);
		y0++;
	}

/*
	width = abs(x1 - x0);
	height = abs(y1 - y0);

	gfx_dma_color = color;

	wait_idle_gfx_dma();

	// lets program DMA to perform the task and free the CPU
	// source is a single pixel of the given color
	gfx_dma->SADDR = &gfx_dma_color;
	gfx_dma->SOFF = 0;					// stay on this pixel
	gfx_dma->ATTR_SRC = 0				// source data size = 1 byte
	gfx_dma->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE | 									// at end of minor loop, adjust destination address (offset)
										DMA_TCD_NBYTES_MLOFFYES_MLOFF(fb_row_stride - width) |	// adjustment is nb bytes in frame buffer line - copied "color" bytes
										DMA_TCD_NBYTES_MLOFFYES_NBYTES(width) ;			// each minor loop transfert copy a line of the rectangle (width byte)
	gfx_dma->SLAST = 0;					// not pointer correction at end of major loop

	gfx_dma->DADDR = frame_buffer + fb_row_stride * y0 + x0;		// destination is pixel (x0,y0) in the frame buffer
	gfx_dma->DOFF = 1;					// after each 
	gfx_dma->ATTR_DST = 0;				// destination data size = 1 byte
	gfx_dma->CITER = width * height;	// major loop should copy the whole rectangle surface
	gfx_dma->DLASTSGA = 0;				// no scatter/gather mode
	gfx_dma->CSR = 0;
	gfx_dma->BITER = gfx_dma->CITER;

nécessite CR[EMLM]=1
est-ce que ça ne pertubera pas le DMA vidéo ???
mettre DMA_DCHPRIx[ECP] = 1
*/
}

// bitmap format must be the same as modeline.img_color_mode
void uVGA::drawBitmap(int16_t x_pos, int16_t y_pos, uint8_t *bitmap, int16_t bitmap_width, int16_t bitmap_height)
{
	int fx;
	int fy;
	int fw;
	int fh;
	int bx;
	int by;
	int off_x, off_y;
	uint8_t *bitmap_ptr;

	fx = clip_x(x_pos);
	// X position outside of image (right of image)
	if(fx < x_pos)
		return;

	// compute the number of pixels to skip at the beginning of each bitmap line and the number of pixel per line to copy
	if(fx > x_pos)
	{
		bx = fx - x_pos;
		fw = bitmap_width - bx;

		// X position outside of image (right of image)
		if(fw <= 0)
			return;
	}
	else
	{	bx = 0;
		fw = bitmap_width;
	}

	fy = clip_y(y_pos);
	// Y position outside of image (bottom of image)
	if(fy < y_pos)
		return;

	// compute the number of lines to skip at the beginning of bitmap and the number of lines to copy
	if(fy > y_pos)
	{
		by = fy - y_pos;
		fh = bitmap_height - by;

		// Y position outside of image (right of image)
		if(fh <= 0)
			return;
	}
	else
	{	by = 0;
		fh = bitmap_height;
	}

	// here, (fx,fy) is the destination position in the image
	//       (bx,by) is the position in the bitmap
	//			(fw,fh) is the size to copy
	
	wait_idle_gfx_dma();

	for(off_y = 0; off_y < fh; off_y++)
	{
		bitmap_ptr = bitmap + by * bitmap_width + bx;

		for(off_x = 0; off_x < fw; off_x++)
		{
			drawPixelFast(	fx + off_x,
								fy + off_y,
								*bitmap_ptr++
					);
		}
	}

}



#define SWAP(x,y) { (x)=(x)^(y); (y)=(x)^(y); (x)=(x)^(y); }

// Fill a triangle - Bresenham method
// Original from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void uVGA::fillTri(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
	int t1x, t2x, y, minx, maxx, t1xp, t2xp;
	bool changed1 = false;
	bool changed2 = false;
	int signx1, signx2, dx1, dy1, dx2, dy2;
	int e1, e2;

	// Sort vertices
	if (y1 > y2)
	{
		SWAP(y1, y2);
		SWAP(x1, x2);
	}

	if (y1 > y3)
	{
		SWAP(y1, y3);
		SWAP(x1, x3);
	}

	if (y2 > y3)
	{
		SWAP(y2, y3);
		SWAP(x2, x3);
	}

	t1x = t2x = x1;
	y = y1; // Starting points

	dx1 = x2 - x1;

	//delta_and_sign(x0, x1, &delta_x, &sign_x);
	if(dx1 < 0)
	{
		dx1 = -dx1;
		signx1 = -1;
	}
	else
	{
		signx1 = 1;
	}

	dy1 = y2 - y1;
	dx2 = x3 - x1;

	if(dx2 < 0)
	{
		dx2 = -dx2;
		signx2 = -1;
	}
	else
	{
		signx2 = 1;
	}

	dy2 = y3 - y1;

	if (dy1 > dx1)     // swap values
	{
		SWAP(dx1, dy1);
		changed1 = true;
	}

	if (dy2 > dx2)     // swap values
	{
		SWAP(dy2, dx2);
		changed2 = true;
	}

	e2 = dx2 >> 1;

	// Flat top, just process the second half
	if(y1 == y2)
		goto next;

	e1 = dx1 >> 1;

	for (int i = 0; i < dx1;)
	{
		t1xp = 0;
		t2xp = 0;
		if(t1x < t2x)
		{
			minx = t1x;
			maxx = t2x;
		}
		else
		{
			minx = t2x;
			maxx = t1x;
		}
		// process first line until y value is about to change
		while(i < dx1)
		{
			i++;
			e1 += dy1;

			while (e1 >= dx1)
			{
				e1 -= dx1;
				if (changed1)
					t1xp = signx1; //t1x += signx1;
				else
					goto next1;
			}
			if (changed1)
				break;
			else
				t1x += signx1;
		}
		// Move line
next1:
		// process second line until y value is about to change
		while (1)
		{
			e2 += dy2;

			while (e2 >= dx2)
			{
				e2 -= dx2;

				if (changed2)
					t2xp = signx2; //t2x += signx2;
				else
					goto next2;
			}

			if (changed2)
				break;
			else
				t2x += signx2;
		}
next2:
		if(minx > t1x)
			minx = t1x;
		if(minx > t2x)
			minx = t2x;
		if(maxx < t1x)
			maxx = t1x;
		if(maxx < t2x)
			maxx = t2x;

		drawHLine(y, minx, maxx, color);

		// Now increase y
		if(!changed1)
			t1x += signx1;

		t1x += t1xp;

		if(!changed2)
			t2x += signx2;

		t2x += t2xp;
		y += 1;

		if(y == y2)
			break;
	}
next:
	// Second half
	dx1 = x3 - x2;

	if(dx1 < 0)
	{
		dx1 = -dx1;
		signx1 = -1;
	}
	else
	{
		signx1 = 1;
	}

	dy1 = y3 - y2;
	t1x = x2;

	if (dy1 > dx1)     // swap values
	{
		SWAP(dy1, dx1);
		changed1 = true;
	}
	else
	{
		changed1 = false;
	}

	e1 = dx1 >> 1;

	for (int i = 0; i <= dx1; i++)
	{
		t1xp = 0;
		t2xp = 0;

		if(t1x < t2x)
		{
			minx = t1x;
			maxx = t2x;
		}
		else
		{
			minx = t2x;
			maxx = t1x;
		}

		// process first line until y value is about to change
		while(i < dx1)
		{
			e1 += dy1;

			while (e1 >= dx1)
			{
				e1 -= dx1;

				if (changed1)
				{
					t1xp = signx1;   //t1x += signx1;
					break;
				}
				else
					goto next3;
			}

			if (changed1)
				break;
			else
				t1x += signx1;

			if(i < dx1)
				i++;
		}
next3:
		// process second line until y value is about to change
		while (t2x != x3)
		{
			e2 += dy2;

			while (e2 >= dx2)
			{
				e2 -= dx2;

				if(changed2)
					t2xp = signx2;
				else
					goto next4;
			}

			if (changed2)
				break;
			else
				t2x += signx2;
		}
next4:

		if(minx > t1x)
			minx = t1x;
		if(minx > t2x)
			minx = t2x;
		if(maxx < t1x)
			maxx = t1x;
		if(maxx < t2x)
			maxx = t2x;

		drawHLine(y, minx, maxx, color);

		// Now increase y
		if(!changed1)
			t1x += signx1;

		t1x += t1xp;

		if(!changed2)
			t2x += signx2;

		t2x += t2xp;
		y += 1;

		if(y > y3)
			return;
	}
}

// all drawing algorithms taken from http://members.chello.at/~easyfilter/bresenham.html

inline void uVGA::delta_and_sign(int v1, int v2, int *delta, int *sign)
{
	if(v2 > v1)
	{
		*delta = v2 - v1;
		*sign = 1;
	}
	else if(v2 < v1)
	{
		*delta = v1 - v2;
		*sign = -1;
	}
	else
	{
		*delta = 0;
		*sign = 0;
	}
}

// draw a line with or without its last pixel
void uVGA::drawLine(int x0, int y0, int x1, int y1, int color, bool no_last_pixel)
{
	int delta_x;
	int sign_x;
	int delta_y;
	int sign_y;
	int err;
	int err2;

	if(x0 == x1)
	{
		if(y0 == y1)
			drawPixel(x0, y0, color);
		else
			drawVLine(x0, y0, y1, color);
		return;
	}
	else if(y0 == y1)
	{
		drawHLine(y0, x0, x1, color);
	}


	delta_and_sign(x0, x1, &delta_x, &sign_x);
	delta_and_sign(y0, y1, &delta_y, &sign_y);

	err= delta_x - delta_y;

	do
	{
		drawPixel(x0, y0, color);

		err2 = 2 * err;

		if(err2 > -delta_y)
		{
			err -= delta_y;
			x0 += sign_x;
		}
		else if(err2 < delta_x)
		{
			err += delta_x;
			y0 += sign_y;
		}
   } while ( (x0 != x1) || (y0 != y1) );
	
	// plot last line pixel;
	if(!no_last_pixel)
		drawPixel(x1, y1, color);
}

// draw a triangle
void uVGA::drawTri(int x0, int y0, int x1, int y1, int x2, int y2, int color)
{
	drawLinex(x0, y0, x1, y1, color);
	drawLinex(x1, y1, x2, y2, color);
	drawLinex(x2, y2, x0, y0, color);
}

// draw a rectangle
void uVGA::drawRect(int x0, int y0, int x1, int y1, int color)
{
	drawHLine(y0, x0, x1, color);
	drawHLine(y1, x0, x1, color);
	drawVLine(x0, y0, y1, color);
	drawVLine(x1, y0, y1, color);
}

void uVGA::drawCircle(int xm, int ym, int r, int color)
{
	int x = -r;
	int y = 0;
	int err = 2-2*r; /* II. Quadrant */ 

	do
	{
		drawPixel(xm-x, ym+y, color); /*	I. Quadrant */
		drawPixel(xm-y, ym-x, color); /*  II. Quadrant */
		drawPixel(xm+x, ym-y, color); /* III. Quadrant */
		drawPixel(xm+y, ym+x, color); /*  IV. Quadrant */

		r = err;

		if (r <= y)
		{
			y++;
			err += y * 2 + 1;			  /* e_xy+e_y < 0 */
		}

		if ( (r > x) || (err > y))
		{
			x++;
			err += x * 2 + 1; /* e_xy+e_x > 0 or no 2nd y-step */
		}
	} while (x < 0);
}

// Yes, it is weird, it is easier to draw a fill circle than a simple circle
void uVGA::fillCircle(int xm, int ym, int r, int color)
{
	int x, y;

	for (y = -r; y <= r; y++)
	{
		for (x = -r; x <= r; x++)
		{
			if ((x * x) + (y * y) <= (r * r))
			{
				drawHLine(ym + y, xm + x, xm - x, color);
				if(y != 0)
					drawHLine(ym - y, xm + x, xm - x, color);
				break;
			}
		}
	}
}


// x0, y0 = center of the ellipse
// x1, y1 = upper right edge of the bounding box of the ellipse
void uVGA::drawEllipse(int _x0, int _y0, int _x1, int _y1, int color)
{
	int half_height = abs(_y0 - _y1) / 2;
	int half_width = abs(_x0 - _x1) / 2;
	int x0 = (_x0 < _x1 ? _x0 : _x1);
	int y0 = (_y0 < _y1 ? _y0 : _y1) + 2 * half_height;
	int x1 = x0 + half_width * 2;
	int y1 = y0 - half_height * 2;

	int a = abs(x1-x0);
	int b = abs(y1-y0);
	int b1 = b & 1; /* values of diameter */

	long dx = 4 * (1 - a) * b * b;
	long dy = 4 * (b1 + 1) * a * a; /* error increment */
	long err = dx + dy + b1 * a * a;
	long e2; /* error of 1.step */

	// if x1,y1 is not the correct edge, try... something
	if(x0 > x1)
	{
		x0 = x1;
		x1 += a;
	}

	if(y0 > y1)
	{
		y0 = y1;
	}

	y0 += (b + 1) / 2;
	y1 = y0 - b1;	/* starting pixel */
	a *= 8*a;
	b1 = 8*b*b;

	do {
		drawPixel(x1, y0, color); /*	I. Quadrant */
		drawPixel(x0, y0, color); /*  II. Quadrant */
		drawPixel(x0, y1, color); /* III. Quadrant */
		drawPixel(x1, y1, color); /*  IV. Quadrant */

		e2 = 2*err;

		if(e2 <= dy)
		{
			y0++;
			y1--;
			dy += a;
			err += dy;
		}  /* y step */ 
		if((e2 >= dx) || (2*err > dy))
		{
			x0++;
			x1--;
			dx += b1;
			err += dx;
		} /* x step */
	} while (x0 <= x1);
	
	while ((y0 - y1) < b)
	{  /* too early stop of flat ellipses a=1 */
		drawPixel(x0 - 1, y0, color); /* -> finish tip of ellipse */
		drawPixel(x1 + 1, y0++, color); 
		drawPixel(x0 - 1, y1, color);
		drawPixel(x1 + 1, y1--, color); 
	}
}

void uVGA::fillEllipse(int x0, int y0, int x1, int y1, int color)
{
	int x;
	int y;
	int half_height = abs(y0 - y1) / 2;
	int half_width = abs(x0 - x1) / 2;
	int center_x = (x0 < x1 ? x0 : x1) + half_width;
	int center_y = (y0 < y1 ? y0 : y1) + half_height;

	for(y = -half_height; y <= 0; y++)
	{
		for(x = -half_width; x <= 0; x++)
		{
			if( (x * x * half_height * half_height + y * y * half_width * half_width) <= (half_height * half_height * half_width * half_width))
			{
				drawHLine(center_y + y, center_x + x, center_x - x, color);
				if(y != 0)
					drawHLine(center_y - y, center_x + x, center_x - x, color);
				break;
			}
		}
	}
}

// copy area s_x,s_y of w*h pixels to destination d_x,d_y
void uVGA::copy(int s_x, int s_y, int d_x, int d_y, int w, int h)
{
	int c_s_x;
	int c_s_y;
	int c_d_x;
	int c_d_y;
	int c_w;
	int c_h;
	int error;
	int sxpos;
	int sypos;
	int dxpos;
	int dypos;
	int dx;
	int dy;

	int off_x;
	int off_y;

	// nothing to copy ?
	if((w <= 0) || (h <= 0))
		return;

	c_s_x = s_x;
	c_s_y = s_y;
	c_d_x = d_x;
	c_d_y = d_y;
	c_w = w;
	c_h = h;

	// let's do some clipping to avoid overflow

	// 1) adjust position and size of source area according to screen size
	if(c_s_x < 0)
	{
		c_w += c_s_x;
		c_d_x -= c_s_x;
		c_s_x = 0;
	}

	if(c_s_y < 0)
	{
		c_h += c_s_y;
		c_d_x -= c_s_y;
		c_s_y = 0;
	}

	error = c_s_x + c_w;
	if(error >= fb_width)
	{
		c_w = fb_width - c_s_x;
	}

	error = c_s_y + c_h;
	if(error >= fb_height)
	{
		c_h = fb_height - c_s_y;
	}

	// 2) adjust destination position and source size according to screen size
	if(c_d_x < 0)
	{
		c_w += c_d_x;
		c_s_x -= c_d_x;
		c_d_x = 0;
	}

	if(c_d_y < 0)
	{
		c_h += c_d_y;
		c_s_y -= c_d_y;
		c_d_y = 0;
	}

	error = c_d_x + c_w;
	if(error >= fb_width)
	{
		c_w = fb_width - c_d_x;
	}

	error = c_d_y + c_h;
	if(error >= fb_height)
	{
		c_h = fb_height - c_d_y;
	}

	// nothing left to copy ?
	if((c_w <= 0) || (c_h <= 0))
		return;

	if(c_d_y > c_s_y)
	{
		// copy from last line
		sypos = c_s_y + c_h - 1;
		dypos = c_d_y + c_h - 1;
		dy = -1;
	}
	else
	{
		// copy from first line
		sypos = c_s_y;
		dypos = c_d_y;
		dy = 1;
	}
		
	if(c_d_x > c_s_x)
	{
		// copy from last line pixel
		sxpos = c_s_x + c_w - 1;
		dxpos = c_d_x + c_w - 1;
		dx = -1;
	}
	else
	{
		// copy from first line pixel
		sxpos = c_s_x;
		dxpos = c_d_x;
		dx = 1;
	}

	wait_idle_gfx_dma();

	for(off_y = 0; off_y < c_h; off_y++)
	{
		for(off_x = 0; off_x < c_w; off_x++)
		{
			drawPixelFast(	dxpos + off_x * dx,
								dypos + off_y * dy,
								getPixelFast(sxpos + off_x * dx,
												 sypos + off_y * dy)
					);
		}
	}
}

void uVGA::scroll(int x, int y, int w, int h, int dx, int dy,int col)
{
	if(dy == 0)
	{
		if(dx == 0)
			return;

		Hscroll(x, y, w, h, dx, col);
	}
	else if(dx == 0)
	{
		Vscroll(x, y, w, h, dy, col);
	}
	else
	{
		copy(x, y, x + dx, y + dy, w, h);

#pragma message "multiscroll not finished, where should col pixel be put if source and destination area intersect ?"
	}

}

inline void uVGA::Hscroll(int x, int y, int w, int h, int dx, int col)
{
	wait_idle_gfx_dma();

	copy(x, y, x + dx, y, w, h);

	// fill empty area created with col
	if(dx > 0)
	{
		// move to the right => fill area on the left side of source area
		fillRect(x, y, x + dx - 1 , y + h - 1, col);
	}
	else
	{
		// move to the left => fill area on the right side of source area
		fillRect(x + w + dx, y, x + w - 1 , y + h - 1, col);
	}
}

inline void uVGA::Vscroll(int x, int y, int w, int h, int dy, int col)
{
	wait_idle_gfx_dma();

	copy(x, y, x, y + dy, w, h);

	// fill empty area created with col
	if(dy > 0)
	{
		// move to the bottom => fill area on the top side of source area
		fillRect(x, y, x + w - 1 , y + dy - 1, col);
	}
	else
	{
		// move to the top => fill area on the bottom side of source are
		fillRect(x, y + h + dy, x + w - 1 , y + h - 1, col);
	}
}

// Text primitives
void uVGA::init_text_settings()
{
	cursor_x = 0;
	cursor_y = 0;
	font_width = 8;		// font width != 8 is not supported
	font_height = 8;
	print_window_x = 0;
	print_window_y = 0;
	print_window_w = fb_width / font_width;
	print_window_h = fb_height / font_height;

	foreground_color = 0xFF;
	background_color = 0x00;
	transparent_background = false;
}

// print a text at a given coordinates. if bg_color = -1, background is transparent
void uVGA::drawText(const char *text, int x, int y, int fg_col, int bg_col, uvga_text_direction dir)
{
	uint8_t t;
	int i,j;
	uint8_t *char_matrix;
	uint8_t b;
	uint8_t pix;

	while((t = ((uint8_t)*text++)) != '\0')
	{
		char_matrix = &_vga_font8x8[t * font_height];

		for(j = 0; j < font_height; j++)
		{
			b = *char_matrix++;

			for(i = 0; i < font_width; i++)
			{
				pix = b & (128 >> i);

				// pixel to draw or non transparent background color ?
				if((pix) || (bg_col != -1))
				{
					switch(dir)
					{
						case UVGA_DIR_RIGHT:
													drawPixel(x + i, y + j, (pix ? fg_col : bg_col));
													break;

						case UVGA_DIR_TOP:
													drawPixel(x + j, y - i, (pix ? fg_col : bg_col));
													break;

						case UVGA_DIR_LEFT:
													drawPixel(x - i, y - j, (pix ? fg_col : bg_col));
													break;

						case UVGA_DIR_BOTTOM:
													drawPixel(x - j, y + i, (pix ? fg_col : bg_col));
													break;

					}
				}
			}
		}

		switch(dir)
		{
			case UVGA_DIR_RIGHT:
											x += font_width;
											break;

			case UVGA_DIR_TOP:
											y -= font_width;
											break;

			case UVGA_DIR_LEFT:
											x -= font_width;
											break;

			case UVGA_DIR_BOTTOM:
											y += font_width;
											break;

		}
	}
}

void uVGA::moveCursor(int column, int line)
{
	if(column < 0)
		cursor_x = 0;
	else if(column >= print_window_w)
		cursor_x = print_window_w - 1;
	else
		cursor_x = column;

	if(line < 0)
		cursor_y = 0;
	else if(line >= print_window_h)
		cursor_y = print_window_h - 1;
	else
		cursor_y = line;
}

// define printing window, width and height are in pixel
void uVGA::setPrintWindow(int x, int y, int width, int height)
{
	if(x < 0)
		print_window_x = 0;
	else if(x >= (fb_width - font_width))
		print_window_x = fb_width - font_width - 1;
	else
		print_window_x = x;

	if(y < 0)
		print_window_y = 0;
	else if(y >= (fb_height - font_height))
		print_window_y = fb_height - font_height - 1;
	else
		print_window_y = y;

	if(width < font_width)
		width = font_width;

	if(height < font_height)
		height = font_height;

	print_window_w = width / font_width;
	print_window_h = height / font_height;
}

void uVGA::clearPrintWindow()
{
	fillRect(print_window_x, print_window_y, print_window_x + print_window_w * font_width - 1, print_window_y + print_window_h * font_height - 1, background_color);
	cursor_x = 0;
	cursor_y = 0;
}

void uVGA::unsetPrintWindow()
{
	print_window_x = 0;
	print_window_y = 0;
	print_window_w = fb_width / font_width;
	print_window_h = fb_height / font_height;
}

void uVGA::scrollPrintWindow()
{
	// move the 2nd line and the following ones one line up
	Vscroll(print_window_x, print_window_y + font_height, 
			print_window_w * font_width, (print_window_h - 1) * font_height, 
			-font_height,
			background_color);
}


void uVGA::setForegroundColor(uint8_t fg_color)   // RGB332 format
{
	foreground_color = fg_color;
}

void uVGA::setBackgroundColor(int bg_color)       // RGB332 format or -1 for transparent background
{
	if(bg_color == -1)
		transparent_background = true;
	else
	{
		transparent_background = false;
		background_color = bg_color;
	}
}

size_t uVGA::write(uint8_t c)
{
	char buf[2];

	switch(c)
	{
		case '\r':
						cursor_x = 0;
						return 1;

		case '\n':
						cursor_x = 0;
						cursor_y ++;

						if(cursor_y >= print_window_h)
						{
							scrollPrintWindow();
							cursor_y = print_window_h - 1;
						}
						return 1;

		case '\t':
						write(' ');

						if(print_window_w >= 8)	// prevent neverending loop if print window width is too small to contain a TAB
						{
							while(cursor_x & 7)
								write(' ');
						}
						return 1;

		default:
					// not enough space on the line ?
					if(cursor_x >= print_window_w)
						write('\n');

					buf[0] = c;
					buf[1] = '\0';

					drawText(buf, print_window_x + cursor_x * font_width, print_window_y + cursor_y * font_height, foreground_color, background_color, UVGA_DIR_RIGHT);
					cursor_x++;
					return 1;
	}
}

size_t uVGA::write(const uint8_t *buffer, size_t size)
{
	size_t i;

	for(i = 0; i < size; i++)
		write(buffer[i]);

	return size;
}
