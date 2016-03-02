#include "main.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "util/bootloader.h"
#include "util/average.h"
#include "util/swap.h"

//#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmain"

static volatile struct avgData32 lAvg10C;
static volatile struct avgData32 lAvg25C;
static volatile struct avgData32 lAvg10M;
static volatile struct avgData32 lAvg25M;

static volatile uint32_t lPM10C = 0;
static volatile uint32_t lPM25C = 0;
static volatile uint32_t lPM10M = 0;
static volatile uint32_t lPM25M = 0;

static volatile uint32_t lMillis = 0;
static volatile uint32_t lMicros = 0;
static volatile uint32_t lUpSecs = 0;

#define millis() lMillis

inline static uint32_t micros()
{
	return lMicros + TCNT1;
}

ISR(TIMER0_COMPA_vect)
{
	lMillis += 10;
	
	if ((lMillis % 1000) == 0)
	{
		++lUpSecs;
	}
}

ISR(TIMER1_OVF_vect)
{
	lMicros += 65535;
}

#define REG_CASE32(base,var)	case base: \
			return var >> 0; \
		case base + 1: \
			return var >> 8; \
		case base + 2: \
			return var >> 16; \
		case base + 3: \
			return var >> 24;


uint8_t lRegRead(uint8_t reg)
{
	switch (reg)
	{
		case 0x00:
			return 0xAA;
		case 0x01: // config register, currently unused
			return 0;
		
		REG_CASE32(0x02, lPM10C);
		REG_CASE32(0x06, lPM25C);
		REG_CASE32(0x0A, lPM10M);
		REG_CASE32(0x0E, lPM25M);
		
		REG_CASE32(0x12, lUpSecs);

		default:
			return 0xFF;
	}
}

uint8_t lRegWrite(uint8_t reg, uint8_t val)
{
	switch (reg)
	{
		case 0x00:
		{
			if (val == 0x55)
			{
				startBootloader();
			}
			break;
		}
	}
	return 0;
}

ISR(TWI_vect)
{
	static uint8_t lReg, lTwiCnt;

	uint8_t ack = (1 << TWEA);
	uint8_t data;

	switch (TWSR & 0xF8)
	{
		/* SLA+W received, ACK returned -> receive data and ACK */
		case 0x60:
		{
			lTwiCnt = 0;
			TWCR |= (1 << TWINT) | (1 << TWEA);
			break;
		}
			/* prev. SLA+W, data received, ACK returned -> receive data and ACK */
		case 0x80:
		{
			data = TWDR;

			switch (lTwiCnt)
			{
				case 0:
				{
					lReg = data;
					break;
				}
				default:
				{
					lRegWrite(lReg++, data);
					break;
				}
			}

			++lTwiCnt;
			TWCR |= (1 << TWINT) | ack;
			break;
		}
		/* SLA+R received, ACK returned -> send data */
		case 0xA8:
		{
			lTwiCnt = 0;
		}
		/* prev. SLA+R, data sent, ACK returned -> send data */
		case 0xB8:
		{
			TWDR = lRegRead(lReg++);
			TWCR |= (1 << TWINT) | (1 << TWEA);
			break;
		}
		/* STOP or repeated START */
		case 0xA0:
		/* data sent, NACK returned */
		case 0xC0:
		{
			TWCR |= (1 << TWINT) | (1 << TWEA);
			break;
		}
		/* illegal state -> reset hardware */
		case 0xF8:
		{
			TWCR |= (1 << TWINT) | (1 << TWSTO) | (1 << TWEA);
			break;
		}
	}
}

void __attribute__((noreturn)) main(void);
void main(void)
{
	unsigned long starttime;
	unsigned long triggerOnP1;
	unsigned long triggerOffP1;
	unsigned long pulseLengthP1;
	unsigned long durationP1 = 0;
	uint8_t valP1 = 1;
	uint8_t triggerP1 = 0;
	unsigned long triggerOnP2;
	unsigned long triggerOffP2;
	unsigned long pulseLengthP2;
	unsigned long durationP2 = 0;
	uint8_t valP2 = 1;
	uint8_t triggerP2 = 0;
	float ratioP1 = 0;
	float ratioP2 = 0;
	unsigned long sampletime_ms = 30000;
	float countP1;
	float countP2;
		
	wdt_disable();
	rngSeed();

	avgSampleInit32(&lAvg10C);
	avgSampleInit32(&lAvg25C);
	
	avgSampleInit32(&lAvg10M);
	avgSampleInit32(&lAvg25M);

	//DDRD |= (1 << PD1);
	//PORTD |= (1 << PD1);

	OCR0A = 0x4D; // 100Hz
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS02) | (1 << CS00);	// div8
	TIMSK0 = (1 << OCIE0A);
	
	TCCR1A = 0;
	TCCR1B = (0 << WGM12) | (1 << CS11) | (1 << CS10); // div64, @125kHz, 8us
	TIMSK1 = (1 << TOIE1);
	
	TWAR = (0x7C << 1);
	TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);

	wdt_enable(WDTO_2S);
	starttime = millis();

	sei();
	for (;;)
	{
		wdt_reset();

		/*
			Following code adapted from dustduino,
			https://publiclab.org/wiki/dustduino
			I only hope, that my implementation of millis() and micros() is correct.
		*/
		
		valP1 = (PIND & (1 << PD2)) ? 1 : 0;
		valP2 = (PIND & (1 << PD3)) ? 1 : 0;

		if (valP1 == 0 && triggerP1 == 0)
		{
			triggerP1 = 1;
			triggerOnP1 = micros();
		}
		
		if (valP1 == 1 && triggerP1 == 1)
		{
			triggerOffP1 = micros();
			pulseLengthP1 = triggerOffP1 - triggerOnP1;
			durationP1 += pulseLengthP1;
			triggerP1 = 0;
		}
		
		if (valP2 == 0 && triggerP2 == 0)
		{
			triggerP2 = 1;
			triggerOnP2 = micros();
		}
		
		if (valP2 == 1 && triggerP2 == 1)
		{
			triggerOffP2 = micros();
			pulseLengthP2 = triggerOffP2 - triggerOnP2;
			durationP2 += pulseLengthP2;
			triggerP2 = 0;
		}

		// Function creates particle count and mass concentration
		// from PPD-42 low pulse occupancy (LPO).
		if ((millis() - starttime) > sampletime_ms)
		{
			// Generates PM10 and PM2.5 count from LPO.
			// Derived from code created by Chris Nafis
			// http://www.howmuchsnow.com/arduino/airquality/grovedust/
			ratioP1 = durationP1 / (sampletime_ms * 10.0);
			ratioP2 = durationP2 / (sampletime_ms * 10.0);
			countP1 = 1.1 * pow(ratioP1, 3) - 3.8 * pow(ratioP1, 2) + 520 * ratioP1 + 0.62;
			countP2 = 1.1 * pow(ratioP2, 3) - 3.8 * pow(ratioP2, 2) + 520 * ratioP2 + 0.62;
			float PM10count = countP2;
			float PM25count = countP1 - countP2;
			// Assues density, shape, and size of dust
			// to estimate mass concentration from particle
			// count. This method was described in a 2009
			// paper by Uva, M., Falcone, R., McClellan, A.,
			// and Ostapowicz, E.
			// http://wireless.ece.drexel.edu/research/sd_air_quality.pdf
			// begins PM10 mass concentration algorithm
			double r10 = 2.6 * pow(10.f, -6.f);
			double pi = 3.14159;
			double vol10 = (4.f / 3.f) * pi * pow(r10, 3.f);
			double density = 1.65 * pow(10, 12);
			double mass10 = density * vol10;
			double K = 3531.5;
			float concLarge = (PM10count) * K * mass10;
			// next, PM2.5 mass concentration algorithm
			double r25 = 0.44 * pow(10.f, -6.f);
			double vol25 = (4.f / 3.f) * pi * pow(r25, 3.f);
			double mass25 = density * vol25;
			float concSmall = (PM25count) * K * mass25;
			
			/*
				Comm protocol does not like floating point math, as it's not very portable at binary level.
				So we multiply everything by 10k. And average 12 samples. PPD-42 is noisy as hell.
			*/
			avgSampleAddD(&lAvg10C, PM10count * 10000.f * 35.3147f); // pt/f3 -> pt/m3
			avgSampleAddD(&lAvg25C, PM25count * 10000.f * 35.3147f); 
			avgSampleAddD(&lAvg10M, concLarge * 10000.f); // ug/m3
			avgSampleAddD(&lAvg25M, concSmall * 10000.f);
			
			lPM10C = avgSampleAvg32(&lAvg10C);
			lPM25C = avgSampleAvg32(&lAvg25C);
			
			lPM10M = avgSampleAvg32(&lAvg10M);
			lPM25M = avgSampleAvg32(&lAvg25M);

			durationP1 = 0;
			durationP2 = 0;
			starttime = millis();
		}
	}
}