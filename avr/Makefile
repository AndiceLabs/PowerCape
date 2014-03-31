PRG            = powercape
PLATFORM       = powercape
TARGET         = atmega328p
CPUCLK         = 1000000
OBJ            = main.o board.o twi_slave.o registers.o
OPTIMIZE       = -Os
DEFS           = -I. -D$(PLATFORM) -DF_CPU=$(CPUCLK)
LIBS           =

# Override is only needed by avr-lib build system.
CFLAGS         = -g -Wall $(OPTIMIZE) -mmcu=$(TARGET) $(DEFS)
LDFLAGS        = -Wl,-Map,$(PRG).map

CC             = avr-gcc
OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump
SIZE           = avr-size
ISP_PROG       = avrdude
ISP_PORT       = usb
ISP_FLAGS      = -p m328p -c avrisp2

all: $(PRG).elf lst text eeprom

program: $(PRG).hex
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xC9:m -U lfuse:w:0x62:m -U efuse:w:0x05:m
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -e -U flash:w:$(PRG).hex:i

fuses:
# External crystal oscillator, max start-up time
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD9:m -U lfuse:w:0xF7:m
# Part defaults (Internal RC osc at 1MHz)
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD9:m -U lfuse:w:0x62:m 
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xC9:m -U lfuse:w:0x62:m -U efuse:w:0x05:m
# Internal RC osc at 8MHz
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xDF:m -U lfuse:w:0xE2:m

%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@
	$(SIZE) $<

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak 
	rm -rf *.lst *.map *.hex *.srec *.bin $(EXTRA_CLEAN_FILES)

