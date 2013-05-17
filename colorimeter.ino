// vim: set foldmethod=marker filetype=cpp:

// Pin layout of the colorimeter:  {{{
// D10	Enable for main sensor (should always be on).
#define SENSOR_MAIN 10
// D2	Enable for orange sensor.
#define SENSOR_ORANGE 2
// D3	Enable for white sensor.
#define SENSOR_WHITE 3

// D9	Enable for orange LED.
#define LED_ORANGE 9
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

// Includes. {{{
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <LcdMenu.h>
// }}}

Adafruit_RGBLCDShield lcd;

void readFreq (uint32_t ms, uint32_t &result0, uint32_t &result1)	// {{{
// This function reads the frequency of the signal on pins 4 (timer 0 input t0) and 5 (timer 1 input t1).
// It waits for 500 ms, then returns the number of counts times 2.
{
	uint16_t counter = (16000000 >> 8) * ms / 1000;	// 16000000 Hz, 256 cycles per overflow: 31250 overflows for 500 ms.
	uint8_t tmpreg1 (0), tmpreg2 (0), low_result0;
	uint16_t overflow0 (0), overflow1 (0), low_result1;
	// use timer2 for keeping time, timer0 and timer1 for counting.
	asm volatile (
		// Disable interrupts.
		"cli\n"
		// Store timer0 settings, so it can be restored.
	"	in %[tccr0a], %[TCCR0A_]\n"
	"	in %[tccr0b], %[TCCR0B_]\n"
		// Set timer0 to count pulses from t0.
	"	out %[TCCR0A_], __zero_reg__\n"
	"	out %[TCCR0B_], %[_7]\n"	// 6 for falling edge, 7 for rising edge.
		// Set timer1 to count pulses from t1.
	"	std %a[counter_base] + %[TCCR1A_], __zero_reg__\n"
	"	std %a[counter_base] + %[TCCR1B_], %[_7]\n"	// 6 for falling edge, 7 for rising edge.
		// Set timer2 to count the system clock.
	"	std %a[counter_base] + %[ASSR_], __zero_reg__\n"
	"	std %a[counter_base] + %[TCCR2A_], __zero_reg__\n"
	"	std %a[counter_base] + %[TCCR2B_], %[_1]\n"
		// Wait a constant time.
	"	std %a[counter_base] + %[TCNT2_], %[_2]\n"					// -6
		// Set timer1 to 0.
	"	std %a[counter_base] + %[TCNT1H_], __zero_reg__\n"	// Set timer0 to 0.	// -4
	"	std %a[counter_base] + %[TCNT1L_], __zero_reg__\n"	// Set timer0 to 0.	// -2
		// Set timer0 to 0.
	"	out %[TCNT0_], __zero_reg__\n"							// 0
		// Clear overflow flags.
	"	out %[TIFR0_], %[_1]\n"								// 1
	"	out %[TIFR1_], %[_1]\n"								// 2
	"	out %[TIFR2_], %[_1]\n"								// 3
	"countloop:\n"
		// If an overflow happened, record and clear it.
	"	sbis %[TIFR0_], 0\n"
	"	rjmp no_overflow0\n"
	"	adiw %[overflow0], 1\n"
	"	out %[TIFR0_], %[_1]\n"	// Clear overflow flag.
	"no_overflow0:\n"
	"	sbis %[TIFR1_], 0\n"
	"	rjmp no_overflow1\n"
	"	adiw %[overflow1], 1\n"
	"	out %[TIFR1_], %[_1]\n"	// Clear overflow flag.
	"no_overflow1:\n"
	"wait_for_overflow:\n"
	"	sbis %[TIFR2_], 0\n"
	"	rjmp wait_for_overflow\n"							// i * 0x100 - 6 - x + 2
	"	out %[TIFR2_], %[_1]\n"	// Clear timer 2 overflow flag.				
		// End loop.
	"	sbiw %[counter], 1\n"								// i * 0x100 - 6 - x + 4
	"	brne countloop\n"								// N * 0x100 - 6 - x + 5
		// Stop timers 1 and 0. (use this order so they record an equal time.)
	"	std %a[counter_base] + %[TCCR1B_], __zero_reg__\n"				// N * 0x100 - 6 - x + 7
	"	out %[TCCR0B_], __zero_reg__\n"							// N * 0x100 - 6 - x + 8
		// Ok, so the timers have run for N * 0x100 - 6 - x + 8 cycles.
		// This must be equal to N * 0x100, so -6 - x + 8 must be 0,
		// in other words, x must be 8 - 6 = 2.

		// Handle one last overflows.
	"	sbic %[TIFR1_], 0\n"
	"	adiw %[overflow1], 1\n"
	"	sbic %[TIFR0_], 0\n"
	"	adiw %[overflow0], 1\n"
		// Read value of timer 1.
	"	ldd %A[result1], %a[counter_base] + %[TCNT1L_]\n"
	"	ldd %B[result1], %a[counter_base] + %[TCNT1H_]\n"
		// Read value of timer 0.
	"	in %[result0], %[TCNT0_]\n"
		// Restore timer 0 values.
	"	out %[TCCR0A_], %[tccr0a]\n"
	"	out %[TCCR0B_], %[tccr0b]\n"
		// Enable interrupts again.
	"	sei"
	:
		[counter] "+w" (counter),
		[overflow0] "+w" (overflow0),
		[overflow1] "+w" (overflow1),
		[tccr0a] "+r" (tmpreg1),
		[tccr0b] "+r" (tmpreg2),
		[result0] "=r" (low_result0),
		[result1] "=r" (low_result1)
	:
		[_1] "r" ((uint8_t)1),
		[_2] "r" ((uint8_t)2),
		[_7] "r" ((uint8_t)7),
		[TCCR0A_] "I" (_SFR_IO_ADDR(TCCR0A)),
		[TCCR0B_] "I" (_SFR_IO_ADDR(TCCR0B)),
		[TCNT0_] "I" (_SFR_IO_ADDR(TCNT0)),
		[TIFR0_] "I" (_SFR_IO_ADDR(TIFR0)),
		[TIFR1_] "I" (_SFR_IO_ADDR(TIFR1)),
		[TIFR2_] "I" (_SFR_IO_ADDR(TIFR2)),
		[TCCR1A_] "I" (_SFR_MEM_ADDR(TCCR1A) - 0x80), // not directly addressable
		[TCCR1B_] "I" (_SFR_MEM_ADDR(TCCR1B) - 0x80), // not directly addressable
		[TCNT1H_] "I" (_SFR_MEM_ADDR(TCNT1H) - 0x80), // not directly addressable
		[TCNT1L_] "I" (_SFR_MEM_ADDR(TCNT1L) - 0x80), // not directly addressable
		[TCCR2A_] "I" (_SFR_MEM_ADDR(TCCR2A) - 0x80), // not directly addressable
		[TCCR2B_] "I" (_SFR_MEM_ADDR(TCCR2B) - 0x80), // not directly addressable
		[TCNT2_] "I" (_SFR_MEM_ADDR(TCNT2) - 0x80), // not directly addressable
		[ASSR_] "I" (_SFR_MEM_ADDR(ASSR) - 0x80), // not directly addressable
		[counter_base] "b" (0x80)
	);
	result0 = ((uint32_t)overflow0 << 8 | (uint32_t)low_result0) * 1000 / ms;
	result1 = ((uint32_t)overflow1 << 16 | (uint32_t)low_result1) * 1000 / ms;
}	// }}}

void setOrange ()
{
	pinMode (SENSOR_WHITE, INPUT);
	pinMode (LED_WHITE, INPUT);
	pinMode (SENSOR_ORANGE, OUTPUT);
	pinMode (LED_ORANGE, OUTPUT);
}

void setWhite ()
{
	pinMode (SENSOR_ORANGE, INPUT);
	pinMode (LED_ORANGE, INPUT);
	pinMode (SENSOR_WHITE, OUTPUT);
	pinMode (LED_WHITE, OUTPUT);
}

void setDark ()
{
	pinMode (SENSOR_WHITE, INPUT);
	pinMode (LED_WHITE, INPUT);
	pinMode (SENSOR_ORANGE, INPUT);
	pinMode (LED_ORANGE, INPUT);
}

void setup () {
	Serial.begin(9600);
       	lcd.begin (16, 2);
	pinMode (SENSOR_MAIN, OUTPUT);
	digitalWrite (SENSOR_MAIN, LOW);
	digitalWrite (SENSOR_ORANGE, LOW);
	digitalWrite (SENSOR_WHITE, LOW);
	digitalWrite (LED_ORANGE, LOW);
	digitalWrite (LED_WHITE, LOW);
	setDark ();
}

void loop () {
	uint32_t c0, c1;
	// Measure 10 seconds with orange light, then 10 seconds with white light.
	setOrange ();
	Serial.print ("Orange\n");
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
	setWhite ();
	Serial.print ("White\n");
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
}
