CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc
LDLIBS = -lcurl -pthread -lz

PASTER2 = paster2

default: all

all: $(PASTER2)


$(PASTER2): $(PASTER2).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)


clean:
	rm -f $(PASTER2) *~