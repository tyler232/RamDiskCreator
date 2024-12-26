CC = gcc
CFLAGS = -Wall -g
TARGET = bin/rdgen
SRCS = ramdisk.c

all: $(TARGET)

$(TARGET): $(SRCS)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -rf bin


