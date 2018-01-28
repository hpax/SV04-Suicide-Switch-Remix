#include "tinyflash.h"

static void delay(void)
{
	_delay_ms(2);
}

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
	MCUCR = 0x20;		/* Enable input pullups, sleep mode = idle */
	DDRB  = 0x13;		/* PB0,1,4 as outputs */
	PORTB = 0x3f;		/* Pullups on all inputs */

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
	for (;;)
		sleep_cpu();	/* Actual work is done in interrupt handler */
}

/* ADC conversion complete interrupt routine */
ISR(ADC_vect)
{
	uint16_t adc;
	uint8_t r, g, b;

	adc = ADCL;
	adc += (uint16_t)ADCH << 8;

	/* Guard band to make sure we go to black */
	const uint16_t guard = 64;
	adc = (adc < guard) ? 0 : adc-guard;

	r = adc >> 2;
	g = (adc < 341) ? 0 : ((adc-341)*3) >> 3;
	b = (adc < 682) ? 0 : ((adc-682)*3) >> 2;

	OCR0A = (uint8_t)~r;
	OCR0B = (uint8_t)~g;
	OCR1B = (uint8_t)~b;
}
