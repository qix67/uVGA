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
	int m, b, t, p;
	int x, y;
	int i;

	uvga.get_frame_buffer_size(&fb_width, &fb_height);


	for(m = 1 ; ; m += 2)
	{
		for(b = 0; b < 17; b++)
		{
			for(y = 0; y < fb_height; y++)
			{
				t = m * m * y;

				for(x = 0; x < fb_width; x++)
				{
					p = ((t * x) & (1 << b)) ? t % 256:0;

					uvga.drawPixel(x,y,p);
				}
			}

			i = 30;
			while( i-- )
			{
				uvga.waitSync();
			}
		}
	}
}
