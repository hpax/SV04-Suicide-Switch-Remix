#ifndef EEPROM_H
#define EEPROM_H

/*
 * EEPROM structure. This can be modified independently of the rest
 * of the firmware, and so does not require the firmware to be recompiled
 * in many cases.
 *
 * Byte 0: default 0x3b
 *   Signal polarity (0 = active high, 1 = active low)
 *   Bit 0: Blue LED
 *   Bit 1: Green LED
 *   Bit 2: Solid state relay
 *   Bit 3: Button input
 *   Bit 4: Red LED
 *   Bit 5: Off signal from mainboard
 *
 * For the LEDs, configure the EEPROM as follows and set JP1 accordingly:
 *   - Common anode:   set the bits 0, 1, and 4 to 1 (active low)
 *   - Common cathode: set bits 0, 1, and 4 to 0 (active high)
 *
 * Most SSRs are active high, however, most power supplies with a control
 * input are active low. Set bit 2 accordingly.
 *
 * The button and off signal inputs should normally always be active low.
 *
 * Byte 1: default 7, unit = 260 ms
 *   For flashing LEDs, flash speed minus 1. For example:
 *      0 -  260 ms (3.85 Hz)
 *      1 -  520 ms (1.92 Hz)
 *      3 - 1040 ms (0.96 Hz)
 *      7 - 2080 ms (0.48 Hz)
 *     15 - 4160 ms (0.24 Hz)
 *
 * Byte 2: default 2, unit = 66 ms
 *   Time after AC applied before the button input is considered stable.
 *
 * Byte 3: default 32 (0x20), unit = 66 ms
 *   Time after power on before a power off signal from the mainboard is
 *   considered valid (to avoid spurious power off signals during
 *   mainboard boot.)
 *
 * Byte 4: default 4, unit = 66 ms
 *   Time the button needs to be pressed before the press is considered
 *   valid (to prevent an accidental bump.)
 *
 * Byte 5: default 48 (0x30), unit = 66 ms
 *   Time before a button press is cancelled (the "changed my mind" feature.)
 */

struct eeprom {
    uint8_t polarity;		/* Polarity control */
    uint8_t flash_speed;
    uint8_t min_delay;
    uint8_t min_off_delay;
    uint8_t button_press;
    uint8_t button_cancel;
};
extern struct eeprom ee;	/* Copy of the eeprom in memory */

#endif
