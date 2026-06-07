CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -D_GNU_SOURCE=200809L
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread

default: all

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f $(TARGET) *.o

.PHONY: all clean