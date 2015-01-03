#include <stdint.h>
#include "ds2483.h"
#ifndef DS18B2O_H
#define DS18B2O_H
int8_t ds18b20_start_conversion(ds2483_dev_t *);
int8_t ds18b20_read_temp(ds2483_dev_t *, int16_t*);
#endif
