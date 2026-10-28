CC      = gcc
CFLAGS  = -Wall -Wextra -O2
TARGET  = VMCacheSim

$(TARGET): main.c config.h milestone1.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c -lm

clean:
	rm -f $(TARGET)

.PHONY: clean