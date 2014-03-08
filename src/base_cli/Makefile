CC := gcc
CFLAGS := -Wall -O2

SRCS := \
main.c \
../base/bano_base.c \
../base/bano_list.c \
../base/bano_socket_snrf.c

OBJS := $(SRCS:.c=.o)

CFLAGS += -I../common -I../base -I.
CFLAGS += -DBANO_CONFIG_PERROR

REPO_DIR ?= $(HOME)/repo
SNRF_DIR := $(REPO_DIR)/nrf/snrf
CFLAGS += -I$(REPO_DIR)/nrf/snrf/src/host
CFLAGS += -I$(REPO_DIR)/nrf/snrf/src/common
LFLAGS += -L$(REPO_DIR)/nrf/snrf/src/host
LIBS += -lsnrf

all: a.out

$(SNRF_DIR)/src/host/libsnrf.a:
	cd $(SNRF_DIR)/src/host && make

a.out:	$(SNRF_DIR)/src/host/libsnrf.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: all clean fclean $(SNRF_DIR)/src/host/libsnrf.a
