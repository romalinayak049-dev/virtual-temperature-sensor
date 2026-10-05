CC = gcc
CFLAGS = -Wall -Wextra -O2

all: monitor

monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c

clean:
	rm -f monitor

.PHONY: all clean
