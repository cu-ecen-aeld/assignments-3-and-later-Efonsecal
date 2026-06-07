CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Werror -D_GNU_SOURCE=200809L
LDFLAGS := -lpthread

default: all

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket $(LDFLAGS)

clean:
	-rm -f aesdsocket *.o

.PHONY: all clean