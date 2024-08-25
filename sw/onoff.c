#include "util.h"
#include "pins.h"
#include "timer.h"
#include "led.h"
#include "eeprom.h"

/* Fuse settings */
FUSES = {
    /* Clock: internal 1 MHz */
    .low	= LFUSE_DEFAULT,
    /* All pins in use as GPIO, no reset/debugwire/SPI programming */
    .high	= FUSE_RSTDISBL,
    /* Self-programming of flash is not used, so disabled */
    .extended	= EFUSE_DEFAULT
};

/* Default flash speed on power up (units of 260 ms, minus one) */
#define FLASH_SPEED	7

/* Default delays in units of 65.536 ms */
#define MIN_DELAY	2	/* Time before deglitch assumed stable */
#define MIN_OFF_DELAY	32	/* Time before power off is enabled */
#define BUTTON_PRESS	4	/* Time before button is considered pressed */
#define BUTTON_CANCEL	48	/* Time before button press is cancelled */

/*
 * EEPROM variables - used for configuration to avoid needing to
 * recompile the firmware to tweak these variables.
 */
static EEMEM struct eeprom eeprom = {
    .polarity           = PINB_POLARITY,
    .flash_speed	= FLASH_SPEED,
    .min_delay          = MIN_DELAY,
    .min_off_delay      = MIN_OFF_DELAY,
    .button_press       = BUTTON_PRESS,
    .button_cancel      = BUTTON_CANCEL
};
struct eeprom ee;

static void load_eeprom(void)
{
    eeprom_read_block((void *)&ee, &eeprom, sizeof ee);
    EECR = 0;			/* For future programming operations */
}

static inline __attribute__((const))
uint8_t inactive(bool active_low, uint8_t mask) {
    return active_low ? mask : 0;
}

static void init(void)
{
    /* Initializing... */
    cli();

    /* Get EEPROM copy in RAM */
    load_eeprom();

    /*
     * Default configuration; see pins.h
     *
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
     * pullups on inputs (set to 1 regardless of sense)
     */
    PORTB = ee.polarity | PINMASK_BUTTON | PINMASK_OFF;
    MCUCR = 0x00;		/* Enable input pullups, sleep mode = idle */
    DDRB  = PINMASK_LEDS|PINMASK_SSR;

    /* Timer interrupt and status */
    TIMSK  = (1 << 1) | (1 << 2);	/* Enable timer 0&1 overflow IRQs */
    TIFR   = 0xff;			/* Clear all pending interrupts */

    /* COMxx bit 0 sets the polarity of PWM output */
    uint8_t oc0a_pol = (ee.polarity & 0x01) << (6-0); /* PB0 */
    uint8_t oc0b_pol = (ee.polarity & 0x02) << (4-1); /* PB1 */
    uint8_t oc1b_pol = (ee.polarity & 0x10) << (4-4); /* PB4 */

    /*
     * Timer 1 programming
     *
     * HARDWARE ERRATUM: COM1Ax must be = COM1Bx or OC1B PWM output
     * does not work.  Fortunately the PWM from timer 0 seems to take
     * priority on pin PB1.
     */
    /* OC1B enabled, inverted PWM mode;  */
    uint8_t gtccr_config = (1 << 6) | (2 << 5) | oc1b_pol;
    /* Bits to be held until after configuration */
#define GTCCR_SYNC	((1 << 7) | (1 << 0))

    GTCCR = gtccr_config | GTCCR_SYNC;
    PLLCSR = 0;
    TCCR1 = (1 << 7) | (0 << 6) | (2 << 4) | (1 << 0) | oc1b_pol;
    OCR1C = 255;		     /* Run from 0 to 255 */

    /*
     * Timer 0 programming
     *
     * OC0A/OC0B are set up in inverting phase correct PWM mode.
     * This gives a dynamic range from (0-255):255.
     */
    TCCR0A = (2 << 6) | (2 << 4) | (1 << 0) | oc0a_pol | oc0b_pol;
    TCCR0B = (0 << 3) | (1 << 0);

    /* Disable all LEDs */
    OCR0A = OCR0B = OCR1B = 0;

    /* Enable PWM counters */
    GTCCR = gtccr_config;

    /* Disable pin interrupts */
    GIMSK = 0;
    PCMSK = 0;

    /* Global enable interrupts */
    sei();
}

static inline void set_power_on(bool power)
{
    if (power ^ !!(ee.polarity & PINMASK_SSR)) {
	PORTB |= PINMASK_SSR;
    } else {
	PORTB &= ~PINMASK_SSR;
    }
}

static void set_button_led(bool button)
{
    set_led(LED_G, button ? LED_ON : LED_OFF);
}

static void set_power_led(bool power)
{
    set_led(LED_R, power ? LED_OFF : LED_ON);
    set_led(LED_B, power ? LED_ON : LED_OFF);
}
static void set_power_led_init(bool power)
{
    set_led(LED_R, power ? LED_OFF : LED_FLASH);
    set_led(LED_B, power ? LED_ON  : LED_OFF);
}


/*
 * Nonvolatile last power status; used to detect power failures.
 * After a power failure when the power was ON, flash the button
 * for attention.
 */


enum button_mode {
    BUTTON_RELEASED,
    BUTTON_DELAYING,
    BUTTON_ACTIVE,
    BUTTON_CANCELLED
};

static void __attribute__((noreturn)) loop(void)
{
    /*
     * Poll both the pushbutton and the power off input with
     * debounce. The power off input is ignored if the power is
     * already off.
     */
    bool     button_state     = false;
    uint8_t  button_mode      = BUTTON_RELEASED;
    bool     power            = false;
    bool     old_off_state    = false;
    bool     delay_armed      = false;
    bool     off_armed        = false;
    uint16_t button_when      = 0;

    set_all_led(LED_OFF);

    while (true) {
	uint16_t now;

	cli();
	now          = _timer.tick.w[0];
	button_state = _timer.button.state;
	sei();

	/* Let the debounce stabilize */
	if (!delay_armed) {
	    if (now < MIN_DELAY)
		continue;
	    delay_armed = true;
	    set_power_led_init(power);
	    set_button_led(button_state);
	    button_mode = button_state
		? BUTTON_CANCELLED : BUTTON_RELEASED;
	}

	if (!button_state) {
	    /* Button released */
	    if (button_mode != BUTTON_RELEASED) {
		if (button_mode == BUTTON_ACTIVE) {
		    power = !power;
		    set_power_on(power);
		    set_power_led(power);
		}
		set_button_led(false);
		button_mode = BUTTON_RELEASED;
	    }
	} else {
	    uint16_t hold_time = now - button_when;
	    /* Button pressed */
	    switch (button_mode) {
	    case BUTTON_RELEASED:
		button_when = now;
		set_button_led(true);
		button_mode = BUTTON_DELAYING;
		break;
	    case BUTTON_DELAYING:
		if (hold_time >= BUTTON_PRESS) {
		    set_power_led(!power);
		    button_mode = BUTTON_ACTIVE;
		}
		break;
	    case BUTTON_ACTIVE:
		if (hold_time >= BUTTON_CANCEL) {
		    set_power_led(power);
		    set_button_led(false);
		    button_mode = BUTTON_CANCELLED;
		}
		break;
	    default:
		break;
	    }
	}

	uint8_t off_state = _timer.off.state;
	if (!off_armed) {
	    if (now < MIN_OFF_DELAY) {
		old_off_state = off_state;
		continue;
	    }
	    off_armed = true;
	}

	if (off_state && !old_off_state) {
	    set_power_on(power = false);
	    set_power_led(false);
	    if (button_mode != BUTTON_RELEASED) {
		button_mode = BUTTON_CANCELLED;
		set_button_led(false);
	    }
	}
	old_off_state = off_state;

	/* Nothing to do until next interrupt */
	asm volatile("sleep");
    }
}

int main(void)
{
    init();
    loop();
}
