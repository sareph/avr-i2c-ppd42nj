#pragma once

#include "../config/eeprom.h"

void _configRead(void *dst, uint16_t start, uint16_t len);
void _configUpdate(void *src, uint16_t start, uint16_t len);
void _configWrite(void *src, uint16_t start, uint16_t len);
