#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdlib.h>
 
static volatile uint32_t seed; // These two variables can be reused in your program after the
static volatile int8_t nrot; // function CreateTrulyRandomSeed()executes in the setup()
                     // function.
 
void rngSeed()
{
	seed = 0;
	nrot = 32; // Must be at least 4, but more increased the uniformity of the produced

#if defined(__AVR_ATmega328P__)
	// seeds entropy.
	// The following five lines of code turn on the watch dog timer interrupt to create
	// the seed value
	cli();
	
	TCCR1A    = (0<<WGM11)|(0<<WGM10);
	TCCR1B    = (0<<WGM13)|(1<<WGM12); // CTC mode
	OCR1A     = 255;
	TCCR1B    |= (0<<CS12)|(1<<CS11)|(0<<CS10);	
	
	MCUSR = 0;
	_WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE);
	_WD_CONTROL_REG = (1<<WDIE);
	sei();
	 
	while (nrot > 0); // wait here until seed is created
	 
	// The following five lines turn off the watch dog timer interrupt
	cli();
	MCUSR = 0;
	_WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE);
	_WD_CONTROL_REG = (0<< WDIE);

	TCCR1A    = 0;
	TCCR1B    = 0;
	OCR1A     = 0;
	sei();
	
	srandom(seed);
#endif
}
 
#if defined(__AVR_ATmega328P__)
ISR(WDT_vect)
{
	nrot--;
	seed = seed << 8;
	seed = seed ^ TCNT1L;
}
#endif
