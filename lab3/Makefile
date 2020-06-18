CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc

TEMP = temp

default: all

all: $(TEMP)

$(TEMP): $(TEMP).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TEMP) *~