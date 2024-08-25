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

/* speed = divider of 260 ms - 1 */
static inline void set_flash_speed(uint8_t speed)
{
    _timer.led.flash_speed = speed;
}

#endif /* LED_H */
