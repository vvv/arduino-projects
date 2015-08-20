/*
 * 7-segment display
 *
 * 10 9 8 7 6
 * ,---------.
 * |  --a--  |
 * | |     | |   a  b  c  d  e  f  g  DP
 * | f     b |   7  6  4  2  1  9  10  5
 * | |     | |
 * |  --g--  |   7  6  4  3  2  8  9   5  <---  Arduino pins
 * | |     | |
 * | e     c |
 * | |     | |  Bit map:
 * |  --d-- .|  DP  g  f  e  d  c  b  a
 * |       DP|
 * `---------'
 *  1 2 3 4 5
 */

static int pins[] = { 7, 6, 4, 3, 2, 8, 9, 5 };

static char digits[] = {
	//          .gfe dcba
	0x3f, // 0  0011 1111
	0x06, // 1  0000 0110
	0x5b, // 2  0101 1011
	0x4f, // 3  0100 1111
	0x66, // 4  0110 0110
	0x6d, // 5  0110 1101
	0x7d, // 6  0111 1101
	0x07, // 7  0000 0111
	0x7f, // 8  0111 1111
	0x6f, // 9  0110 1111
	0x77, // a  0111 0111
	0x7c, // b  0111 1100
	0x39, // c  0011 1001
	0x5e, // d  0101 1110
	0x79, // e  0111 1001
	0x71  // f  0111 0001
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

/*
 * This code is written for a 7-segment display with common anode.
 * For common cathode replace HIGH with LOW and vice versa.
 */

static void leds_set(char mask)
{
	for (int i = 0; i < ARRAY_SIZE(pins); ++i)
		digitalWrite(pins[i], (mask & 1 << i) ? LOW : HIGH);
}

void setup() {
	for (int i = 0; i < ARRAY_SIZE(pins); ++i) {
		pinMode(pins[i], OUTPUT);
		digitalWrite(pins[i], HIGH);
	}
}

void loop() {
	static char i;
	static bool dot;

	leds_set(digits[i] | dot << 7);
	delay(1000);
	if (++i == ARRAY_SIZE(digits)) {
		i = 0;
		dot = !dot;
	}
}
