CC = gcc
CFLAGS = -g -Wall -Wpedantic -Wunused-const-variable -Werror
LDFLAGS = -lraylib -lm

all:
	$(CC) $(CFLAGS) $(LDFLAGS) platform.c -o platform

.PHONY: all
