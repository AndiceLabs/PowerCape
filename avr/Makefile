PRG            = power
MAJOR          = 1
MINOR          = 1
TARGET         = atmega328p
CPUCLK         = 8000000
OBJ            = main.o board.o twi_slave.o bb_i2c.o ina3221.o registers.o eeprom.o
OPTIMIZE       = -Os
DAY            = $(shell date +%-d)
MONTH          = $(shell date +%-m)
YEAR           = $(shell date +%-y)
BUILD          = -DDAY=$(DAY) -DMONTH=$(MONTH) -DYEAR=$(YEAR)
DEFS           = -I. -DF_CPU=$(CPUCLK) $(BUILD) -DVER_MAJ=$(MAJOR) -DVER_MIN=$(MINOR)
#DEFS           += -DDEBUG
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

all: $(PRG).elf lst text

combined.hex: $(PRG).hex
	srec_cat $(PRG).hex -intel -offset 0 twiboot/twiboot.hex -intel -offset 0x7800 -o combined.srec

full: eeprom fuses combined.srec
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -e -U flash:w:combined.srec:s

program: fuses eeprom $(PRG).hex
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -e -U flash:w:$(PRG).hex:i

eeprom: 
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U eeprom:w:0xff,0xff,0x02,0x50,0x30:m

fuses:
# External crystal oscillator, max start-up time
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD9:m -U lfuse:w:0xF7:m
# Part defaults (Internal RC osc at 1MHz)
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD9:m -U lfuse:w:0x62:m 
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD1:m -U lfuse:w:0x62:m -U efuse:w:0x07:m
# Internal RC osc at 8MHz
#	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xDF:m -U lfuse:w:0xE2:m
	$(ISP_PROG) $(ISP_FLAGS) -P $(ISP_PORT) -U hfuse:w:0xD1:m -U lfuse:w:0xE2:m -U efuse:w:0x07:m

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

