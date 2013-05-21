// vim: set foldmethod=marker filetype=cpp:

// Pin layout of the colorimeter:  {{{
// D10	Enable for main sensor (should always be on).
#define SENSOR_MAIN 10
// D2	Enable for orange sensor.
#define SENSOR_COD 2
// D3	Enable for white sensor.
#define SENSOR_NEPH 3

// D9	Enable for orange LED.
#define LED_COD 9
// D8	Enable for white LED.
#define LED_NEPH 8

// D4	Count from main sensor.
#define COUNT_MAIN 4
// D5	Count from reference sensors (only one must be enabled).
#define COUNT_REF 5

// A4 + A5: I2C pins; LCD shield connected to it.

// Enable pins are active by setting them to output low; inactive by setting
// them to input.  So all are set to low and toggling is done by changing
// the pin direction.
// }}}

// Includes. {{{
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <LcdMenu.h>
#include <DuplexFrequencyCounter.h>
// }}}

Adafruit_RGBLCDShield lcd;

// Hardware setup functions. {{{
void setCOD ()	// {{{
{
	pinMode (SENSOR_NEPH, INPUT);
	pinMode (LED_NEPH, INPUT);
	pinMode (SENSOR_COD, OUTPUT);
	pinMode (LED_COD, OUTPUT);
}	// }}}

void setNeph ()	// {{{
{
	pinMode (SENSOR_COD, INPUT);
	pinMode (LED_COD, INPUT);
	pinMode (SENSOR_NEPH, OUTPUT);
	pinMode (LED_NEPH, OUTPUT);
}	// }}}

void setDark ()	// {{{
{
	pinMode (SENSOR_NEPH, INPUT);
	pinMode (LED_NEPH, INPUT);
	pinMode (SENSOR_COD, INPUT);
	pinMode (LED_COD, INPUT);
}	// }}}
// }}}

// Arduino main functions. {{{
void setup () {	// {{{
	Serial.begin(9600);
       	lcd.begin (16, 2);
	pinMode (SENSOR_MAIN, OUTPUT);
	digitalWrite (SENSOR_MAIN, LOW);
	digitalWrite (SENSOR_COD, LOW);
	digitalWrite (SENSOR_NEPH, LOW);
	digitalWrite (LED_COD, LOW);
	digitalWrite (LED_NEPH, LOW);
	setDark ();
}	// }}}

void loop () {	// {{{
	uint32_t c0, c1;
	// Measure 10 seconds with orange light, then 10 seconds with white light.
	setCOD ();
	Serial.print ("COD\n");
	// Half a second per measurement, so 20 measurements.
	for (uint8_t i = 0; i < 20; ++i)
	{
		readFreq (500, c0, c1);
		Serial.print (c0);
		Serial.print ("\t");
		Serial.print (c1);
		Serial.print ("\n");
		lcd.clear ();
		lcd.print (c0);
		lcd.setCursor (0, 1);
		lcd.print (c1);
	}
	setNeph ();
	Serial.print ("Neph\n");
	// Half a second per measurement, so 20 measurements.
	for (uint8_t i = 0; i < 20; ++i)
	{
		readFreq (500, c0, c1);
		Serial.print (c0);
		Serial.print ("\t");
		Serial.print (c1);
		Serial.print ("\n");
		lcd.clear ();
		lcd.print (c0);
		lcd.setCursor (0, 1);
		lcd.print (c1);
		// Allow the serial port to catch up.
	}
	delay (100);
}	// }}}
// }}}
