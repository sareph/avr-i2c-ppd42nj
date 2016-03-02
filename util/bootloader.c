#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>

static void (*nullVector)(void) __attribute__((__noreturn__)) = 0x0000;

/*
	Not needed, since we use wdt for "bootloader enter". It's done this way, for cases when 
	m64 binary code is loaded into m128 device.
*/
/*
#if defined(__AVR_ATmega328P__)	
static void (*bootVector)(void) __attribute__((__noreturn__)) = 0x7000;
#elif defined(__AVR_ATmega64__)	
static void (*bootVector)(void) __attribute__((__noreturn__)) = 0xE000;
#elif defined(__AVR_ATmega128__)	
static void (*bootVector)(void) __attribute__((__noreturn__)) = 0x1E000;
#endif
*/

void leaveBootloader()
{
    cli();
    boot_rww_enable_safe();
	
	// check if first page is clear, if so - this function will fail
	if (pgm_read_byte_near(0x01) == 0xFF)
	{
		return;
	}
		
	SPSR = 0;
	SPCR = 0;

	DDRB 	= 0;
	DDRC 	= 0;
	DDRD 	= 0;
	
    MCUCR = (1 << IVCE);     /* enable change of interrupt vectors */
    MCUCR = (0 << IVSEL);    /* move interrupts to application flash section */

	wdt_enable(WDTO_2S);	// if something went wrong watchdog should reset mcu
	
    nullVector();
}

void startBootloader()
{
	sleep_disable();
	cli();
	wdt_disable();

#if defined(__AVR_ATmega328P__)	
	TIMSK1 = 0;
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	
	ASSR = 0;
	TCNT2 = 0;
	TIMSK2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;
	
	TCNT0 = 0;
	TIMSK0 = 0;
	TCCR0A = 0;
	TCCR0B = 0;
	
	TCCR0A = 0;
	TCCR0B = 0;
	TIMSK0 = 0;
	
	TCCR2A = 0;
	TCCR2B = 0;
	TIMSK2 = 0;
	
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = 0;
	
	TIFR2 = 0xFF;
	TIFR1 = 0xFF;
	TIFR0 = 0xFF;
	
	SPSR = 0;
	SPCR = 0;

	PCICR 	= 0;
	PCMSK0 	= 0;
	PCMSK1 	= 0;
	PCMSK2 	= 0;

	DDRB 	= 0;
	DDRC 	= 0;
#endif	

#if defined(__AVR_ATmega64__)
	SPSR = 0;
	SPCR = 0;

	DDRB 	= 0;
	DDRC 	= 0;
	DDRD 	= 0;
	
	EIMSK = 0x00;
	EICRA = 0x00;
	EICRB = 0x00;
	EIFR = 0xFF;
	
	TCCR2 = 0x00;
	MCUCR = 0;
	
#endif

	wdt_enable(WDTO_15MS);
	while(1);
}

