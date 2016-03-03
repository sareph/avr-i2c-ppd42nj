#pragma once

#include "config/config.h"

#define SYS_SAMPLE_AVERAGE_COUNT 24 // 12 minutes

struct avgInfo
{
	uint8_t sampleCurrent;
	uint8_t m;
};

struct avgData32
{
	struct avgInfo i;
	double data[SYS_SAMPLE_AVERAGE_COUNT];
};

struct avgData64
{
	struct avgInfo i;
	int64_t data[SYS_SAMPLE_AVERAGE_COUNT];
};

void avgSampleInitDbl(struct avgData32 * data);
void avgSampleAddDbl(struct avgData32 * data, double smp);
double avgSampleAvgDbl(struct avgData32 * data);

void avgSampleInit64(struct avgData64 * data);
void avgSampleAdd64(struct avgData64 * data, int64_t smp);
int64_t avgSampleAvg64(struct avgData64 * data);