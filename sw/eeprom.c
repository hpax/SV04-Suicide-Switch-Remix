

#ifdef EEPROM

/*
 * Interrupt-driven EEPROM handler. Two copies of the EEPROM are cached
 * in memory: one which reflects the current desired state, and one which
 * reflects the current programmed state.
 */
struct eeprom_struct {
    bool power_status;
};
static EEMEM struct eeprom_struct eeprom;
static volatile struct eeprom_struct ee; /* Data requested to eeprom */
static struct eeprom_struct eec; /* Copy of the programmed eeprom */
static volatile uint8_t ee_update_offset;

ISR(EE_RDY_vect, ISR_BLOCK)
{
    uint8_t offs = ee_update_offset;
    bool done = false;

    while (!done) {
	const volatile uint8_t * const eep = (const uint8_t *)&ee;
	uint8_t * const eecp = (uint8_t *)&eec;
	uint8_t o = offs;

	offs++;
	if (offs == (uint8_t)sizeof eeprom) {
	    EECR &= ~(1 << EERIE);
	    done = true;
	}

	uint8_t v = eep[o];
	if (v != eecp[o]) {
	    uint16_t a = (size_t)&eeprom + offs;
	    EEARH = a >> 8;
	    EEARL = (uint8_t)a;
	    EEDR = v;
	    EECR |= 1 << EEMPE;
	    EECR |= 1 << EEPE;
	    done = true;
	}
    }
    ee_update_offset = offs;
}

static inline void update_eeprom(void)
{
    ee_update_offset = 0;
    EECR |= 1 << EERIE;
}
static inline bool eeprom_done(void)
{
    return !(EECR & ((1 << EERIE)|(1 << EEPE)));
}

#endif
