CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc

PASTER2 = paster2

default: all

all: $(PASTER2)


$(PASTER2): $(PASTER2).c
	$(CC) $(CFLAGS) -o $@ $< -lz

$(TEMP): $(TEMP).c
	$(CC) $(CFLAGS) -o $@ $< -lz 


clean:
	rm -f $(PASTER2) *~