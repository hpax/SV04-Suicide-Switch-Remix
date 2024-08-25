#ifndef LED_H
#define LED_H

#include "timer.h"

/* LED intensities */
#define LED_MAX	255

enum led {
    LED_R,
    LED_G,
    LED_B
};

enum led_state {
    LED_OFF    =   0,
    LED_ON     =   1,
    LED_FLASH  =   2,
    LED_FLASHI =   3
};

#define PIN_TO_LED(x)			\
    ((PIN_LED_R == (x)) ? LED_R :	\
     (PIN_LED_G == (x)) ? LED_G :	\
     LED_B)

#define LED_OC0A PIN_TO_LED(PIN_5)	/* OC0A == PB0 == pin 5 */
#define LED_OC0B PIN_TO_LED(PIN_6)	/* OC0B == PB1 == pin 6 */
#define LED_OC1B PIN_TO_LED(PIN_3)	/* OC1B == PB4 == pin 3 */

/* The actual update of OCRxx happens in the timer interrupt */
static inline void set_led(enum led led, enum led_state state)
{
    _timer.led.mode[led] = state;
}
static inline void set_all_led(enum led_state state)
{
    for (uint8_t i = 0; i < sizeof(_timer.led.mode); i++)
	_timer.led.mode[i] = state;
}

#endif /* LED_H */
