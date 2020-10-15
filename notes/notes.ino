/* -*- c -*- */
/*
 * Arduino Tunes with piezo buzzer and PWM.
 *
 * Connect the positive side of the buzzer to pin 3, then the negative
 * side to a 300 Ohm resistor.
 * Connect the other side of the resistor to ground (GND) pin
 * on the Arduino.
 *
 * Based on the code by Dipto Pratyaksa.
 * Rewritten by Valery V. Vorotyntsev.
 *
 * TODO:
 *   - Reduce memory footprint by using bit fields for struct Note.
 *   - Use PROGMEM.
 *   - Get a second buzzer to play bass notes on.
 *
 * References:
 *   0. http://www.mariopiano.com/mario-sheet-music-overworld-main-theme.html
 *   1. http://www.princetronics.com/supermariothemesong/
 *   2. http://www.linuxcircle.com/2013/03/31/playing-mario-bros-tune-with-arduino-and-piezo-buzzer/
 */

#define __ASSERT_USE_STDERR
#include <assert.h> /* defines assert(); uses __assert() defined below */
#include "notes.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

enum {
	PIN_BUZZER = 3,
	PIN_LED    = 13,
};

enum Note_Type {
	NT_NOTE,
	NT_REST,
	NT_MM /**< metronome mark */
};

/** Default metronome mark, crotchets per minute. */
enum { DEFAULT_MM = 120 };

/** The duration of the whole note, miliseconds. */
static unsigned int whole_note;

struct Note {
	enum Note_Type n_type;
	/** See enum Note_Freq for values. */
	unsigned int n_frequency;
	/**
	 * Inverted note value.
	 *
	 * 1 - whole (semibreve),
	 * 2 - half (minim),
	 * 4 - quarter (crotchet),
	 * 8 - eighth (quaver),
	 * etc.
	 *
	 * If .n_type == NT_MM, then .n_value specifies the number
	 * of crotchets per minute.
	 */
	unsigned int n_value;
	/** Whether to "join" this note with the next one. */
	bool n_slur;
};

static bool note_invariant(const struct Note *note)
{
	return note != NULL &&
		(note->n_type >= NT_NOTE && note->n_type <= NT_MM) &&
		note->n_value != 0 &&
		/* only NOTEs can have .n_slur set */
		(!note->n_slur || note->n_type == NT_NOTE) &&
		/* only NOTEs can have non-zero .n_frequency */
		(note->n_frequency == 0 || note->n_type == NT_NOTE);
}

#define n(pitch, value)  { NT_NOTE, NOTE_ ## pitch, (value), false }
#define ns(pitch, value) { NT_NOTE, NOTE_ ## pitch, (value), true }
#define r(value)         { NT_REST, 0, (value), false }
#define mm(value)        { NT_MM, 0, (value), false }

/* https://en.wikipedia.org/wiki/Note_value */
static unsigned int dot(unsigned int inverted_value)
{
	/* Add 1 to compensate for integer division. */
	return (inverted_value + 1) * 2 / 3;
}

/* https://youtu.be/P5L6Qgmcjfw */
static const struct Note sherlock[] = {
	mm(200),
#define XXX_LEFTY 1
#if XXX_LEFTY
	n(C3,4), n(G3,4), n(G2,4), n(G3,4),
	n(C3,4), n(G3,4), n(G2,4), n(G3,4),
#else
	r(1),
	r(1),
#endif
	n(C4,4), n(G4,2), n(G4,4),
	n(FS4,8), n(G4,8), n(GS4,2), n(G4,4),
	n(F4,4), ns(C5,dot(2)),
	n(C5,1),

	n(F4,4), n(C5,2), n(C5,4),
	n(B4,8), n(C5,8), n(D5,2), n(C5,4),
	ns(DS5,1),
	n(DS5,1),
	n(G5,4), n(C5,2), n(C5,4),
	n(D5,4), n(DS5,4), n(D5,4), n(C5,4),

	n(F5,4), n(C5,4), r(2),
	r(2), r(4), n(C5,8), n(D5,8),
	n(DS5,4), n(DS5,8), n(F5,8), n(D5,4), n(D5,8), n(DS5,8),

	n(C5,4), n(C5,8), n(D5,8), n(B4,4), n(GS4,8), n(G4,8),
	n(C5,1),
};

#if 0 /* XXX -------------------------------------------------------- */
static struct Note mario_overworld[] = {
	mm(400), // XXX Not sure about this.
	n(E5,4), n(E5,4), r(4), n(E5,4), r(4), n(C5,4), n(E5,4), r(4),
	n(G5,4), r(4), r(4), r(4), n(G4,4), r(4), r(4), r(4),
	n(C5,4), r(4), r(4), n(G4,4), r(4), r(4), n(E4,4), r(4),
	r(4), n(A4,4), r(4), n(B4,4), r(4), n(AS4,4), n(A4,4), r(4),
	n(G4,3), n(E5,3), n(G5,3), n(A5,4), r(4), n(F5,4), n(G5,4),
	r(4), n(E5,4), r(4), n(C5,4), n(D5,4), n(B4,4), r(4), r(4),
	n(C5,4), r(4), r(4), n(G4,4), r(4), r(4), n(E4,4), r(4),
	r(4), n(A4,4), r(4), n(B4,4), r(4), n(AS4,4), n(A4,4), r(4),
	n(G4,3), n(E5,3), n(G5,3), n(A5,4), r(4), n(F5,4), n(G5,4),
	r(4), n(E5,4), r(4), n(C5,4), n(D5,4), n(B4,4), r(4), r(4)
};

static struct Note mario_underworld[] = {
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

static struct Note mario_death[] = {
	{ NOTE_C5, 24 }, { NOTE_CS5, 24 }, { NOTE_D5, 36 }, { 0, 12 }, { 0, 6 },
	{ NOTE_B4, 12 }, { NOTE_F5, 12 }, { 0, 12 }, { NOTE_F5, 12 },
	{ NOTE_F5, 9 }, { NOTE_E5, 9 }, { NOTE_D5, 9 },
	{ NOTE_C5, 12 }, { NOTE_E4, 12 }, { 0, 12 }, { NOTE_E4, 12 },
	{ NOTE_C4, 12 }, { 0, 12 }, { 0, 6 },
};

/* http://www.8notes.com/scores/1110.asp */
static struct Note happy_birthday[] = {
	n(C4,dot(8)), n(C4,16),
	n(D4,4), n(C4,4), n(F4,4),
	n(E4,2), n(C4,dot(8)), n(C4,16),
	n(D4,4), n(C4,4), n(G4,4),
	n(F4,2), n(C4,dot(8)), n(C4,16),

	n(C5,4), n(A4,4), n(F4,4),
	n(E4,4), n(D4,4), n(AS4,dot(8)), n(AS4,16),
	n(A4,4), n(F4,4), n(G4,4),
	n(F4,dot(2)),
};

/* https://youtu.be/xAMsLDQi2b0 */
static struct Note monkey_island_intro[] = {
	mm(120),
	ns(E4,16), ns(B4,16), ns(E5,16), ns(FS5,16), ns(FS5,dot(2)), n(FS5,1),
	ns(E6,16), ns(B5,16), ns(G5,16), ns(FS5,16), n(FS5,2), n(D5,4),
	n(E5,4), n(FS5,4), n(D5,4), n(E5,4),
	n(B4,2), ns(G3,4), ns(A3,4),

	ns(B3,1),
	ns(D4,2), ns(CS4,2),
	ns(D4,2), ns(E4,2),
	ns(F4,4), ns(G4,4), ns(A4,2),
	ns(B4,2), n(D5,2),
	mm(88),
	n(E3,16), ns(E4,16), ns(G4,16), n(E4,16), ns(B4,16), ns(G4,16), n(E4,8), n(D3,16), ns(D4,16), ns(F4,16), n(D4,16),
	n(E3,16), ns(E4,16), ns(G4,16), n(E4,16), ns(B4,16), ns(G4,16), n(E4,8), n(D3,16), ns(D4,16), ns(F4,16), n(D4,16), n(E3,16), ns(E4,16), ns(G4,16), n(E4,16),
	n(E5,8), ns(E5,16), ns(G5,16), ns(FS5,16), n(E5,16), n(D5,8), n(E5,4), r(4), n(D5,8),
	ns(D5,16), ns(C5,16), ns(B4,16), ns(D5,16), n(C5,dot(8)), n(C5,dot(8)), n(B4,4), r(4), n(E5,dot(8)),

	n(E5,dot(8)), ns(G5,16), ns(FS5,16), ns(E5,16), n(D5,8), ns(E5,4), n(E5,dot(8)), n(FS5,16),
	n(G5,dot(8)), n(G5,dot(8)), n(A5,4), n(FS5,dot(8)), ns(G5,16), ns(FS5,16), ns(E5,16), ns(D5,16), ns(FS5,16),
	n(G5,dot(8)), n(G5,dot(8)), n(FS5,4), n(E5,dot(8)), ns(G5,16), ns(FS5,16), ns(E5,16), ns(D5,16), ns(FS5,16),

	n(G5,dot(8)), n(G5,dot(8)), n(FS5,4), n(E5,dot(8)), ns(G5,16), ns(FS5,16), ns(E5,16), ns(D5,8),
	n(E5,dot(8)), n(E5,dot(8)), n(E5,dot(4)), n(E5,8), ns(D5,16), ns(C5,16), ns(B4,16), ns(D5,16), n(C5,dot(8)), n(C5,dot(8)),
	n(B4,4), r(4), r(4),
	r(4), r(4), n(B4,8), n(D5,8),

	ns(C6,8), n(G5,8), n(G5,4), ns(C6,16), ns(B5,16), ns(A5,16), ns(C6,16),
	ns(B5,8), n(G5,8), n(G5,dot(4)), n(B4,4), n(G5,8),
	ns(A5,8), n(D5,8), n(D5,dot(8)), ns(A5,16), ns(B5,16), ns(A5,16), ns(G5,16), n(A5,16),

	ns(B5,8), n(G5,dot(8)), ns(G5,8), n(E5,dot(8)), n(E5,4), n(F4,8), n(G4,8),
	ns(A4,16), ns(F4,16), ns(C5,16), ns(A4,16), ns(F5,16), ns(C5,16), ns(A5,16), ns(F5,16), ns(C5,16), ns(A4,16), ns(F5,16), ns(C5,16), ns(A5,16), ns(F5,16), ns(C6,16), n(F6,16),
	ns(G5,8), n(E5,dot(8)), n(E5,dot(8)), n(G5,16), n(FS5,dot(8)), n(G5,dot(8)), n(FS5,16), n(G5,16), n(E5,16), n(G5,16),

	r(16), ns(C4,16), ns(G4,16), ns(DS4,16), ns(C5,16), ns(G4,16), ns(C5,16), ns(DS5,16), r(16), ns(G4,16), ns(E5,16), ns(C5,16), ns(G5,16), ns(E5,16), ns(G5,16), n(C6,16),
	r(8), n(FS5,dot(8)), n(FS5,dot(8)), n(FS5,16), n(E5,dot(8)), n(FS5,dot(8)), n(E5,16), n(FS5,16), n(D5,8),
	n(E5,dot(8)), ns(E5,16), ns(G5,16), ns(FS5,16), ns(E5,16), n(D5,dot(8)), n(E5,4), r(8), n(D5,8),
	/* XXX WIP */
};
#endif /* XXX ------------------------------------------------------- */

#undef mm
#undef r
#undef s
#undef n

static void _play(const struct Note *score, size_t score_len, const char *name)
{
	const struct Note *note;
	unsigned int duration;

	whole_note = 240000 / DEFAULT_MM;
	if (name != NULL)
		Serial.println(name);
	for (size_t i = 0; i < score_len; ++i) {
		note = &score[i];
		assert(note_invariant(note));
		if (note->n_type == NT_MM) {
			/* MM * t/4 = 60s ==> t = 240s / MM */
			whole_note = 240000 / note->n_value;
			continue;
		}
		duration = whole_note / note->n_value;
		if (note->n_type == NT_REST) {
			delay(duration);
			continue;
		}
		assert(note->n_type == NT_NOTE);
		if (note->n_slur) {
			buzz(note->n_frequency, duration);
		} else {
			buzz(note->n_frequency, 0.7 * duration);
			delay(0.3 * duration); /* pause between notes */
		}
	}
}
#define play(score) _play(score, ARRAY_SIZE(score), #score)

/*
 * `frequency' is the number of cycles per second.
 * `duration' is measured in miliseconds.
 */
static void buzz(long frequency, long duration)
{
	assert(frequency != 0);
	digitalWrite(PIN_LED, HIGH);
	/*
	 * Delay between transitions =
	 * 1 second's worth of microseconds / frequency / 2,
	 * where 2 is number of phases (HIGH and LOW) per cycle.
	 */
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

/** Handle diagnostic informations given by assertion and abort program. */
void __assert(const char *__func, const char *__file, int __lineno,
	      const char *__sexp)
{
	Serial.print(__file);
	Serial.print(':');
	Serial.print(__lineno, DEC);
	Serial.print(" (");
	Serial.print(__func);
	Serial.print("): ");
	Serial.println(__sexp);
	Serial.flush();
	abort();
}

void setup(void)
{
	pinMode(PIN_BUZZER, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	Serial.begin(9600);
}

void loop()
{
	/* play(mario_overworld); */
	/* play(mario_underworld); */
	/* play(mario_death); */
	play(sherlock);
	/* play(happy_birthday); */
	/* play(monkey_island_intro); */
}
