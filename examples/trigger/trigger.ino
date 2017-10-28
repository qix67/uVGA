// Trigger
// show how to use trigger location
// LED is initially on. After 5 seconds, led blink is driven by a PWM signal
// PWM base frequency is 60Hz, duty is (vtotal - vres) / vtotal  = ~4.6%

#include <uVGA.h>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>

DMAChannel start_img_dma;
DMAChannel end_img_dma;

uint32_t led_port_pin[1] = {CORE_PIN13_BITMASK};

void setup()
{
	Serial.begin(115200);
	int ret; 
	
	uvga.disable_clocks_autostart();
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	delay(5000);

	start_img_dma.begin(false);
	start_img_dma.sourceBuffer(led_port_pin, 4);
	start_img_dma.destination(CORE_PIN13_PORTCLEAR);
	uvga.trigger_dma_channel(UVGA_TRIGGER_LOCATION_START_OF_VGA_IMAGE, start_img_dma.channel);

	end_img_dma.begin(false);
	end_img_dma.sourceBuffer(led_port_pin,4);
	end_img_dma.destination(CORE_PIN13_PORTSET);
	uvga.trigger_dma_channel(UVGA_TRIGGER_LOCATION_END_OF_VGA_IMAGE, end_img_dma.channel);

	ret = uvga.begin(&modeline);

	Serial.println(ret);

	if(ret != 0)
	{
		Serial.println("fatal error");
		while(1);
	}

	uvga.clocks_start();
	uvga.clear(0b1);
}

void loop()
{	
}
