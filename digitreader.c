#include <stdint.h>
#include "digitreader.h"

#define MAX_DIGITS 4
static struct {
	uint8_t digits[MAX_DIGITS];
	uint8_t max_digits;
	void (*print)(uint8_t *, uint8_t);
} reader;

void digitreader_init(uint8_t max_digits, void (*print)(uint8_t *digits, uint8_t max_digits)) {
	reader.max_digits = max_digits;
	reader.print = print;

	for (uint8_t i = 0; i < reader.max_digits; ++i)
		reader.digits[i] = 0;

	reader.print(reader.digits,reader.max_digits);
}

void digitreader_handle_digit(uint8_t digit) {
	for (uint8_t i = reader.max_digits-1; i > 0; --i)
		reader.digits[i] = reader.digits[i-1];

	reader.digits[0] = digit;
	reader.print(reader.digits,reader.max_digits);
}

uint8_t *digitreader_get(uint8_t *size) {
	*size = reader.max_digits;
	return reader.digits;
}
