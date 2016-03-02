#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "../config/config.h"
#include <stdlib.h>

void _configRead(void *dst, uint16_t start, uint16_t len)
{
	eeprom_read_block(dst, (void*)start, len);
}

void _configUpdate(void *src, uint16_t start, uint16_t len)
{
	if (len <= 32)
	{
		uint8_t tmp[32];
		_configRead(tmp, start, len);
		if (memcmp(tmp, src, len))
		{
			eeprom_write_block(src, (void*)start, len);
		}
	}
	else
	{
		eeprom_write_block(src, (void*)start, len);
	}
}

void _configWrite(void *src, uint16_t start, uint16_t len)
{
	eeprom_write_block(src, (void*)start, len);
}
