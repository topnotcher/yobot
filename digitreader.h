#include <stdint.h>

#ifndef DIGITREADER_H
#define DIGITREADER_H
void digitreader_init(uint8_t max_digits, void (*print)(uint8_t *digits, uint8_t max_digits));
void digitreader_handle_digit(uint8_t digit);
uint8_t *digitreader_get(uint8_t *size);
#endif
