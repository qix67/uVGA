// DrawingTest
// test of all drawing functions

// This example requires a lot of bandwidth due to DMA displaying an image using nearly all SRAM
// and the CPU drawing a lot of pixels in the same SRAM.

// the delay() calls are here only to let you see lines misplacement when DMA is delayed (if they exists)
// Despite this, the image stays visible and the monitor is not losing its sync
#include <uVGA.h>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>

void setup()
{
	int ret; 
	ret = uvga.begin(&modeline);

	Serial.println(ret);

	if(ret != 0)
	{
		Serial.println("fatal error");
		while(1);
	}
}

double a=0;

void loop()
{	
	int fb_width, fb_height;
	int x0, y0, x1, y1, x2, y2;
	static uvga_text_direction dir[] = { UVGA_DIR_RIGHT, UVGA_DIR_TOP, UVGA_DIR_LEFT, UVGA_DIR_BOTTOM };

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	x0=random(fb_width);
	y0=random(fb_height);
	x1=random(fb_width);
	y1=random(fb_height);
	x2=random(fb_width);
	y2=random(fb_height);
	uvga.fillTri(x0,y0,x1,y1,y2,y2,random(512)-256);
	uvga.drawTri(x0,y0,x1,y1,y2,y2,random(512)-256);
	//delay(10);

	x0=random(fb_width);
	y0=random(fb_height);
	x1=random(fb_width);
	y1=random(fb_height);
	uvga.fillRect(x0,y0,x1,y1,random(512)-256);
	//delay(10);
	uvga.drawRect(x0,y0,x1,y1,random(512)-256);
	//delay(10);
	
	x0=random(fb_width);
	y0=random(fb_height);
	x1=random(fb_width / 2);
	y1=random(fb_height / 2);
	uvga.fillEllipse(x0,y0,x1,y1,random(512)-256);
	uvga.drawEllipse(x0,y0,x1,y1,random(512)-256);
	//delay(10);
	
	x0=random(fb_width);
	y0=random(fb_height);
	int r=random(70);
	uvga.fillCircle(x0,y0,r,random(512)-256);
	uvga.drawCircle(x0,y0,r,random(512)-256);
	//delay(10);
	
	x0=random(fb_width);
	y0=random(fb_height);
	r=random(5)-1;
	uvga.drawText("Hello Teensy!",x0,y0,random(256),random(256),dir[r]);
	
	x0=random(fb_width);
	y0=random(fb_height);
	r=random(5)-1;
	uvga.drawText("uVGA Library",x0,y0,random(256),random(256),dir[r]);
	//delay(10);
	
	uvga.scroll(0,0,fb_width,fb_height,8*sin(a),8*cos(a), random(256) - 1);
	a+=0.05;
	//delay(1000/60);
}
