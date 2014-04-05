BANO_CC := avr-gcc
BANO_LD := avr-gcc
BANO_OBJCOPY := avr-objcopy
BANO_OBJDUMP := avr-objdump
BANO_AVRDUDE := avrdude

BANO_O_FILES := $(BANO_C_FILES:.c=.o)

NRF_DIR ?= $(BANO_DIR)/../nrf

BANO_RAND := $(BANO_DIR)/util/rand/a.out

ifeq ($(BANO_NODE_ADDR),)
 BANO_NODE_ADDR = $(shell $(BANO_RAND) -f uint32 -n 1)
endif

ifeq ($(BANO_NODE_SEED),)
 BANO_NODE_SEED = $(shell $(BANO_RAND) -f uint32 -n 1)
endif

ifeq ($(BANO_NODE_KEY),)
 BANO_NODE_KEY = $(shell $(BANO_RAND) -f uint8 -n 16)
endif

# use lazy evaluation to build rand
BANO_C_FLAGS = \
-g -Wall -O2 -mmcu=atmega328p -Wno-unused-function \
-DF_CPU=8000000L \
-I$(BANO_DIR)/.. \
-I$(NRF_DIR) \
-DBANO_CONFIG_NODE_ADDR=$(BANO_NODE_ADDR) \
-DBANO_CONFIG_NODE_SEED=$(BANO_NODE_SEED) \
-DBANO_CONFIG_NODE_KEY=$(BANO_NODE_KEY)

BANO_L_FLAGS := -mmcu=atmega328p -Wl,-Map,main.map

.PHONY: clean $(BANO_RAND) upload

all: $(BANO_DIR)/bano/util/rand/a.out main.elf lst text eeprom

$(BANO_DIR)/bano/util/rand/a.out:
	( cd $(BANO_DIR)/util/rand && make )

%.o: %.c
	$(BANO_CC) $(BANO_C_FLAGS) -c -o $@ $^

main.elf: $(BANO_O_FILES)
	$(BANO_LD) $(BANO_L_FLAGS) -o $@ $^

clean:
	rm -f $(BANO_O_FILES) main.elf
	rm -f main.hex main.srec main.lst main.map main.bin
	rm -f main_eeprom.bin main_eeprom.hex main_eeprom.srec

lst: main.lst
%.lst: %.elf
	$(BANO_OBJDUMP) -h -S $< > $@

text: hex bin srec

hex:  main.hex
bin:  main.bin
srec: main.srec

%.hex: %.elf
	$(BANO_OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(BANO_OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(BANO_OBJCOPY) -j .text -j .data -O binary $< $@

eeprom: ehex ebin esrec

ehex:  main_eeprom.hex
ebin:  main_eeprom.bin
esrec: main_eeprom.srec

%_eeprom.hex: %.elf
	$(BANO_OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.srec: %.elf
	$(BANO_OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(BANO_OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }

upload:
	sudo $(BANO_AVRDUDE) \
	-patmega328p -carduino -P/dev/ttyUSB0 -b57600 -D \
	-Uflash:w:main.hex:i
