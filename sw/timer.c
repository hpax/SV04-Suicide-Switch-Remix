/*
 * Timer interrupt and LED PWM handling.
 */

#include "timer.h"
#include "pins.h"
#include "led.h"
#include "eeprom.h"

volatile struct timer_state _timer = {
    .tick   = { .l = 0 },
    .button = { false, 0 },
    .off    = { false, 0 },
    .led = {
	.flash_val   = 0,
	.flash_dir   = 1,
	.flash_ctr   = 0,
	.mode = { LED_OFF, LED_OFF, LED_OFF }
    }
};

/*
 * Timer 0 is used for the blue and green LED PWM and for
 * controlling the current level for the LED flash PWM.
 */
ISR(TIMER0_OVF_vect, ISR_BLOCK)
{
    uint8_t flv = _timer.led.flash_val;

    /* LED updating */
    uint8_t l, v;

    l = _timer.led.mode[LED_OC0A];
    v = (l & LED_FLASH) ? flv : 0;
    if (l & LED_ON)
	v = LED_MAX - v;
    OCR0A = v;

    l = _timer.led.mode[LED_OC0B];
    v = (l & LED_FLASH) ? flv : 0;
    if (l & LED_ON)
	v = LED_MAX - v;
    OCR0B = v;

    uint8_t ctr = _timer.led.flash_ctr;
    if (unlikely(!ctr)) {
	/* LED flash update */
	ctr = ee.flash_speed;
	int8_t dir = _timer.led.flash_dir;
	flv += dir;
	_timer.led.flash_val = flv;
	if (!flv)
	    _timer.led.flash_dir = 1;
	else if (flv >= LED_MAX)
	    _timer.led.flash_dir = -1;
    } else {
	ctr--;
    }
    _timer.led.flash_ctr = ctr;
}

/*
 * Timer 1 is used for the red LED PWM, the system timer, and input
 * deglitching.
 */
ISR(TIMER1_OVF_vect, ISR_BLOCK)
{
    /* Update LED */
    uint8_t l = _timer.led.mode[LED_OC1B];
    uint8_t v = 0;
    if (l & LED_FLASH)
	v = _timer.led.flash_val;
    if (l & LED_ON)
	v = LED_MAX - v;
    OCR1B = v;

    uint32_t tick = _timer.tick.l;
    _timer.tick.l = ++tick;

    /* Input read and debouncing */
    bool state;
    uint8_t ctr;

    v = PINB ^ ee.polarity;

    state = !!(v & PINMASK_BUTTON);
    if (state == _timer.button.state) {
	ctr = 0;
    } else {
	ctr = _timer.button.ctr;
	if (ctr) {
	    if (!--ctr)
		_timer.button.state = state;
	} else {
	    ctr = BUTTON_DEGLITCH - 1;
	}
    }
    _timer.button.ctr = ctr;

    state = !!(v & PINMASK_OFF);
    if (state == _timer.off.state) {
	ctr = 0;
    } else {
	ctr = _timer.off.ctr;
	if (ctr) {
	    if (!--ctr)
		_timer.off.state = state;
	} else {
	    ctr = OFF_DEGLITCH - 1;
	}
    }
    _timer.off.ctr = ctr;
}
