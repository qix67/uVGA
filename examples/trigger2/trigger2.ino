// Trigger2
// show how to use trigger location
// It is a useless demo. DMA change "red" MSB (pin 5) at start of line and main loop clear the same bit
// If the main loop does nothing; left margin becomes red as well as top and bottom margin but it is a side effect
// because nothing clear the bit (unlike when frame buffer is displayed)
// Note: because LCD monitor show no margin, you have to manually move the screen to the right using monitor control panel
// to observe the red band

#include <uVGA.h>

uVGA uvga;

#define UVGA_DEFAULT_REZ
#include <uVGA_valid_settings.h>

DMAChannel start_line_dma;

uint32_t red2_port_pin[1] = {CORE_PIN5_BITMASK};

void setup()
{
	Serial.begin(115200);
	int ret; 
	
	uvga.disable_clocks_autostart();

	start_line_dma.begin(false);
	start_line_dma.sourceBuffer(red2_port_pin, 4);
	start_line_dma.destination(CORE_PIN5_PORTSET);
	uvga.trigger_dma_channel(UVGA_TRIGGER_LOCATION_START_OF_DISPLAY_LINE, start_line_dma.channel);

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
	//digitalWrite(5,LOW);
}
