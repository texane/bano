CC := gcc
CFLAGS := -Wall -O2

REPO_DIR ?= $(HOME)/repo

SRCS := \
main.c \
$(REPO_DIR)/bano/src/base/bano_base.c \
$(REPO_DIR)/bano/src/base/bano_list.c \
$(REPO_DIR)/bano/src/base/bano_socket_snrf.c

OBJS := $(SRCS:.c=.o)

CFLAGS += -I$(REPO_DIR)/bano/src/common -I$(REPO_DIR)/bano/src/base -I.
CFLAGS += -DBANO_CONFIG_PERROR

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
	sudo chown root a.out
	sudo chmod 4755 a.out

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: all clean fclean $(SNRF_DIR)/src/host/libsnrf.a
