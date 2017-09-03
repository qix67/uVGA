// Ripple
// 3d graph of sinc function
// demonstrates low-level access to colour buffer

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

void loop()
{
	int fb_width, fb_height;
	int x, y;
	int my;
	float X, Y, Z, R;
	uint16_t c;
	byte col;

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	for(x = 0; x < fb_width; x++)
	{
		my = fb_height;

		for(Z = -15.02; Z < 15.0; Z += 0.01)
		{
			X = (x - ((float)fb_width) / 2.0 - 0.02) / 16.0;
			R = sqrtf(X * X + Z * Z);
			Y = 150.0 * (1.0 + sinf(R) / R); // sinc(x) == sin(x)/x

			y = 320.0 - (Z * 4.0) - Y;
			if((y < my) && (y >= 0))
			{
				c = Y;

				while(c > 255)
					c -= 255;

				uvga.drawPixel(x,y, c);
				my = y;
			}
		}
	}

	for(;;)
	{
		for(y = 0; y < fb_height; y++)
		{
			for(x = 0; x < fb_width; x++)
			{
				col = uvga.getPixel(x, y);

				if(col)
				{
					col++;
					if(col == 0)
						col++;
					uvga.drawPixel(x, y, col);
				}
			}
		}
	}
}
