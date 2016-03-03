#include "main.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "util/bootloader.h"
#include "util/average.h"
#include "util/swap.h"

//#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmain"

#define SAMPLE_TIME_MS 30000

static volatile uint32_t lPM10C = 0;
static volatile uint32_t lPM25C = 0;
static volatile uint32_t lPM10M = 0;
static volatile uint32_t lPM25M = 0;

static volatile uint32_t lMillis = 0;
static volatile uint32_t lMicros = 0;
static volatile uint32_t lUpSecs = 0;
static uint32_t lVersion = 0x11;

static volatile uint32_t lDebug1 = 0;
static volatile uint32_t lDebug2 = 0;

#define millis() lMillis
static volatile uint32_t lP1Duration = 0;
static volatile uint32_t lP2Duration = 0;

inline static uint32_t micros()
{
	/* Capture timer value */
	uint32_t ret = TCNT1;
	/* Multiply by it's resolution, 8us */
	ret *= 8;
	/* And return full value */
	return lMicros + ret;
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
	lMicros += 0xFFFFL * 8L;
}

#define REG_CASE32(base,var) \
		case base: \
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
		REG_CASE32(0x16, lVersion);

		REG_CASE32(0x1A, lDebug1);
		REG_CASE32(0x1E, lDebug2);

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

ISR(INT0_vect) /* PD2, P1 */
{
	static uint32_t lP1OnMicros = 0;

	if (PIND & (1 << PD2)) // high
	{
		lP1Duration += (micros() - lP1OnMicros);
	}
	else
	{
		lP1OnMicros = micros();
	}
}

ISR(INT1_vect) /* PD3, P2 */
{
	static uint32_t lP2OnMicros = 0;

	if (PIND & (1 << PD3)) // high
	{
		lP2Duration += (micros() - lP2OnMicros);
	}
	else
	{
		lP2OnMicros = micros();
	}
}

void __attribute__((noreturn)) main(void);
void main(void)
{
	float ratioP1, ratioP2, countP1, countP2;
	uint32_t startTime, sampleTimeMs = SAMPLE_TIME_MS;
	
	struct avgData32 lAvg10C;
	struct avgData32 lAvg10M;
	struct avgData32 lAvg25C;
	struct avgData32 lAvg25M;
	
	wdt_disable();
	rngSeed();

	avgSampleInitDbl(&lAvg10C);
	avgSampleInitDbl(&lAvg25C);
	
	avgSampleInitDbl(&lAvg10M);
	avgSampleInitDbl(&lAvg25M);

	//DDRD |= (1 << PD1);
	//PORTD |= (1 << PD1);

	OCR0A = 0x4D; // 100Hz
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS02) | (1 << CS00);	// div8
	TIMSK0 = (1 << OCIE0A);
	
	TCCR1A = 0;
	TCCR1B = (0 << WGM12) | (1 << CS11) | (1 << CS10); // div64, @125kHz, 8us period
	TIMSK1 = (1 << TOIE1);
	
	TWAR = (0x7C << 1);
	TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);

	EICRA = (1<<ISC10) | (1<<ISC00); /* both interrupts, both edges */
	EIMSK = (1<<INT0) | (1<<INT1);
	
	wdt_enable(WDTO_2S);
	startTime = millis();

	sei();
	for (;;)
	{
		wdt_reset();

		/*
			Following code adapted from dustduino,
			https://publiclab.org/wiki/dustduino
			I only hope, that my implementation of millis() and micros() is correct.
		*/
		// Function creates particle count and mass concentration
		// from PPD-42 low pulse occupancy (LPO).
		if ((millis() - startTime) > sampleTimeMs)
		{	
			// Generates PM10 and PM2.5 count from LPO.
			// Derived from code created by Chris Nafis
			// http://www.howmuchsnow.com/arduino/airquality/grovedust/

			ratioP1 = lP1Duration / (sampleTimeMs * 10.0); /* 0 -> 100 */
			ratioP2 = lP2Duration / (sampleTimeMs * 10.0);
			countP1 = 1.1 * pow(ratioP1, 3) - 3.8 * pow(ratioP1, 2) + 520 * ratioP1 + 0.62;
			countP2 = 1.1 * pow(ratioP2, 3) - 3.8 * pow(ratioP2, 2) + 520 * ratioP2 + 0.62;
			float PM10count = countP2; // particles/0.01cf
			float PM25count = countP1 - countP2;

			lDebug1 = lP1Duration;
			lDebug2 = lP2Duration;
			
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
			double K = 3531.47;
			float concLarge = (PM10count) * K * mass10;
			// next, PM2.5 mass concentration algorithm
			double r25 = 0.44 * pow(10.f, -6.f);
			double vol25 = (4.f / 3.f) * pi * pow(r25, 3.f);
			double mass25 = density * vol25;
			float concSmall = (PM25count) * K * mass25;

			/*
				PPD42 is noisy as hell, so average last 24 samples. 12 minutes window.
			*/
			avgSampleAddDbl(&lAvg10C, PM10count); 
			avgSampleAddDbl(&lAvg25C, PM25count); 
			avgSampleAddDbl(&lAvg10M, concLarge);
			avgSampleAddDbl(&lAvg25M, concSmall);
			
			lPM10C = avgSampleAvgDbl(&lAvg10C) * K; // pt/0.01f3 -> pt/m3
			lPM25C = avgSampleAvgDbl(&lAvg25C) * K;
			
			lPM10M = avgSampleAvgDbl(&lAvg10M) * 10000.f;  // ug/m3 * 10k
			lPM25M = avgSampleAvgDbl(&lAvg25M) * 10000.f;

			lP1Duration = 0;
			lP2Duration = 0;
			startTime = millis();
			
			/* Should we reset micros() here? If there is no pulse in progress. Just to avoid counter overflow after 70m */
			cli();
			if ((PIND & (1 << PD3)) && (PIND & (1 << PD2)))
			{
				lMicros = 0;
				TCNT1 = 0;
			}
			sei();
		}
	}
}