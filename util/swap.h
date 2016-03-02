#pragma once

#define SWAP16(val) ((((uint16_t) (val)) << 8) | (((uint16_t) (val)) >> 8))
#define SWAP32(val) (((((uint32_t) (val)) & 0x000000ff) << 24) | ((((uint32_t) (val)) & 0x0000ff00) <<	8) | ((((uint32_t) (val)) & 0x00ff0000) >>	8) | ((((uint32_t) (val)) & 0xff000000) >> 24))