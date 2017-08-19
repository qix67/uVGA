// Newton fractal

#include <uVGA.h>
#include <complex>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>

using namespace std;

const byte cmap[] = {0b00000000, 0b11100000, 0b11100100, 0b11101000, 0b11101100, 0b11110000, 0b11110100, 0b11111000, 0b11111100,
							0b11011100, 0b10111100, 0b10011100, 0b01111100, 0b01011100, 0b00111100, 0b00011100, 0b00011101, 0b00011110,
							0b00011111, 0b00011011, 0b00010111, 0b00010011, 0b00001111, 0b00001011, 0b00000111, 0b00000011, 0b00100011,
							0b01000011, 0b01100011, 0b10000011, 0b10100011, 0b11000011, 0b11100011, 0b11100010, 0b11100001, 0b11100000, 0b00000000
						  };

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
	int i, j;
	int n;
	int fb_width, fb_height;

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	for(i = 0; i < fb_width; i++)
	{
		for(j = 0; j < fb_height; j++)
		{
			complex<float> c(0, 0), z((i - (float)fb_width / 2.0) / ((float)fb_width / 4.0), (j - (float)fb_height / 2.0) / ((float)fb_height / 4.0)), zz, zzz;
			complex<float> one(1.0, 0.0), two(2.0, 0.0), three(3.0, 0.0);

			for(n = 1; n < (int)sizeof(cmap); n++)
			{
				zz = z * z;
				zzz = zz * z;
				c = (zzz + zz + z + one) / (zz * three + z * two + one);
				z -= c;
				if(norm(c) < 0.01)break;
			}
			uvga.drawPixel(i, j, cmap[sizeof(cmap) - n]);
		}
	}
	for(;;);
}

