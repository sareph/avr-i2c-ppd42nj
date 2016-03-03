#include <stdio.h>
#include <stdlib.h>
#include "average.h"
#include <string.h>

void avgSampleInitDbl(struct avgData32 * data)
{
	data->i.sampleCurrent = 0;
	data->i.m = 0;
}

void avgSampleAddDbl(struct avgData32 * data, double smp)
{
	data->data[data->i.sampleCurrent] = smp;
	
	if (data->i.m < SYS_SAMPLE_AVERAGE_COUNT)
	{
		++data->i.m;
	}
	
	++data->i.sampleCurrent;
	
	if (data->i.sampleCurrent >= SYS_SAMPLE_AVERAGE_COUNT)
	{
		data->i.sampleCurrent = 0;
	}
}

double avgSampleAvgDbl(struct avgData32 * data)
{
	double s = 0;
	
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
		return (s / (double)data->i.m);
	}
}

void avgSampleInit64(struct avgData64 * data)
{
	data->i.sampleCurrent = 0;
	data->i.m = 0;
}

void avgSampleAdd64(struct avgData64 * data, int64_t smp)
{
	data->data[data->i.sampleCurrent] = smp;
	
	if (data->i.m < SYS_SAMPLE_AVERAGE_COUNT)
	{
		++data->i.m;
	}
	
	++data->i.sampleCurrent;
	
	if (data->i.sampleCurrent >= SYS_SAMPLE_AVERAGE_COUNT)
	{
		data->i.sampleCurrent = 0;
	}
}

int64_t avgSampleAvg64(struct avgData64 * data)
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
		return (s / (int64_t)data->i.m);
	}
}