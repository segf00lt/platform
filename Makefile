CFLAGS = -g -Wall -Wpedantic -Werror
LDFLAGS = -lraylib -lm

all:
	$(CC) $(CFLAGS) $(LDFLAGS) platform.c -o platform

.PHONY: all
