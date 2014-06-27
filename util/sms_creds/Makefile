MATRIXSSL_DIR := $(HOME)/repo/matrixssl/matrixssl-3-6-1-open
BANO_BASE_DIR := ../../src/base

CC := gcc

CFLAGS := -Wall -O2 -I.
CFLAGS += -I$(MATRIXSSL_DIR) -DLINUX
CFLAGS += -I$(BANO_BASE_DIR)
LFLAGS := -L$(MATRIXSSL_DIR)
LIBS := -lmatrixssl

SRCS := main.c
SRCS += $(BANO_BASE_DIR)/bano_sms.c
SRCS += $(BANO_BASE_DIR)/bano_string.c

OBJS := $(SRCS:.c=.o)

all: a.out

a.out:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: all clean fclean
