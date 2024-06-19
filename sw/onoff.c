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

/* Pinmasks - these are PBx port bit numbers, not physical pins! */
#define PIN_OFF		5
#define PINMASK_OFF	(1U << PIN_OFF)
#define PIN_BUTTON	3
#define PINMASK_BUTTON	(1U << PIN_BUTTON)
#define PIN_SSR		2
#define PINMASK_SSR	(1U << PIN_SSR)
#define PINMASK_LEDS	((1U << 0)|(1U << 1)|(1U << 4))

enum led {
    LED_R,
    LED_G,
    LED_B
};

static inline void set_led(enum led led, uint8_t val)
{
    switch (led) {
    case LED_R: OCR1B = val; break;
    case LED_G: OCR0B = val; break;
    case LED_B: OCR0A = val; break;
    }
}

static void init(void)
{
    /* Initializing... */
    cli();

    /* Interrupt control */
    GIMSK = 0;
    PCMSK = 0;

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
    PORTB = (uint8_t)~((LED_COMMON_ANODE ? 0 : PINMASK_LEDS)|PINMASK_SSR);
    MCUCR = 0x00;		/* Enable input pullups, sleep mode = idle */
    DDRB  = PINMASK_LEDS|PINMASK_SSR;

    /* Timer interrupt and status */
    TIMSK  = 0;
    TIFR   = 0;

    /* COMxx values */
#define TCRCOM		(LED_COMMON_ANODE ? 3 : 2)

    /*
     * Timer 1 programming
     *
     * HARDWARE ERRATUM: COM1Ax must be = COM1Bx or OC1B PWM output
     * does not work.  Fortunately the PWM from timer 0 seems to take
     * priority on pin PB1.
     */
    /* OC1B enabled, inverted PWM mode;  */
#define GTCCR_CONFIG	((1 << 6) | (TCRCOM << 4))
    /* Bits to be held until after configuration */
#define GTCCR_SYNC	((1 << 7) | (1 << 0))

    GTCCR = GTCCR_CONFIG | GTCCR_SYNC;
    PLLCSR = 0;
    TCCR1 = (1 << 7) | (0 << 6) | (TCRCOM << 4) | (1 << 0);
    OCR1C = 255;		     /* Run from 0 to 255 */

    /*
     * Timer 0 programming
     *
     * OC0A/OC0B are set up in inverting phase correct PWM mode.
     * This gives a dynamic range from (0-255):255.
     */
    TCCR0A = (TCRCOM << 6) | (TCRCOM << 4) | (1 << 0);
    TCCR0B = (0 << 3) | (1 << 0);

    /* Disable all LEDs */
    set_led(LED_R, 0);
    set_led(LED_G, 0);
    set_led(LED_B, 0);

    /* Enable to PWM counters */
    GTCCR = GTCCR_CONFIG;

    /* If we ever do interrupt stuff, should be OK now... */
    sei();
}

static inline bool button_pressed(void)
{
    return !((PINB >> PIN_BUTTON) & 1);
}
static inline bool poweroff_input(void)
{
    return !((PINB >> PIN_OFF) & 1);
}

static void set_button_led(bool button)
{
    set_led(LED_G, button ? LEDG_PWM : 0);
}

static void set_power_led(bool power)
{
    set_led(LED_R, power ? 0 : LEDR_PWM);
    set_led(LED_B, power ? LEDB_PWM : 0);
}

static inline bool power_is_on(void)
{
    return (PORTB >> PIN_SSR) & 1;
}

static void loop(void)
{
    /*
     * Poll both the pushbutton and the power off input with
     * debounce. The power off input is ignored if the power is
     * already off.
     */
    bool power  = power_is_on();
    bool button = button_pressed();

    /* Debounce counters */
    unsigned int button_ctr   = button ? 32768UL : 0;
    unsigned int poweroff_ctr = 0;
    unsigned int poweron_ctr  = 0; /* Time after poweron until OFF valid  */

    const unsigned int button_ctr_incr = 65536ULL/256;
    const unsigned int button_ctr_decr = 65536ULL/16;

    const unsigned int poweroff_incr   = 65536ULL/256;
    const unsigned int poweron_incr    = 1;

    set_power_led(power);
    set_button_led(button);

    while (true) {
	if (button_pressed()) {
	    if (button_ctr <= ~button_ctr_incr) {
		button_ctr += button_ctr_incr;
	    } else if (!button) {
		set_button_led(button = true);
		set_power_led(power = !power);
		PINB = PINMASK_SSR; /* Toggle SSR output */
	    }
	} else {
	    if (button_ctr >= button_ctr_decr) {
		button_ctr -= button_ctr_decr;
	    } else if (button) {
		set_button_led(button = false);
	    }
	}

	if (power) {
	    if (poweron_ctr <= ~poweron_incr) {
		poweron_ctr += poweron_incr;
	    }
	} else {
	    poweron_ctr = 0;
	}

	if (poweron_ctr > ~poweron_incr) {
	    if (poweroff_input()) {
		if (poweroff_ctr <= ~poweroff_incr) {
		    poweroff_ctr += poweroff_incr;
		} else if (power) {
		    set_power_led(power = false);
		    PORTB &= ~PINMASK_SSR;
		}
	    } else {
		if (poweroff_ctr >= poweroff_incr) {
		    poweroff_ctr -= poweroff_incr;
		}
	    }
	} else {
	    poweroff_ctr = 0;
	}
    }
}

static inline void delay(unsigned int multiplier)
{
    _delay_ms(multiplier * 2);
}

static void test_leds(void)
{
    struct rgbs {
	int8_t r, g, b;
    };
    static const struct rgbs fades[] = {
	{ +1,  0,  0 },		/* K > R */
	{  0, +1,  0 },		/* R > Y */
	{ -1,  0,  0 },		/* Y > G */
	{  0,  0, +1 },		/* G > C */
	{  0, -1,  0 },		/* C > B */
	{ +1,  0,  0 },		/* B > M */
	{  0, +1,  0 },		/* M > W */
	{ -1, -1, -1 }		/* W > K */
    };
    uint8_t r, g, b;

    set_led(LED_R, r = 0);
    set_led(LED_G, g = 0);
    set_led(LED_B, b = 0);

    for (uint8_t n = 0; n < 8; n++) {
	const struct rgbs *fade = fades;
	for (uint8_t nf = 0; nf < ARRAY_SIZE(fades); nf++) {
	    int8_t dr = fade->r;
	    int8_t dg = fade->g;
	    int8_t db = fade->b;
	    fade++;
	    for (uint8_t i = 1; i; i++) {
		delay(1);
		set_led(LED_R, r += dr);
		set_led(LED_G, g += dg);
		set_led(LED_B, b += db);
	    }
	}
    }

    delay(32);
}

int main(void)
{
    init();
    if (button_pressed())
	test_leds();
    loop();
}
