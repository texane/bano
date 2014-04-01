CC := gcc
CFLAGS := -Wall -O2
SRCS := main.c
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
