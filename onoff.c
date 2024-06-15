#include "util.h"

/* Fuse settings */
FUSES = {
    /* Clock: internal 1 MHz */
    .low	= LFUSE_DEFAULT,
    /* All pins in use as GPIO, no reset/debugwire/SPI programming */
    .high	= FUSE_RSTDISBL,
    /* Self-programming of flash is not used, so disabled */
    .extended	= EFUSE_DEFAULT
};

/* LED intensities */
#define LEDR_PWM	255
#define LEDG_PWM	255
#define LEDB_PWM	255

#define LED_COMMON_ANODE	1
#define TCRCOM		(LED_COMMON_ANODE ? 3 : 2)

/* Pinmasks */
#define PINMASK_OFF	(1U << 5)
#define PINMASK_BUTTON	(1U << 3)
#define PINMASK_SSR	(1U << 7)
#define PINMASK_LEDS	((1U << 3)|(1U << 5)|(1U << 6))

enum led {
    LED_R,
    LED_G,
    LED_B
};

static inline void set_led(enum led led, uint8_t val)
{
    switch (led) {
    case LED_R: OCR1B = ~val; break;
    case LED_G: OCR0B = ~val; break;
    case LED_B: OCR0A = ~val; break;
    }
}

static void init(void)
{
    /* Initializing... */
    cli();

    /*
     * Inputs:
     * OFF#    - PB5 - pin 1 - power off from mainboard
     * BUTTON# - PB3 - pin 2 - pushbutton
     * Outputs:
     * OC1B    - PB4 - pin 3 - red LED	  (active low)
     * OC0A    - PB0 - pin 5 - blue LED   (active low)
     * OC0B    - PB1 - pin 6 - green LED  (active low)
     * SSR     - PB2 - pin 7 - SSR enable (active high)
     *
     * Initially drive everything to its inactive state; enable
     * pullups on inputs.
     */
    PORTB = (uint8_t)~PINMASK_SSR;
    MCUCR = 0x00;		/* Enable input pullups, sleep mode = idle */
    DDRB  = PINMASK_LEDS|PINMASK_SSR;

    /* Timer interrupt and status */
    TIMSK  = 0;
    TIFR   = 0;

    /*
     * Timer 0 programming
     *
     * OC0A/OC0B are set up in inverting phase correct PWM mode.
     * This gives a dynamic range from (0-255):255.
     */
    TCCR0A = (TCRCOM << 6) | (TCRCOM << 4) | (1 << 0);
    TCCR0B = (0 << 3) | (1 << 0);

    /*
     * Timer 1 programming
     *
     * HARDWARE ERRATUM: COM1Ax must be = COM1Bx or OC1B PWM output
     * does not work.  Fortunately the PWM from timer 0 seems to take
     * priority on pin PB1.
     */
    PLLCSR = 0;
    TCCR1 = (1 << 7) | (0 << 6) | (TCRCOM << 4) | (1 << 0);
    GTCCR = (1 << 6) | (TCRCOM << 4); /* OC1B enabled, inverted PWM mode */
    OCR1C = 255;		     /* Run from 0 to 255 */

    /* Disable all LEDs */
    set_led(LED_R, 0);
    set_led(LED_G, 0);
    set_led(LED_B, 0);
}

static void loop(void)
{
    /*
     * Poll both the pushbutton and the power off input with
     * debounce. The power off input is ignored if the power is
     * already off.
     */

    /* This can be edited; can use eeprom to preserve pre-powerfail state */
    bool power  = false;	/* Off */
    bool button = false;	/* Not pressed */
    unsigned int button_ctr   = 0;
    unsigned int poweroff_ctr = 0;
    unsigned int poweron_ctr  = 0; /* Time after poweron until OFF valid  */

    const unsigned int button_ctr_incr = 65536UL/256;
    const unsigned int button_ctr_decr = 65536UL/16;

    const unsigned int poweroff_incr   = 65536UL/256;
    const unsigned int poweron_incr    = 1;

    while (true) {
	bool prev_power  = power;
	bool prev_button = button;

	set_led(LED_G, button ? LEDG_PWM : 0);
	set_led(LED_R, power  ? 0 : LEDR_PWM);
	set_led(LED_B, power  ? LEDB_PWM : 0);

	if (power != !!(PORTB & PINMASK_SSR))
	    PINB = PINMASK_SSR;

	do {
	    uint8_t pb = PORTB;

	    if (!(pb & PINMASK_BUTTON)) {
		if (button_ctr <= ~button_ctr_incr) {
		    button_ctr += button_ctr_incr;
		} else {
		    button = true;
		}
	    } else {
		if (button_ctr >= button_ctr_decr) {
		    button_ctr -= button_ctr_decr;
		} else {
		    button = false;
		}
	    }

	    if (button && !prev_button)
		power = !power;

	    if (power) {
		if (poweron_ctr <= ~poweron_incr) {
		    poweron_ctr += poweron_incr;
		}
	    } else {
		poweron_ctr = 0;
	    }

	    if (poweron_ctr > ~poweron_incr) {
		if (!(pb & PINMASK_OFF)) {
		    if (poweroff_ctr <= ~poweroff_incr) {
			poweroff_ctr += poweroff_incr;
		    } else {
			power = false;
		    }
		} else {
		    if (poweroff_ctr >= poweroff_incr) {
			poweroff_ctr -= poweroff_incr;
		    }
		}
	    } else {
		poweroff_ctr = 0;
	    }
	} while (power == prev_power && button == prev_button);
    }
}

int main(void)
{
    init();
    loop();
}
