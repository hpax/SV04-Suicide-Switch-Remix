PROJ    = abcjoy

MCU     = atmega328p
FREQ    = 8000000

CC      = avr-gcc
CFLAGS  = -O2 -g -DF_CPU=$(FREQ) -W -Wall -Werror -mmcu=$(MCU)
OBJCOPY = avr-objcopy
PERL    = perl

.SUFFIXES: .c .o .elf .hex .bin .zip

CSRC = abcjoy.c
GENC = joytbl.c
OBJS = $(patsubst %.c,%.o,$(CSRC) $(GENC))
HEADERS = abcjoy.h

all: $(PROJ).zip

%.zip: %.flash.bin %.eeprom.bin %.fuses.bin
	zip -9 -o $@ $^

.PRECIOUS: %.elf
$(PROJ).elf: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-Map=$(PROJ).map -o $@ $(OBJS)

.PRECIOUS: %.flash.bin
%.flash.bin: %.elf
	$(OBJCOPY) -R .eeprom -R .fuse -O binary $< $@

.PRECIOUS: %.eeprom.bin
%.eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 -O binary $< $@

.PRECIOUS: %.fuses.bin
%.fuses.bin: %.elf
	$(OBJCOPY) -j .fuse --set-section-flags=.fuse="alloc,load" \
		--change-section-lma .fuse=0 -O binary $< $@

.PRECIOUS: %.o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -Wa,-ahlmns=$*.lst -o $@ $<

joytbl.c: joytbl.pl
	$(PERL) $< > $@ || ( rm -f $@ ; false )

clean:
	rm -f *.o *.elf *.bin *.hex *.lst *.map *.zip $(GENC)

spotless: clean
	rm -f *~ \#* *.bak
