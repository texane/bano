MATRIXSSL_DIR ?= ../../src/base/third/matrixssl-3-6-1-open
BANO_BASE_DIR := ../../src/base

CC := $(CROSS_COMPILE)gcc

CFLAGS := -Wall -O2 -I. -static
CFLAGS += -DBANO_CONFIG_PERROR
CFLAGS += -I$(MATRIXSSL_DIR) -DLINUX
CFLAGS += -I$(BANO_BASE_DIR)
LFLAGS := -L$(MATRIXSSL_DIR)
LIBS := -lmatrixssl
DEPS += $(MATRIXSSL_DIR)/libmatrixssl.a

SRCS := main.c
SRCS += $(BANO_BASE_DIR)/bano_sms.c
SRCS += $(BANO_BASE_DIR)/bano_string.c

OBJS := $(SRCS:.c=.o)

all: $(DEPS) a.out

$(MATRIXSSL_DIR)/libmatrixssl.a:
	cd $(MATRIXSSL_DIR) && CROSS=$(CROSS_COMPILE) make

a.out:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: all clean fclean
