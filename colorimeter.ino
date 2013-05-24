// vim: set foldmethod=marker filetype=cpp:

// Pin layout of the colorimeter:  {{{
// D10	Enable for main sensor (should always be on). (yellow)
#define SENSOR_MAIN 10
// D11	Power for sensors (should always be high). (brown)
//	Power for LEDs (should always be high). (red)
#define POWER 11
// D2	Enable for orange sensor. (purple)
#define SENSOR_620 2
// D3	Enable for white sensor. (green)
#define SENSOR_WHITE 3

// D9	Enable for orange LED. (grey)
#define LED_620 9
// D8	Enable for white LED. (blue)
#define LED_WHITE 8

// D4	Count from main sensor. (black)
#define COUNT_MAIN 4
// D5	Count from reference sensors (only one must be enabled). (orange)
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
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <LcdMenu.h>
#include <DuplexFrequencyCounter.h>
// }}}

// Globals.  {{{
#define WARMUP_TIME 1000	// For testing, 1 second warmup time will do.
static uint16_t measure_time = 1000;
Adafruit_RGBLCDShield lcd;
static uint32_t newsource = 0;
static uint32_t cNTU = 4000;	// centi-nephalometric turbidity units.
static float cod_zero_value = ~0;
static float neph_zero_value = ~0;
static uint32_t sensor, ref;	// Make these global for debugging.
// The thing below is in eeprom, so the bytes must be directly addressable.
typedef struct
{
	float cod;
	float neph;
} cal_t;
static union {
	cal_t c;
	char bytes[sizeof (cal_t)];
} cal;
// }}}

void writeCal ()	// {{{
{
	for (uint8_t i = 0; i < sizeof (cal); ++i)
		EEPROM.write (i, cal.bytes[i]);
}	// }}}

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

void waitForSource ()	// {{{
{
	uint32_t wait = millis () - newsource;
	if (wait < WARMUP_TIME)
	{
		lcd.clear ();
		message ("Source warms up", "Please wait");
		delay (WARMUP_TIME - wait);
	}
}	// }}}

float getSensor ()	// {{{
{
	readFreq (measure_time, sensor, ref);
	return (float)sensor / ref;
}	// }}}

// Menus, including implementation of the functions.  {{{
MenuItem (COD_zero)	// {{{
{
	waitForSource ();
	cod_zero_value = getSensor ();
	return 1;
}	// }}}

MenuItem (COD_measure)	// {{{
{
	static uint32_t index = 0;
	// Don't measure without zero.
	if (cod_zero_value == ~0)
	{
		message ("Please zero the", "instrument first");
		waitForButton ();
		return -1;
	}
	waitForSource ();
	message ("Measuring");
	float data = getSensor ();
	// TODO: compute concentration.
	float trans = data / cod_zero_value;
	float absorb = -log10 (trans);
	lcd.clear ();
	lcd.print ("T:");
	lcd.print (trans, 3);
	lcd.setCursor (8, 0);
	lcd.print ("A:");
	lcd.print (absorb, 3);
	lcd.setCursor (0, 1);
	lcd.print (index);
	lcd.print (" ");
	unsigned long m = millis ();
	lcd.print (m);
	Serial.print (absorb, 3);
	Serial.print ("\t");
	Serial.print (index++);
	Serial.print ("\t");
	Serial.print (m);
	Serial.print ("\n");
	waitForButton ();
	return 0;
}	// }}}

MenuItem (COD_calibrate)	// {{{
{
	waitForSource ();
	return -1;
}	// }}}

Menu <3> COD_Menu ("COD", (char const *[]){"Zero", "Measure", "Calibrate"}, (action *[]){&COD_zero, &COD_measure, &COD_calibrate});

MenuItem (doCOD)	// {{{
{
	setSource (SOURCE_620);
	bool ret = COD_Menu.run ();
	setSource (SOURCE_NONE);
	return ret;
}	// }}}

MenuItem (Neph_zero)	// {{{
{
	waitForSource ();
	neph_zero_value = getSensor ();
	return 1;
}	// }}}

MenuItem (Neph_measure)	// {{{
{
	static uint32_t index = 0;
	// Don't measure without zero.
	if (neph_zero_value == ~0)
	{
		message ("Please zero the", "instrument first");
		waitForButton ();
		return -1;
	}
	waitForSource ();
	message ("Measuring");
	float data = getSensor () - neph_zero_value;
	float turb = data / cal.c.neph;
	lcd.clear ();
	lcd.print (turb, 3);
	lcd.print (" NTU");
	lcd.setCursor (0, 1);
	lcd.print (index);
	Serial.print (data, 3);
	Serial.print ("\t");
	Serial.print (turb, 3);
	Serial.print ("\t");
	Serial.print (index++);
	Serial.print ("\t");
	Serial.print (millis ());
	Serial.print ("\n");
	waitForButton ();
	return 0;
}	// }}}

MenuItem (Neph_calibrate)	// {{{
{
	// Don't calibrate without zero.
	if (neph_zero_value == ~0)
	{
		message ("Please zero the", "instrument first");
		waitForButton ();
		return -2;
	}
	message ("Calibration:", "0000.00 NTU     ");
	cNTU = readNum (0, 1, 4, 2, cNTU);
	waitForSource ();
	message ("Calibrating");
	float data = getSensor () - neph_zero_value;
	cal.c.neph = data * 100 / cNTU;
	writeCal ();
	return -1;
}	// }}}

MenuItem (Neph_bulk)	// {{{
{
	while (lcd.readButtons () != 0) {}
	while (true)
	{
		if (lcd.readButtons () != 0)
			return 0;
		float data = getSensor ();
		Serial.print (sensor);
		Serial.print ("\t");
		Serial.print (ref);
		Serial.print ("\t");
		Serial.print (data * 1e6);
		Serial.print ("\n");
		while (~UCSR0A & (1 << 5)) {}
	}
}	// }}}

Menu <4> Neph_Menu ("Neph", (char const *[]){"Zero", "Measure", "Calibrate", "Bulk debug"}, (action *[]){&Neph_zero, &Neph_measure, &Neph_calibrate, &Neph_bulk});

MenuItem (doNeph)	// {{{
{
	setSource (SOURCE_WHITE);
	bool ret = Neph_Menu.run ();
	setSource (SOURCE_NONE);
	return ret;
}	// }}}

Menu <2> MainMenu ("Main menu", (char const *[]){"COD", "Neph"}, (action *[]){&doCOD, &doNeph});
// }}}

// Arduino main functions. {{{
void setup () {	// {{{
	Serial.begin(9600);
	lcd.begin (16, 2);
	pinMode (SENSOR_MAIN, OUTPUT);
	pinMode (POWER, OUTPUT);
	digitalWrite (SENSOR_MAIN, LOW);
	digitalWrite (POWER, HIGH);
	digitalWrite (SENSOR_620, LOW);
	digitalWrite (SENSOR_WHITE, LOW);
	digitalWrite (LED_620, LOW);
	digitalWrite (LED_WHITE, LOW);
	setSource (SOURCE_NONE);
	for (uint8_t i = 0; i < sizeof (cal); ++i)
		cal.bytes[i] = EEPROM.read (i);
}	// }}}

void loop () {	// {{{
	MainMenu.run ();
}	// }}}
// }}}
