PRG	    = main
OBJ	    = main.o

MCU_TARGET     = atmega328p
OPTIMIZE       = -O2

DEFS	   = -DF_CPU=8000000L
LIBS	   =

# You should not have to change anything below here.

CC	     = avr-gcc

# Override is only needed by avr-lib build system.

REPO_DIR ?= $(HOME)/repo

BANO_DIR := $(REPO_DIR)
BANO_CFLAGS = \
-I$(BANO_DIR) \
-DBANO_CONFIG_NODE_ADDR=$(shell $(BANO_DIR)/bano/util/rand/a.out) \
-DBANO_CONFIG_NODE_SEED=$(shell $(BANO_DIR)/bano/util/rand/a.out)

NRF_DIR := $(REPO_DIR)/nrf
NRF_CFLAGS := -I$(NRF_DIR)

CFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS) -Wno-unused-function \
$(BANO_CFLAGS) $(NRF_CFLAGS)
LDFLAGS = -mmcu=$(MCU_TARGET) $(DEFS) -Wl,-Map,$(PRG).map

OBJCOPY	:= avr-objcopy
OBJDUMP	:= avr-objdump

.PHONY: clean $(BANO_DIR)/bano/util/rand/a.out

all: $(BANO_DIR)/bano/util/rand/a.out $(PRG).elf lst text eeprom

$(BANO_DIR)/bano/util/rand/a.out:
	(cd $(BANO_DIR)/bano/util/rand && make)

$(PRG).elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# dependency:
demo.o: main.c

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak 
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

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

# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV		 = fig2dev
EXTRA_CLEAN_FILES       = *.hex *.bin *.srec

dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@

