#pragma once

#include "config/config.h"

#define SYS_SAMPLE_AVERAGE_COUNT 12

struct avgInfo
{
	uint8_t sampleCurrent;
	uint8_t m;
};

struct avgData32
{
	struct avgInfo i;
	int32_t data[SYS_SAMPLE_AVERAGE_COUNT];
};

void avgSampleInit32(volatile struct avgData32 * data);
void avgSampleAdd32(volatile struct avgData32 * data, int32_t *smp);
void avgSampleAddD(volatile struct avgData32 * data, double smp);
int32_t avgSampleAvg32(volatile struct avgData32 * data);