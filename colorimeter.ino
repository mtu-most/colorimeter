// vim: set foldmethod=marker filetype=cpp:

// Pin layout of the colorimeter:  {{{
// D10	Enable for main sensor (should always be on). (yellow)
#define SENSOR_MAIN 10
// D11	Power for sensors (should always be high). (brown)
//	Power for LEDs (should always be high). (red)
#define POWER 11
// D2	Enable for orange sensor. (purple)
#define SENSOR_620 2
// D3	Enable for IR sensor. (green)
#define SENSOR_IR 3

// D9	Enable for orange LED. (grey)
#define LED_620 9
// D8	Enable for IR LED. (blue)
#define LED_IR 8

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
	SOURCE_IR,
	SOURCE_620
}; // }}}

// Includes. {{{
#define SERIAL_LCD	// Output lcd display on serial port.

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
static uint32_t sensor, ref;	// Make these global for debugging.

// Nephalometry calibration values.
struct Neph_cal {	// {{{
	int num;
	float x;
	float y;
	float xy;
	float x2;
	float y2;
	float a;
	float b;
};	// }}}

// The thing below is in eeprom, so the bytes must be directly addressable.
typedef struct
{
	float cod;
	Neph_cal neph;
} cal_t;
static union {
	cal_t c;
	char bytes[sizeof(cal_t)];
} cal;
// }}}

void writeCal()	// {{{
{
	for (uint8_t i = 0; i < sizeof(cal); ++i)
		EEPROM.write(i, cal.bytes[i]);
}	// }}}

void setSource(uint8_t source)	// {{{
{
	pinMode(SENSOR_IR, INPUT);
	pinMode(SENSOR_620, INPUT);
	pinMode(LED_IR, INPUT);
	pinMode(LED_620, INPUT);

	switch (source)
	{
	case SOURCE_NONE:
		break;
	case SOURCE_IR:
		pinMode(SENSOR_IR, OUTPUT);
		pinMode(LED_IR, OUTPUT);
		break;
	case SOURCE_620:
		pinMode(SENSOR_620, OUTPUT);
		pinMode(LED_620, OUTPUT);
		break;
	}
	newsource = millis();
}	// }}}

void waitForSource()	// {{{
{
	uint32_t wait = millis() - newsource;
	if (wait < WARMUP_TIME)
	{
		lcd.clear();
		message("Source warms up", "Please wait");
		delay(WARMUP_TIME - wait);
	}
}	// }}}

float getSensor()	// {{{
{
	readFreq(measure_time, sensor, ref);
	return (float)sensor / ref;
}	// }}}

uint8_t checkButton(bool block) { // {{{
	while (true) {
		uint8_t ret = lcd.readButtons();
		if (ret != BUTTON_NONE || !block)
			return ret;
	}
} // }}}

// Menus, including implementation of the functions.  {{{
MenuItem(COD_zero)	// {{{
{
	waitForSource();
	cod_zero_value = getSensor();
	return 1;
}	// }}}

MenuItem(COD_measure)	// {{{
{
	static uint32_t index = 0;
	// Don't measure without zero.
	if (cod_zero_value == ~0)
	{
		message("Please zero the", "instrument first");
		checkButton(true);
		return -1;
	}
	waitForSource();
	message("Measuring");
	float data = getSensor();
	// TODO: compute concentration.
	float trans = data / cod_zero_value;
	float absorb = -log10(trans);
	lcd.clear();
	lcd.print("T:");
	lcd.print(trans, 3);
	lcd.setCursor(8, 0);
	lcd.print("A:");
	lcd.print(absorb, 3);
	lcd.setCursor(0, 1);
	lcd.print(index);
	lcd.print(" ");
	unsigned long m = millis();
	lcd.print(m);
	Serial.print(absorb, 3);
	Serial.print("\t");
	Serial.print(index++);
	Serial.print("\t");
	Serial.print(m);
	Serial.print("\n");
	checkButton(true);
	return 0;
}	// }}}

MenuItem(COD_calibrate)	// {{{
{
	waitForSource();
	return -1;
}	// }}}

Menu <3> COD_Menu("COD", (char const *[]){"Zero", "Measure", "Calibrate"}, (action *[]){&COD_zero, &COD_measure, &COD_calibrate});

MenuItem(doCOD)	// {{{
{
	setSource(SOURCE_620);
	bool ret = COD_Menu.run(true);
	setSource(SOURCE_NONE);
	return ret;
}	// }}}

MenuItem(Neph_measure)	// {{{
{
	static uint32_t index = 0;
	waitForSource();
	message("Measuring");
	float data = getSensor();
	if (isnan(cal.c.neph.a))
	{
		lcd.clear();
		lcd.print("Not calibrated");
		lcd.setCursor(0, 1);
		lcd.print("Read: ");
		int power = 0;
		while (data < 1) {
			power -= 1;
			data *= 10;
		}
		while (data >= 10) {
			power += 1;
			data /= 10;
		}
		lcd.print(data);
		lcd.print("e");
		lcd.print(power);
	}
	else
	{
		float turb = cal.c.neph.a * data + cal.c.neph.b;
		lcd.clear();
		lcd.print(turb, 3);
		lcd.print(" NTU");
		lcd.setCursor(0, 1);
		lcd.print("index: ");
		lcd.print(index);
		Serial.print(index++);
		Serial.print("\t");
		Serial.print(turb);
		Serial.print("\t");
		Serial.print(millis());
		Serial.print("\n");
	}
	checkButton(true);
	return 0;
}	// }}}

MenuItem(Neph_recalibrate)	// {{{
{
	cal.c.neph.a = NAN;
	cal.c.neph.b = NAN;
	cal.c.neph.num = 0;
	cal.c.neph.x = 0;
	cal.c.neph.y = 0;
	cal.c.neph.xy = 0;
	cal.c.neph.x2 = 0;
	cal.c.neph.y2 = 0;
	writeCal();
	message("Calibration", "erased.");
	checkButton(true);
	return 1;
}	// }}}

MenuItem(Neph_calibrate)	// {{{
{
	message("Calibration:", "0000.00 NTU     ");
	cNTU = readNum(0, 1, 4, 2, cNTU);
	waitForSource();
	message("Calibrating");
	float data = getSensor();
	cal.c.neph.x += data;
	cal.c.neph.y += cNTU / 100.;
	cal.c.neph.xy += data * (cNTU / 100.);
	cal.c.neph.x2 += data * data;
	cal.c.neph.y2 += (cNTU / 100.) * (cNTU / 100.);
	++cal.c.neph.num;
	if (cal.c.neph.num >= 4 && cal.c.neph.y2 != cal.c.neph.y * cal.c.neph.y)
	{
		cal.c.neph.a = (cal.c.neph.xy - cal.c.neph.x * cal.c.neph.y / cal.c.neph.num) / (cal.c.neph.x2 - cal.c.neph.x * cal.c.neph.x / cal.c.neph.num);
		cal.c.neph.b = (cal.c.neph.y - cal.c.neph.a * cal.c.neph.x) / cal.c.neph.num;
		writeCal();
		message("Calibration read");
		lcd.setCursor(0, 1);
		lcd.print("Error: ");
		lcd.print((cal.c.neph.y2 + cal.c.neph.a * cal.c.neph.a * cal.c.neph.x2 - 2 * cal.c.neph.a * cal.c.neph.xy - 2 * cal.c.neph.b * cal.c.neph.y + 2 * cal.c.neph.a * cal.c.neph.b + cal.c.neph.x) / cal.c.neph.num + cal.c.neph.b * cal.c.neph.b);
	}
	else
		message("Calibration read", "More needed");
	checkButton(true);
	return 0;
}	// }}}

Menu <3> Neph_Menu("Nephalometry", (char const *[]){"Measure", "Recalibrate", "Calibrate"}, (action *[]){&Neph_measure, &Neph_recalibrate, &Neph_calibrate});

MenuItem(doNeph)	// {{{
{
	setSource(SOURCE_IR);
	bool ret = Neph_Menu.run(true);
	setSource(SOURCE_NONE);
	return ret;
}	// }}}

void debug_newsource(uint8_t s) // {{{
{
	setSource(s);
	lcd.setCursor(0, 1);
	lcd.print("Source: ");
	switch (s)
	{
	case SOURCE_NONE:
		lcd.print("None ");
		break;
	case SOURCE_620:
		lcd.print("620  ");
		break;
	case SOURCE_IR:
		lcd.print("IR");
		break;
	}
}	// }}}

MenuItem(debug)	// {{{
{
	measure_time = 100;
	lcd.print("Time: 0100 ms");
	uint8_t s = SOURCE_NONE;
	while (lcd.readButtons() != 0) {}
	debug_newsource(s);
	while (true)
	{
		uint8_t b = lcd.readButtons();
		switch (b)
		{
		case 0:
			break;
		case BUTTON_UP:
			s = (s + 1) % 3;
			debug_newsource(s);
			while (lcd.readButtons() != 0) {}
			break;
		case BUTTON_DOWN:
			s = (s - 1 + 3) % 3;
			debug_newsource(s);;
			while (lcd.readButtons() != 0) {}
			break;
		case BUTTON_LEFT:
			setSource(SOURCE_NONE);
			return 0;
		case BUTTON_RIGHT:
		case BUTTON_SELECT:
			measure_time = readNum(6, 0, 4, 0, measure_time);
			while (lcd.readButtons() != 0) {}
		}
		// Let the serial port catch up.
		while (~UCSR0A & (1 << 5)) {}
		// Send data to the port.
		unsigned long m = millis();
		float data = getSensor();
		Serial.print(m);
		Serial.print("\t");
		Serial.print(data * 1e6);
		Serial.print("\t");
		Serial.print(sensor);
		Serial.print("\t");
		Serial.print(ref);
		Serial.print("\t");
		Serial.print(s);
		Serial.print("\t");
		Serial.print(measure_time);
		Serial.print("\n");
	}
}	// }}}

Menu <3> MainMenu("Main menu", (char const *[]){"COD", "Nephalometry", "Debug"}, (action *[]){&doCOD, &doNeph, &debug});
// }}}

// Arduino main functions. {{{
void setup() {	// {{{
	Serial.begin(9600);
	lcd.begin(16, 2);
	pinMode(SENSOR_MAIN, OUTPUT);
	pinMode(POWER, OUTPUT);
	digitalWrite(SENSOR_MAIN, LOW);
	digitalWrite(POWER, HIGH);
	digitalWrite(SENSOR_620, LOW);
	digitalWrite(SENSOR_IR, LOW);
	digitalWrite(LED_620, LOW);
	digitalWrite(LED_IR, LOW);
	setSource(SOURCE_NONE);
	for (uint8_t i = 0; i < sizeof(cal); ++i)
		cal.bytes[i] = EEPROM.read(i);
}	// }}}

void loop() {	// {{{
	MainMenu.run(true);
}	// }}}
// }}}
