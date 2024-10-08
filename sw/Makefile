PROJ    = onoff

# Target MCU name and frequency
MCU     = attiny85
FREQ    = 1000000

# MCU name according to various programmers
MCU_MINIPRO = attiny85

# Size of flash and eeprom in bytes (some programmers want them padded to the
# full size of the memory)
FLASHSIZE  = 8192
EEPROMSIZE = 512

CC      = avr-gcc
CFLAGS  = -O2 -g -DF_CPU=$(FREQ) -W -Wall -Werror -mmcu=$(MCU)
OBJCOPY = avr-objcopy
PERL    = perl

.SUFFIXES: .c .o .S .s .i .a .asm .elf .hex .bin .zip

CSRC = onoff.c timer.c
GENC = # List of generated C files
OBJS = $(patsubst %.c,%.o,$(CSRC) $(GENC))
LIBS =

all: $(PROJ).zip

%.zip: %.flash.bin %.eeprom.bin %.fuses.bin %.fuses.conf
	zip -9 -o $@ $^

.PRECIOUS: %.elf
$(PROJ).elf: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-Map=$(PROJ).map -o $@ $(OBJS) $(LIBS)

.PRECIOUS: %.flash.bin
%.flash.bin: %.elf
	$(OBJCOPY) -R .eeprom -R .fuse -O binary \
		--pad-to $(FLASHSIZE) --gap-fill 0xff $< $@

.PRECIOUS: %.eeprom.bin
%.eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 -O binary \
		--pad-to $(EEPROMSIZE) --gap-fill 0xff $< $@

.PRECIOUS: %.fuses.bin
%.fuses.bin: %.elf
	$(OBJCOPY) -j .fuse --set-section-flags=.fuse="alloc,load" \
		--change-section-lma .fuse=0 -O binary $< $@

.PRECIOUS: %.fuses.conf
%.fuses.conf: %.fuses.bin
	$(PERL) -e 'read(STDIN,$$n,3); printf "fuses_lo = 0x%x\nfuses_hi = 0x%x\nfuses_ext = 0x%x\nlock_byte = 0xff\n",unpack("C*",$$n);' < $< > $@

.PRECIOUS: %.o
%.o: %.c
	$(CC) $(CFLAGS)  -MMD -MQ '$@' -MF '.$(@F).d' \
		-c -Wa,-ahlmns=$*.lst -o $@ $<

.PRECIOUS: %.i
%.i: %.c
	$(CC) $(CFLAGS) -E -o $@ $<

.PRECIOUS: %.s
%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<

.PRECIOUS: %.o
%.o: %.S
	$(CC) $(CFLAGS) -MMD -MQ '$@' -MF '.$(@F).d' \
		-c -Wa,-ahlmns=$*.lst -o $@ $<

.PRECIOUS: %.o
%.o: %.asm
	$(CC) $(CFLAGS) -MMD -MQ '$@' -MF '.$(@F).d' \
		-x assembler -c -Wa,-ahlmns=$*.lst -o $@ $<

clean:
	rm -f *.[soi] *.elf *.bin *.hex *.lst *.map *.zip $(GENC)
	rm -f .*.d

spotless: clean
	rm -f *~ \#* *.bak
	rm -rf gen

# Flashing using various programmers

# Not really sure why we need to disable verification
flash_minipro: $(PROJ).zip
	if [ -s $(PROJ).fuses.conf ]; then \
		minipro -p $(MCU_MINIPRO) -c config -w $(PROJ).fuses.conf; \
	fi
	if [ -s $(PROJ).flash.bin ]; then \
		minipro -p $(MCU_MINIPRO) -c code -w $(PROJ).flash.bin; \
	fi
	if [ -s $(PROJ).eeprom.bin ]; then \
		minipro -v -p $(MCU_MINIPRO) -c data -w $(PROJ).eeprom.bin; \
	fi

-include .*.d
