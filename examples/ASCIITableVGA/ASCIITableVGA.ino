// This is the ASCIITable example by Nicholas Zambetti and Tom Igoe, modified for uVGA library.

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

	uvga.println("ASCII Table ~ Character Map");  
}

byte thisByte = 33; 

void loop()
{  
	uvga.write(thisByte);    
	uvga.print(", dec: "); 
	uvga.print(thisByte);      
	uvga.print(", hex: "); 
	uvga.print(thisByte, HEX);     
	uvga.print(", oct: "); 
	uvga.print(thisByte, OCT);     
	uvga.print(", bin: "); 
	uvga.println(thisByte, BIN);   
	thisByte++;  
  
	delay(300);
}
