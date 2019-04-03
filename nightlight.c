#include "util.h"

static void delay(void)
{
	_delay_ms(2);
}

/* Lowest and highest ADC values seen, stored in EEPROM */
static uint16_t adc_min, adc_max;
static EEMEM uint32_t eeprom_adc = UINT32_C(511) + (UINT32_C(512) << 16);

static void read_adc_limits(void)
{
	uint32_t eeval;

	eeval = eeprom_read_dword(&eeprom_adc);
	adc_min = (uint16_t)eeval;
	adc_max = (uint16_t)(eeval >> 16);
}

static void write_adc_limits(void)
{
	uint32_t eeval;

	eeval = adc_min + ((uint32_t)adc_max << 16);
	eeprom_write_dword(&eeprom_adc, eeval);
}

static volatile bool write_eeprom = false;

int main(void)
{
	/* Initializing... */
	cli();

	/*
	 * Outputs are:
	 * OC0A - PB0 - pin 5
	 * OC0B - PB1 - pin 6
	 * OC1B - PB4 - pin 3
	 */
	MCUCR = 0x20;		/* Disable input pullups, sleep mode = idle */
	DDRB  = 0x37;		/* PB0,1,4 as outputs; PB2,5 unused so drive to GND */
	PORTB = 0x00;		/* Drive everything to zero initially */

	/* Timer interrupt and status */
	TIMSK  = 0;
	TIFR   = 0;

	/*
	 * Timer 0 programming
	 *
	 * OC0A/OC0B are set up in inverting phase correct PWM mode.
	 * This gives a dynamic range from (0-255):255.
	 */
	TCCR0A = (3 << 6) | (3 << 4) | (1 << 0);
	TCCR0B = (0 << 3) | (1 << 0);

	OCR0A  = 255;		/* Off */
	OCR0B  = 255;		/* Off */

	/*
	 * Timer 1 programming
	 *
	 * HARDWARE ERRATUM: COM1Ax must be = COM1Bx or OC1B PWM output
	 * does not work.  Fortunately the PWM from timer 0 seems to take
	 * priority on pin PB1.
	 */
	PLLCSR = 0;
	TCCR1 = (1 << 7) | (0 << 6) | (3 << 4) | (1 << 0);
	GTCCR = (1 << 6) | (3 << 4); /* OC1B enabled, inverted PWM mode */
	OCR1C = 255;		     /* Run from 0 to 255 */
	OCR1B = 255;		     /* Off */

	/*
	 * ADC input programming
	 */
	DIDR0  = 0x08;		/* Analog input on ADC3/PB3/pin 2 */
	ADMUX  = 0x03;		/* Single ended input on ADC3 vs Vcc */
	ADCSRA = 0;		/* Disabled for now */

	/* Read known ADC limits */
	read_adc_limits();

	/* We are now in a "safe" place to enable interrupts */
	sei();

	/*
	 * OCR0A = R
	 * OCR0B = G
	 * OCR1B = B
	 */
	/* Pulse R */
	for (uint8_t i = 1; i; i++) {
		OCR0A = (uint8_t)~i;
		delay();
	}
	for (uint8_t i = 254; i != 255; i--) {
		OCR0A = (uint8_t)~i;
		delay();
	}
	/* Pulse G */
	for (uint8_t i = 1; i; i++) {
		OCR0B = (uint8_t)~i;
		delay();
	}
	for (uint8_t i = 254; i != 255; i--) {
		OCR0B = (uint8_t)~i;
		delay();
	}
	/* Pulse B */
	for (uint8_t i = 1; i; i++) {
		OCR1B = (uint8_t)~i;
		delay();
	}
	for (uint8_t i = 254; i != 255; i--) {
		OCR1B = (uint8_t)~i;
		delay();
	}

	/*
	 * Enable ADC and ADC interrupt
	 */
	ADCSRB = 0x00;		/* Free running mode */
	ADCSRA = 0xec;		/* Enable, auto trigger, IRQ,
				   ck/16 = 62.5 kHz */
	for (;;) {
		if (unlikely(write_eeprom)) {
			write_eeprom = false;
			write_adc_limits();
		}
		sleep_cpu();	/* Actual work is done in interrupt handler */
	}
}

/* ADC conversion complete interrupt routine */
ISR(ADC_vect)
{
	uint16_t adc;
	uint8_t r, g, b;
	static uint8_t eeprom_ctr = 0;

	adc = ADCL;
	adc += (uint16_t)ADCH << 8;

	/* Check to see if we have new limits */
	const uint8_t eeprom_delay = 64; /* Write max 1 per second */
	if (adc < adc_min) {
		adc_min = adc;
		eeprom_ctr = eeprom_delay;
	}
	if (adc > adc_max) {
		adc_max = adc;
		eeprom_ctr = eeprom_delay;
	}

	/* Scale the ADC values to the range [0, 2^15+guard) */
	const uint16_t guard = 512;
	adc -= adc_min;
	adc = (uint32_t)adc * (0x7fff + guard) / (adc_max - adc_min);

	/* Guard band to make sure we go to black */
	adc = (adc < guard) ? 0 : adc-guard;

	r = adc >> 7;
	g = (adc < 0x2aaa) ? 0 : ((adc-0x2aaa)*3) >> 8;
	b = (adc < 0x5555) ? 0 : ((adc-0x5555)*3) >> 7;

	OCR0A = (uint8_t)~r;
	OCR0B = (uint8_t)~g;
	OCR1B = (uint8_t)~b;

	/* Do we need to update the eeprom? */
	if (eeprom_ctr > 1)
		eeprom_ctr--;

	if (eeprom_ctr != 1)
		return;

	write_eeprom = true;
	eeprom_ctr = 0;
}
