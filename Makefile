PROJ    = # Project name here

MCU     = atmega328p
FREQ    = 8000000

CC      = avr-gcc
CFLAGS  = -O2 -g -DF_CPU=$(FREQ) -W -Wall -Werror -mmcu=$(MCU)
OBJCOPY = avr-objcopy
PERL    = perl

.SUFFIXES: .c .o .S .s .i .a .asm .elf .hex .bin .zip

CSRC = # List of C files
GENC = # List of generated C files
OBJS = $(patsubst %.c,%.o,$(CSRC) $(GENC))
LIBS =
HEADERS = # List of header files

all: $(PROJ).zip

%.zip: %.flash.bin %.eeprom.bin %.fuses.bin
	zip -9 -o $@ $^

.PRECIOUS: %.elf
$(PROJ).elf: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-Map=$(PROJ).map -o $@ $(OBJS) $(LIBS)

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
	$(CC) $(CFLAGS)  -MMD -MQ '$@' -MF '.$(@F).d' \
		-c -Wa,-ahlmns=$*.lst -o $@ $<

.PRECIOUS: %.i
%.i: %.c $(HEADERS)
	$(CC) $(CFLAGS) -E -o $@ $<

.PRECIOUS: %.s
%.s: %.c $(HEADERS)
	$(CC) $(CFLAGS) -S -o $@ $<

.PRECIOUS: %.o
%.o: %.S $(HEADERS)
	$(CC) $(CFLAGS) -MMD -MQ '$@' -MF '.$(@F).d' \
		-c -Wa,-ahlmns=$*.lst -o $@ $<

.PRECIOUS: %.o
%.o: %.asm $(HEADERS)
	$(CC) $(CFLAGS) -MMD -MQ '$@' -MF '.$(@F).d' \
		-x assembler -c -Wa,-ahlmns=$*.lst -o $@ $<

clean:
	rm -f *.[soi] *.elf *.bin *.hex *.lst *.map *.zip $(GENC)
	rm -f .*.d

spotless: clean
	rm -f *~ \#* *.bak
	rm -rf gen

-include .*.d
