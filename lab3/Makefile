CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc

TEMP = temp
PASTER = paster

default: all

all: $(PASTER)

$(PASTER): $(PASTER).c
	$(CC) $(CFLAGS) -o $@ $< -lz

clean:
	rm -f $(PASTER) *~