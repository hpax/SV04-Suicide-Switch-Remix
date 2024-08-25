#ifndef PINS_H
#define PINS_H

/* Pinmasks - these are PBx port bit numbers, not physical pins! */
#define HIGH 0
#define LOW  1

/* Mapping between package pins and PBx port numbers */
#define PIN_1			5
#define PIN_2			3
#define PIN_3			4
#define PIN_5			0
#define PIN_6			1
#define PIN_7			2

#define PIN_OFF			PIN_1
#define PIN_OFF_SENSE		LOW
#define PIN_BUTTON		PIN_2
#define PIN_BUTTON_SENSE	LOW
#define PIN_SSR			PIN_7
#define PIN_SSR_SENSE		HIGH

#define PIN_LED_R		PIN_3
#define PIN_LED_B		PIN_5
#define PIN_LED_G		PIN_6

/* LED polarity */
#define LED_COMMON_ANODE	1

/* Deglitch hysteresis in timer ticks (256 µs) */
#define BUTTON_DEGLITCH		256
#define OFF_DEGLITCH		256

/* PBx port numbers, not physical pins! */
#define PINMASK_OFF	(1U << PIN_OFF)
#define PINMASK_BUTTON	(1U << PIN_BUTTON)
#define PINMASK_SSR	(1U << PIN_SSR)
#define PINMASK_LED_R	(1U << PIN_LED_R)
#define PINMASK_LED_B	(1U << PIN_LED_B)
#define PINMASK_LED_G	(1U << PIN_LED_G)

#define PINMASK_LEDS	(PINMASK_LED_R|PINMASK_LED_G|PINMASK_LED_B)

#define PINB_XOR ((PIN_BUTTON_SENSE << PIN_BUTTON)|(PIN_OFF_SENSE << PIN_OFF))

#endif
