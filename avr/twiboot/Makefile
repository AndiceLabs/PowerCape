PRG            = twiboot
OBJ            = main.o
MCU_TARGET     = atmega328p
OPTIMIZE       = -Os

ifeq ($(MCU_TARGET), atmega88p)
BOOTLOADER_START=0x1800
AVRDUDE_MCU=m88p
LFUSE=0xe2
HFUSE=0xd7
EFUSE=0x00
endif
ifeq ($(MCU_TARGET), atmega168p)
BOOTLOADER_START=0x3800
AVRDUDE_MCU=m168p
LFUSE=0xe2
HFUSE=0xd7
EFUSE=0x00
endif
ifeq ($(MCU_TARGET), atmega328p)
BOOTLOADER_START=0x7800
AVRDUDE_MCU=m328p
LFUSE=0xe2
HFUSE=0xd2
EFUSE=0x07
endif

DEFS           = -DAPP_END=$(BOOTLOADER_START)
LIBS           =

# Override is only needed by avr-lib build system.
override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map,--section-start=.text=$(BOOTLOADER_START)

CC             = avr-gcc
OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump
SIZE           = avr-size

all: $(PRG).elf lst text
	$(SIZE) -x -A $(PRG).elf

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(MAKEFILE_LIST)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(PRG).lst $(PRG).map $(PRG).elf $(PRG).hex $(PRG).bin

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

text: hex bin

hex:  $(PRG).hex
bin:  $(PRG).bin

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

program: text
	avrdude -c avrisp2 -P usb -p $(AVRDUDE_MCU) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m
	avrdude -c avrisp2 -P usb -p $(AVRDUDE_MCU) -U eeprom:w:0xff,0xff,0x00,0x41,0x31:m
#	avrdude -c avrisp2 -P usb -p $(AVRDUDE_MCU) -U eeprom:w:0xff,0xff,0x01,0x41,0x32:m
#	avrdude -c avrisp2 -P usb -p $(AVRDUDE_MCU) -U eeprom:w:0xff,0xff,0x02,0x42,0x30:m
	avrdude -c avrisp2 -P usb -p $(AVRDUDE_MCU) -U flash:w:$(PRG).hex

#fuses:
#	avrdude -c dragon_isp -P usb -p $(AVRDUDE_MCU) -U lfuse:w:0xc2:m
#	avrdude -c dragon_isp -P usb -p $(AVRDUDE_MCU) -U hfuse:w:0xdd:m
#	avrdude -c dragon_isp -P usb -p $(AVRDUDE_MCU) -U efuse:w:0xfa:m
