#include "tinyflash.h"

int main(void)
{
	int i;

	MCUCR = 0;		/* Enable input pullups */
	DDRB  = 0x07;		/* PB0-2 as outputs */
	PORTB = 0x38;		/* Pullups on inputs */

	for (i = 0; ; i = (i+1) & 7) {
		_delay_ms(500);
		PORTB = 0x38 | i;
	}
}

