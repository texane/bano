#
#	Common makefile definitions
#	Copyright (c) INSIDE Secure, 2002-2014. All Rights Reserved.
#
###########################
#
#	Things you can set
#
# 	1. Whether debug build or not
BUILD = release 
#BUILD = debug 

# 	2. Platform (./core/OSDEP/osdep.c)
OSDEP	= POSIX

# 	3. File system support (reading PEM files from disk) 
FILE_SYS = -DMATRIX_USE_FILE_SYSTEM

# 	4. Optional path to cryptoki.h if using PKCS11 
PKCS11_INCL = .

# 	5. Optional path and names of PKCS#11 libraries
PKCS11_LIB =


###################################
#
#	Shouldn't have to touch below
#

DFLAGS  = $(WARNINGS) 

ifdef MATRIX_DEBUG
DFLAGS  += -g -DDEBUG 
STRIP   = test
endif

ifdef PS_OPENSSL
ifndef CROSS
$(error Please define OPENSSL_ROOT)
	OPENSSL_ROOT = 
	DFLAGS += -DUSE_OPENSSL -I$(OPENSSL_ROOT)/include
	STATICS += $(OPENSSL_ROOT)/libcrypto.a
else
	DFLAGS += -DUSE_OPENSSL
	STATICS += -lcrypto
endif
endif

default: $(BUILD) 

debug:
	@$(MAKE) compile "MATRIX_DEBUG = 1"

release:
	@$(MAKE) compile

#
# For cross platform, call using something like this:
#   make TILERA=y
#   make WRT54G=y
#   make WRT54G=1
#   make WRT54G=anything_but_blank
#   make LINUXSTAMP=1 
#   make MARVELL=1 
#
ifdef TILERA
  $(error Please define TILERA_ROOT and TSTACK_DIR)
  ifndef TILERA_ROOT
    TILERA_ROOT = 
  endif
  CROSS      = $(TILERA_ROOT)/bin/tile-
  TSTACK_DIR = 
  LINK_TOKEN_BUILDER = 1
  CFLAGS += -DUSE_HARDWARE_CRYPTO_PKA -DUSE_HARDWARE_CRYPTO_RECORD\
		-DUSE_TILERA_MICA
endif
ifdef WRT54G
  CROSS   = mipsel-openwrt-linux-uclibc-
endif
ifdef LINUXSTAMP
  CROSS   = arm-linux-uclibc-
endif
ifdef MARVELL
  CROSS   = arm-none-eabi-
endif
ifdef ANDROID
  CROSS   = arm-linux-androideabi-
endif
ifdef ANDROIDX86
  CROSS   = i686-android-linux-
endif

SHARED = -shared
STRIP   = strip
LDFLAGS += -lc
CFLAGS += $(DFLAGS) -D$(OSDEP) $(FILE_SYS) -I$(PKCS11_INCL)

ifdef CROSS
  CC      = $(CROSS)gcc		#Assume GCC for cross compiling
  STRIP   = $(CROSS)strip
  AR      = $(CROSS)ar
  LDFLAGS += -fno-builtin -static
else
  CFLAGS += -I/usr/include
endif

O       = .o
SO      = .so
A       = .a
E       =

ifndef CROSS
ifeq ($(shell uname),Darwin)
  STRIP = test
  SO = .dylib
  DFLAGS += -mdynamic-no-pic
  CFLAGS += -isystem -I/usr/include
  ifndef MATRIX_DEBUG
    DFLAGS += -O3
  endif
  #
  # Use the compiler default to figure out 32 or 64 bit mode
  #
  CCARCH = $(shell $(CC) -v 2>&1)
  ifneq (,$(findstring x86_64,$(CCARCH)))
    CFLAGS += -m64 -DPSTM_64BIT -DPSTM_X86_64
    ifeq ($(shell sysctl -n hw.optional.aes),1)
      CFLAGS += -maes -mpclmul -msse4.1
    endif
  else
    LDFLAGS += -read_only_relocs suppress
    ifndef PS_DEBUG
      CFLAGS += -DPSTM_X86
    endif
  endif
else
  #
  # Linux shows the architecture in uname
  #
  CFLAGS += -fomit-frame-pointer
  ifeq ($(shell uname -m),x86_64)
    CFLAGS  += -m64 -DPSTM_64BIT -DPSTM_X86_64 -fPIC 
	ifndef MATRIX_DEBUG
		DFLAGS += -O3
	endif
  else ifeq ($(shell uname -m),i686)
    CFLAGS  += -DPSTM_X86
	ifndef MATRIX_DEBUG
		DFLAGS += -O2
	endif
  else ifeq ($(shell uname -m),tilegx)
	TILERA=1
    LINK_TOKEN_BUILDER = 1
    CFLAGS += -DUSE_HARDWARE_CRYPTO_PKA -DUSE_HARDWARE_CRYPTO_RECORD \
		-DUSE_TILERA_MICA
	ifndef MATRIX_DEBUG
		DFLAGS += -O2
	endif
  endif
  # Check for aes-ni support, will define __AES__ in preprocessor
  ifeq ($(shell cat /proc/cpuinfo | grep -o aes),aes)
    CFLAGS += -maes -mpclmul -msse4.1
  endif
endif
#Cross compile
else
  ifneq (,$(findstring mips,$(CROSS)))
    ifndef PS_DEBUG
      CFLAGS += -DPSTM_MIPS
    endif
  endif
  ifneq (,$(findstring arm,$(CROSS)))
#    CFLAGS += -DPSTM_ARM -mthumb -mcpu=arm9tdmi -mthumb-interwork
    CFLAGS += -DPSTM_ARM -mcpu=arm9tdmi -mthumb-interwork
  endif
  ifneq (,$(findstring i686,$(CROSS)))
#	TODO - for android x86 try to optimize
#    CFLAGS  += -DPSTM_X86
  endif
endif

ifeq ($(shell cc --version | grep -o clang),clang)
WARNINGS = -Wall -Werror -Wno-error=unused-variable -Wno-error=\#warnings
else ifeq ($(shell cc --version | grep -o Tilera),Tilera)
WARNINGS = -Wall
else ifeq (TILERA,"y")
WARNINGS = -Wall -Werror -Wno-error=cpp -Wno-error=unused-variable -Wno-error=unused-but-set-variable
#else
WARNINGS = -Wall
endif

# STROPTS holds a string with current settings for informational display
STROPTS = "Optimizations: $(CC) "
STROPTS +=$(findstring -O0,$(CFLAGS))
STROPTS +=$(findstring -O1,$(CFLAGS))
STROPTS +=$(findstring -O2,$(CFLAGS))
STROPTS +=$(findstring -O3,$(CFLAGS))
STROPTS +=$(findstring -Os,$(CFLAGS))
STROPTS +=$(findstring -g,$(CFLAGS))

ifneq (,$(findstring PSTM_X86_64,$(CFLAGS)))
  STROPTS +=", 64-bit Intel PKA assembly"
else ifneq (,$(findstring PSTM_X86,$(CFLAGS)))
  STROPTS +=", 32-bit Intel PKA assembly"
else ifneq (,$(findstring PSTM_ARM,$(CFLAGS)))
  STROPTS +=", 32-bit ARM PKA assembly"
else ifneq (,$(findstring PSTM_ARM,$(CFLAGS)))
  STROPTS +=", 32-bit MIPS PKA assembly"
endif

ifneq (,$(findstring -maes,$(CFLAGS)))
  STROPTS +=", AES-NI assembly"
endif
