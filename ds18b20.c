#include "ds18b20.h"
#include "ds2483.h"
#include "error.h"

//applicable when there is only ONE one wire device on the bus!
#define DS18B2O_CMD_SKIP_ROM 0xCC
#define DS18B2O_CMD_READ_SCRATCHPAD 0xBE
#define DS18B2O_CMD_CONVERT_T 0x44

int8_t ds18b20_start_conversion(ds2483_dev_t *onewiredev) {
	if (!ds2483_1w_rst(onewiredev))
		return -ENODEV;

	ds2483_1w_write(onewiredev, DS18B2O_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, DS18B2O_CMD_CONVERT_T);

	return 0;
}

int8_t ds18b20_read_temp(ds2483_dev_t *onewiredev, int16_t *temp) {
	if (!ds2483_1w_rst(onewiredev))
		return -ENODEV;

	ds2483_1w_write(onewiredev, DS18B2O_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, DS18B2O_CMD_READ_SCRATCHPAD);

	uint8_t low = ds2483_1w_read_byte(onewiredev);
	uint8_t high = ds2483_1w_read_byte(onewiredev);

	//note: the low bits in the low byte are fractional
	*temp = (low>>4) | (high<<4);
	
	return 0;
}
