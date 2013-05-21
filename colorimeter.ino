// vim: set foldmethod=marker filetype=cpp:

// Pin layout of the colorimeter:  {{{
// D10	Enable for main sensor (should always be on).
#define SENSOR_MAIN 10
// D2	Enable for orange sensor.
#define SENSOR_620 2
// D3	Enable for white sensor.
#define SENSOR_WHITE 3

// D9	Enable for orange LED.
#define LED_620 9
// D8	Enable for white LED.
#define LED_WHITE 8

// D4	Count from main sensor.
#define COUNT_MAIN 4
// D5	Count from reference sensors (only one must be enabled).
#define COUNT_REF 5

// A4 + A5: I2C pins; LCD shield connected to it.

// Enable pins are active by setting them to output low; inactive by setting
// them to input.  So all are set to low and toggling is done by changing
// the pin direction.
// }}}

enum SourceNames {	// {{{
	SOURCE_NONE,
	SOURCE_WHITE,
	SOURCE_620
}; // }}}

// Includes. {{{
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <LcdMenu.h>
#include <DuplexFrequencyCounter.h>
// }}}

// Globals.  {{{
Adafruit_RGBLCDShield lcd;
uint32_t newsource = 0;
float cod_zero_value = 0;
// }}}

void setSource (uint8_t source)	// {{{
{
	pinMode (SENSOR_WHITE, INPUT);
	pinMode (SENSOR_620, INPUT);
	pinMode (LED_WHITE, INPUT);
	pinMode (LED_620, INPUT);

	switch (source)
	{
	case SOURCE_NONE:
		break;
	case SOURCE_WHITE:
		pinMode (SENSOR_WHITE, OUTPUT);
		pinMode (LED_WHITE, OUTPUT);
		break;
	case SOURCE_620:
		pinMode (SENSOR_620, OUTPUT);
		pinMode (LED_620, OUTPUT);
		break;
	}
	newsource = millis ();
}	// }}}

void waitForSource ()
{
	uint32_t wait = millis () - newsource;
	if (wait < 10000)
	{
		lcd.clear ();
		message ("Source warms up", "Please wait");
		delay (10000 - wait);
	}
}

float getSensor ()	// {{{
{
	uint32_t sensor, ref;
	readFreq (500, sensor, ref);
	return (float)sensor / ref;
}	// }}}

MenuItem (COD_zero)	// {{{
{
	waitForSource ();
	cod_zero_value = getSensor ();
	return true;
}	// }}}

MenuItem (COD_measure)	// {{{
{
	waitForSource ();
	message ("Measuring");
	float data = getSensor ();
	// TODO: compute stuff.
	message ("TODO:COD measure");
	lcd.setCursor (0, 1);
	lcd.print (data - cod_zero_value);
	waitForButton ();
	return false;
}	// }}}

MenuItem (COD_calibrate)	// {{{
{
	waitForSource ();
	return false;
}	// }}}

Menu <3> COD_Menu ("COD", (char *[]){"Zero", "Measure", "Calibrate"}, (action *[]){&COD_zero, &COD_measure, &COD_calibrate});

MenuItem (doCOD)	// {{{
{
	setSource (SOURCE_620);
	return COD_Menu.run ();
}	// }}}

MenuItem (Neph_zero)	// {{{
{
	waitForSource ();
	return true;
}	// }}}

MenuItem (Neph_measure)	// {{{
{
	waitForSource ();
	return false;
}	// }}}

MenuItem (Neph_calibrate)	// {{{
{
	waitForSource ();
	return false;
}	// }}}

Menu <3> Neph_Menu ("COD", (char *[]){"Zero", "Measure", "Calibrate"}, (action *[]){&COD_zero, &COD_measure, &COD_calibrate});

MenuItem (doNeph)	// {{{
{
	setSource (SOURCE_WHITE);
	return Neph_Menu.run ();
}	// }}}

Menu <2> MainMenu ("Main menu", (char *[]){"COD", "Neph"}, (action *[]){&doCOD, &doNeph});

// Arduino main functions. {{{
void setup () {	// {{{
	Serial.begin(9600);
       	lcd.begin (16, 2);
	pinMode (SENSOR_MAIN, OUTPUT);
	digitalWrite (SENSOR_MAIN, LOW);
	digitalWrite (SENSOR_620, LOW);
	digitalWrite (SENSOR_WHITE, LOW);
	digitalWrite (LED_620, LOW);
	digitalWrite (LED_WHITE, LOW);
	setSource (SOURCE_NONE);
}	// }}}

void loop () {	// {{{
	MainMenu.run ();
}	// }}}
// }}}
