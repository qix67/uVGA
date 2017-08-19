// Hello World Colour using VGA library by stimmer

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
	// text colour
	uvga.setForegroundColor(random(256));
 
	// text background colour
	uvga.setBackgroundColor(random(256)); 
	
	// print message
	uvga.print(" Hello Teensy ");
	
	uvga.waitSync();
}
