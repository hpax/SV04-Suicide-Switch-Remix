#ifndef EEPROM_H
#define EEPROM_H

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
