#include <stdio.h>
#include <stdlib.h>
#include "average.h"
#include <string.h>

void avgSampleInit32(volatile struct avgData32 * data)
{
	data->i.sampleCurrent = 0;
	data->i.m = 0;
	memset((struct avgData32 *)data->data, 0, sizeof(int32_t) * SYS_SAMPLE_AVERAGE_COUNT);
}

void avgSampleAdd32(volatile struct avgData32 * data, int32_t *smp)
{
	data->data[(data->i.sampleCurrent % SYS_SAMPLE_AVERAGE_COUNT)] = *smp;
	
	if (data->i.m < SYS_SAMPLE_AVERAGE_COUNT)
	{
		data->i.m ++;
	}
	
	data->i.sampleCurrent++;
}

void avgSampleAddD(volatile struct avgData32 * data, double smp)
{
	int32_t t = smp;
	avgSampleAdd32(data, &t);
}

int32_t avgSampleAvg32(volatile struct avgData32 * data)
{
	int64_t s = 0;
	
	for (uint8_t i = 0; i < data->i.m; ++i)
	{
		s += data->data[i];
	}
	
	if (data->i.m == 0)
	{
		return 0;
	}
	else
	{
		return s / data->i.m;
	}
}