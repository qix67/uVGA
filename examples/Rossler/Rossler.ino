// Rössler chaotic attractor
// adapted from due demo by JLS1

#include <uVGA.h>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>


float a = 0.2;
float b = 0.2;
float c = 8;

float x, y, z;

float xnn = 0;
float ynn = 0;
float znn = 0;

float dt = 0.02;

int cnt = 0;

boolean vis;

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

	uvga.clear();
	//VGA.drawText(" R\x94ssler chaotic attractor ", 25, 25, 0, 1);
	//VGA.drawText("dx/dt = -y - z", 25, 50, 1);
	//VGA.drawText("dy/dt = x + ax", 25, 60, 1);
	//VGA.drawText("dz/dt = b + z(x-c)", 25, 70, 1);
	//VGA.drawText("a = 0.2", 25, 90, 1);
	//VGA.drawText("b = 0.2", 25, 100, 1);
	//VGA.drawText("c = 8", 25, 110, 1);
}

void loop()
{
	int fb_width, fb_height;

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	xnn = -(y + z);
	ynn = x + a * y;
	znn = b + x * z - c * z;

	x = x + xnn * dt;
	y = y + ynn * dt;
	z = z + znn * dt;

	uvga.drawPixel(fb_width / 2 + (fb_width / 90 * ((z / 2) - x)), fb_height / 4 + (fb_height / 45 * ((z / 2) - y)), vis ? 0xFF: 0);

	if (cnt == 1500)
	{
		vis = !vis;
		cnt = 0;
	}

	cnt ++;
}
