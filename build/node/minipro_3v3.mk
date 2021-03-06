BANO_AS := avr-gcc
BANO_CC := avr-gcc
BANO_LD := avr-gcc
BANO_OBJCOPY := avr-objcopy
BANO_OBJDUMP := avr-objdump
BANO_AVRDUDE := avrdude

NRF_DIR ?= $(BANO_DIR)/../nrf
CRYPTOLIB_DIR ?= $(BANO_DIR)/../crypto-lib

BANO_RAND := $(BANO_DIR)/util/rand/rand.sh

ifeq ($(BANO_NODE_ADDR),)
 BANO_NODE_ADDR := $(shell $(BANO_RAND) -f uint32 -n 1)
endif

ifeq ($(BANO_NODE_SEED),)
 BANO_NODE_SEED := $(shell $(BANO_RAND) -f uint32 -n 1)
endif

ifeq ($(BANO_CIPHER_ALG),xtea)
 BANO_CIPHER_ALG := BANO_CIPHER_ALG_XTEA
 CRYPTOLIB_C_FLAGS := -I$(CRYPTOLIB_DIR)/xtea
 BANO_C_FILES += $(CRYPTOLIB_DIR)/xtea/xtea.c
else ifeq ($(BANO_CIPHER_ALG),aes)
 BANO_CIPHER_ALG := BANO_CIPHER_ALG_AES
 CRYPTOLIB_C_FLAGS := -I$(CRYPTOLIB_DIR)/aes
 CRYPTOLIB_C_FLAGS += -I$(CRYPTOLIB_DIR)/gf256mul
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes128_dec.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes128_enc.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes_dec.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes_enc.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes_invsbox.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes_keyschedule.c
 BANO_C_FILES += $(CRYPTOLIB_DIR)/aes/aes_sbox.c
 BANO_S_FILES += $(CRYPTOLIB_DIR)/gf256mul/gf256mul.S
else
 BANO_CIPHER_ALG := BANO_CIPHER_ALG_NONE
 CRYPTOLIB_C_FLAGS :=
endif

ifneq ($(BANO_CIPHER_ALG),BANO_CIPHER_ALG_NONE)
 ifeq ($(BANO_CIPHER_KEY),)
  BANO_CIPHER_KEY := $(shell $(BANO_RAND) -f uint8 -n 16)
 endif
endif

ifeq ($(BANO_NODL_ID),)
 BANO_NODL_ID := 0x00000000
endif

BANO_O_FILES := $(BANO_C_FILES:.c=.o) $(BANO_S_FILES:.S=.o)

BANO_S_FLAGS := -mmcu=atmega328p

BANO_C_FLAGS := -g -Wall -O2 -mmcu=atmega328p -Wno-unused-function -DF_CPU=8000000L
BANO_C_FLAGS += -I$(BANO_DIR)/..
BANO_C_FLAGS += -I$(NRF_DIR)
BANO_C_FLAGS += $(CRYPTOLIB_C_FLAGS)
BANO_C_FLAGS += -DBANO_CONFIG_NODE_ADDR=$(BANO_NODE_ADDR)
BANO_C_FLAGS += -DBANO_CONFIG_NODE_SEED=$(BANO_NODE_SEED)
BANO_C_FLAGS += -DBANO_CONFIG_CIPHER_ALG=$(BANO_CIPHER_ALG)
BANO_C_FLAGS += -DBANO_CONFIG_CIPHER_KEY=$(BANO_CIPHER_KEY)
BANO_C_FLAGS += -DBANO_CONFIG_NODL_ID=$(BANO_NODL_ID)

BANO_L_FLAGS := -mmcu=atmega328p -Wl,-Map,main.map

.PHONY: clean upload

all: main.elf lst text eeprom

%.o: %.c
	$(BANO_CC) $(BANO_C_FLAGS) -c -o $@ $^

%.o: %.S
	$(BANO_AS) $(BANO_S_FLAGS) -c -o $@ $^

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
