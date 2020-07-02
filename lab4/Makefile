CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc
LDLIBS = -lcurl -pthread -lz

FINDPNG2 = findpng2

default: all

all: $(FINDPNG2)


$(FINDPNG2): $(FINDPNG2).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *~ *.d *.o $(FINDPNG2) *.png *.html