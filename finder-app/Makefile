SRC := writer.c
TARGET := writer
OBJS := writer.o
CC := $(CROSS_COMPILE)gcc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) -c $

clean:
	-rm -f *.o $(TARGET) *.elf *.map

.PHONY: all clean