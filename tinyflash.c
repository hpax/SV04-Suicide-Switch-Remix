#include "tinyflash.h"

static void delay(void)
{
	_delay_ms(20);
}

int main(void)
{
	/*
	 * Outputs are:
	 * OC0A - PB0 - pin 5
	 * OC0B - PB1 - pin 6
	 * OC1B - PB4 - pin 3
	 */
	MCUCR = 0;		/* Enable input pullups */
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
	 * OCR0A = R
	 * OCR0B = G
	 * OCR1B = B
	 */
	for (;;) {
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
#if 0
		/* K -> R */
		for (uint8_t i = 1; i; i++) {
			OCR0A = (uint8_t)~i;
			delay();
		}
		/* R -> Y */
		for (uint8_t i = 1; i; i++) {
			OCR0B = (uint8_t)~i;
			delay();
		}
		/* Y -> G */
		for (uint8_t i = 254; i != 255; i--) {
			OCR0A = (uint8_t)~i;
			delay();
		}
		/* G -> C */
		for (uint8_t i = 1; i; i++) {
			OCR1B = (uint8_t)~i;
			delay();
		}
		/* C -> B */
		for (uint8_t i = 254; i != 255; i--) {
			OCR0B = (uint8_t)~i;
			delay();
		}
		/* B -> M */
		for (uint8_t i = 1; i; i++) {
			OCR0A = (uint8_t)~i;
			delay();
		}
		/* M -> W */
		for (uint8_t i = 1; i; i++) {
			OCR0B = (uint8_t)~i;
			delay();
		}
		/* W -> K */
		for (uint8_t i = 254; i != 255; i--) {
			OCR0A = OCR0B = OCR1B = (uint8_t)~i;
			delay();
		}
#endif
	}
}
