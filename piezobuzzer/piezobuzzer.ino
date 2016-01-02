/* -*- c -*- */
/*
 * Arduino Mario Bros Tunes
 * With Piezo Buzzer and PWM
 *
 * Connect the positive side of the Buzzer to pin 3,
 * then the negative side to a 1k ohm resistor. Connect
 * the other side of the 1 k ohm resistor to
 * ground(GND) pin on the Arduino.
 *
 * by: Dipto Pratyaksa
 * last updated: 31/3/13
 *
 * refactoring: Valery V. Vorotyntsev
 * 2015-12-31
 *
 * References:
 *   1. http://www.princetronics.com/supermariothemesong/
 *   2. http://www.linuxcircle.com/2013/03/31/playing-mario-bros-tune-with-arduino-and-piezo-buzzer/
 */

#include "notes.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

enum {
	PIN_BUZZER = 3,
	PIN_LED    = 13,
};

struct Note {
	int frequency;
	int relative_duration;
};

static struct Note mario_main_theme[] = {
	{ NOTE_E7, 12 }, { NOTE_E7, 12 }, { 0, 12 },       { NOTE_E7, 12 },
	{ 0, 12 },       { NOTE_C7, 12 }, { NOTE_E7, 12 }, { 0, 12 },
	{ NOTE_G7, 12 }, { 0, 12 },       { 0, 12 },       { 0, 12 },
	{ NOTE_G6, 12 }, { 0, 12 },       { 0, 12 },       { 0, 12 },

	{ NOTE_C7, 12 }, { 0, 12 },        { 0, 12 },       { NOTE_G6, 12 },
	{ 0, 12 },       { 0, 12 },        { NOTE_E6, 12 }, { 0, 12 },
	{ 0, 12 },       { NOTE_A6, 12 },  { 0, 12 },       { NOTE_B6, 12 },
	{ 0, 12 },       { NOTE_AS6, 12 }, { NOTE_A6, 12 }, { 0, 12 },

	{ NOTE_G6, 9 },  { NOTE_E7, 9 },  { NOTE_G7, 9 },
	{ NOTE_A7, 12 }, { 0, 12 },       { NOTE_F7, 12 }, { NOTE_G7, 12 },
	{ 0, 12 },       { NOTE_E7, 12 }, { 0, 12 },       { NOTE_C7, 12 },
	{ NOTE_D7, 12 }, { NOTE_B6, 12 }, { 0, 12 },       { 0, 12 },

	{ NOTE_C7, 12 }, { 0, 12 },        { 0, 12 },       { NOTE_G6, 12 },
	{ 0, 12 },       { 0, 12 },        { NOTE_E6, 12 }, { 0, 12 },
	{ 0, 12 },       { NOTE_A6, 12 },  { 0, 12 },       { NOTE_B6, 12 },
	{ 0, 12 },       { NOTE_AS6, 12 }, { NOTE_A6, 12 }, { 0, 12 },

	{ NOTE_G6, 9 },  { NOTE_E7, 9 },  { NOTE_G7, 9 },
	{ NOTE_A7, 12 }, { 0, 12 },       { NOTE_F7, 12 }, { NOTE_G7, 12 },
	{ 0, 12 },       { NOTE_E7, 12 }, { 0, 12 },       { NOTE_C7, 12 },
	{ NOTE_D7, 12 }, { NOTE_B6, 12 }, { 0, 12 },       { 0, 12 },
};

static struct Note underworld_melody[] = {
	{ NOTE_C4, 12 }, { NOTE_C5, 12 }, { NOTE_A3, 12 }, { NOTE_A4, 12 },
	{ NOTE_AS3, 12 }, { NOTE_AS4, 12 }, { 0, 6 },
	{ 0, 3 },
	{ NOTE_C4, 12 }, { NOTE_C5, 12 }, { NOTE_A3, 12 }, { NOTE_A4, 12 },
	{ NOTE_AS3, 12 }, { NOTE_AS4, 12 }, { 0, 6 },
	{ 0, 3 },
	{ NOTE_F3, 12 }, { NOTE_F4, 12 }, { NOTE_D3, 12 }, { NOTE_D4, 12 },
	{ NOTE_DS3, 12 }, { NOTE_DS4, 12 }, { 0, 6 },
	{ 0, 3 },
	{ NOTE_F3, 12 }, { NOTE_F4, 12 }, { NOTE_D3, 12 }, { NOTE_D4, 12 },
	{ NOTE_DS3, 12 }, { NOTE_DS4, 12 }, { 0, 6 },
	{ 0, 6 }, { NOTE_DS4, 18 }, { NOTE_CS4, 18 }, { NOTE_D4, 18 },
	{ NOTE_CS4, 6 }, { NOTE_DS4, 6 },
	{ NOTE_DS4, 6 }, { NOTE_GS3, 6 },
	{ NOTE_G3, 6 }, { NOTE_CS4, 6 },
	{ NOTE_C4, 18 }, { NOTE_FS4, 18 }, { NOTE_F4, 18 },
	{ NOTE_E3, 18 }, { NOTE_AS4, 18 }, { NOTE_A4, 18 },
	{ NOTE_GS4, 10 }, { NOTE_DS4, 10 }, { NOTE_B3, 10 },
	{ NOTE_AS3, 10 }, { NOTE_A3, 10 }, { NOTE_GS3, 10 },
	{ 0, 3 }, { 0, 3 }, { 0, 3 },
};

static void _play(const struct Note *tune, size_t tune_len, const char *name)
{
	if (name != NULL)
		Serial.println(name);
	for (size_t i = 0; i < tune_len; ++i) {
		int duration = 1000 / tune[i].relative_duration;
		buzz(tune[i].frequency, duration);
		delay(1.3 * duration); /* pause between notes */
		buzz(0, duration);
	}
}
#define play(tune) _play(tune, ARRAY_SIZE(tune), #tune)

/*
 * `frequency' is the number of cycles per second.
 * `duration' is measured in miliseconds.
 */
static void buzz(long frequency, long duration)
{
	/* XXX assert(frequency != 0) */
	digitalWrite(PIN_LED, HIGH);
	/*
	 * Delay between transitions =
	 * 1 second's worth of microseconds / frequency / 2,
	 * where 2 is number of phases (HIGH and LOW) per cycle.
	 */
	/* XXX Why doesn't the program crash when frequency == 0? */
	long delay = 1000000 / frequency / 2;
	long ncycles = frequency * duration / 1000;
	for (long i = 0; i < ncycles; i++) {
		digitalWrite(PIN_BUZZER, HIGH);
		delayMicroseconds(delay);
		digitalWrite(PIN_BUZZER, LOW);
		delayMicroseconds(delay);
	}
	digitalWrite(PIN_LED, LOW);
}

void setup(void)
{
	pinMode(PIN_BUZZER, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	Serial.begin(9600);
}

void loop()
{
	play(mario_main_theme);
	play(underworld_melody);
}
