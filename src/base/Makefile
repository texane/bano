CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip
CFLAGS := -Wall -O2 -static

REPO_DIR ?= $(HOME)/repo

SRCS := \
main.c \
$(REPO_DIR)/bano/src/base/bano_base.c \
$(REPO_DIR)/bano/src/base/bano_node.c \
$(REPO_DIR)/bano/src/base/bano_nodl.c \
$(REPO_DIR)/bano/src/base/bano_string.c \
$(REPO_DIR)/bano/src/base/bano_parser.c \
$(REPO_DIR)/bano/src/base/bano_timer.c \
$(REPO_DIR)/bano/src/base/bano_list.c \
$(REPO_DIR)/bano/src/base/bano_dict.c \
$(REPO_DIR)/bano/src/base/bano_cipher.c \
$(REPO_DIR)/bano/src/base/bano_socket.c \
$(REPO_DIR)/bano/src/base/bano_socket_snrf.c

CFLAGS += -I.
CFLAGS += -I$(REPO_DIR)/bano/src/common
CFLAGS += -I$(REPO_DIR)/bano/src/base
CFLAGS += -DBANO_CONFIG_PERROR

SNRF_DIR := $(REPO_DIR)/nrf/snrf
CFLAGS += -I$(REPO_DIR)/nrf/snrf/src/host
CFLAGS += -I$(REPO_DIR)/nrf/snrf/src/common
LFLAGS += -L$(REPO_DIR)/nrf/snrf/src/host
LIBS += -lsnrf

# httpd server
# use mongoose version 35bd2b603251b40d7dce606b565d39f6a6b6d8f2
MG_DIR ?= ./third/mongoose
SRCS += $(MG_DIR)/mongoose.c
SRCS += $(REPO_DIR)/bano/src/base/bano_httpd.c
SRCS += $(REPO_DIR)/bano/src/base/bano_socket_httpd.c
SRCS += $(REPO_DIR)/bano/src/base/bano_html.c
CFLAGS += -DBANO_CONFIG_HTTPD -I$(MG_DIR) -pthread
LIBS += -lpthread

# camera support
SRCS += $(REPO_DIR)/bano/src/base/bano_cam.c
SRCS += $(REPO_DIR)/bano/src/base/bano_bmp.c
CFLAGS += -DBANO_CONFIG_CAM

OBJS := $(SRCS:.c=.o)

all: a.out

$(SNRF_DIR)/src/host/libsnrf.a:
	cd $(SNRF_DIR)/src/host && make

a.out:	$(SNRF_DIR)/src/host/libsnrf.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS) $(LIBS)
	$(STRIP) $@
	sudo chown root a.out
	sudo chmod 4755 a.out

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tar: a.out
	@mkdir -p bano > /dev/null
	@cp a.out bano > /dev/null
	@cp -rf conf bano > /dev/null
	@cp -rf nodl bano > /dev/null
	@cp -rf sh bano > /dev/null
	@tar czvf bano.tar.gz bano > /dev/null

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: tar all clean fclean $(SNRF_DIR)/src/host/libsnrf.a
