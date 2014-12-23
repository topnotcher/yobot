#include <util/crc16.h>
#include "ds18b20.h"
#include "ds2483.h"
#include "error.h"

//applicable when there is only ONE one wire device on the bus!
#define DS18B2O_CMD_SKIP_ROM 0xCC
#define DS18B2O_CMD_READ_SCRATCHPAD 0xBE
#define DS18B2O_CMD_WRITE_SCRATCHPAD 0x4E
#define DS18B2O_CMD_CONVERT_T 0x44

static int8_t ds18b20_write_config(ds2483_dev_t *onewiredev);

int8_t ds18b20_start_conversion(ds2483_dev_t *onewiredev) {
	int8_t err = ds18b20_write_config(onewiredev);

	if (err)
		return err;

	if (!ds2483_1w_rst(onewiredev))
		return -ENODEV;

	ds2483_1w_write(onewiredev, DS18B2O_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, DS18B2O_CMD_CONVERT_T);

	return 0;
}

/**
 * This is writing data to the scratchpad to verify on read. i.e. if the data
 * comes back different, teh device lost power. This is to attempt to account
 * for the default temperature of 85C on boot. (Even the CRC is valid in this case???)
 */
static int8_t ds18b20_write_config(ds2483_dev_t *onewiredev) {
	if (!ds2483_1w_rst(onewiredev))
		return -ENODEV;

	ds2483_1w_write(onewiredev, DS18B2O_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, DS18B2O_CMD_WRITE_SCRATCHPAD);
	ds2483_1w_write(onewiredev, 0x13);
	ds2483_1w_write(onewiredev, 0x13);
	ds2483_1w_write(onewiredev, 0b01111111);
	return 0;
}


int8_t ds18b20_read_temp(ds2483_dev_t *onewiredev, int16_t *temp) {
	if (!ds2483_1w_rst(onewiredev))
		return -ENODEV;

	ds2483_1w_write(onewiredev, DS18B2O_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, DS18B2O_CMD_READ_SCRATCHPAD);

	uint8_t low = ds2483_1w_read_byte(onewiredev);
	uint8_t high = ds2483_1w_read_byte(onewiredev);
	uint8_t chk = ds2483_1w_read_byte(onewiredev);

	if (chk != 0x13)
		return -EINVAL;

	//note: the low bits in the low byte are fractional
	*temp = (low>>4) | (high<<4);

	return 0;
}
