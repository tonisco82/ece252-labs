CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc

CATPNG = catpng
FINDPNG = findpng
PNGINFO = pnginfo

default: all

all: $(FINDPNG) $(PNGINFO)

$(FINDPNG): $(FINDPNG).c
	$(CC) $(CFLAGS) -o $@ $<

$(PNGINFO): $(PNGINFO).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(CATPNG) $(CATPNG) $(FINDPNG) $(PNGINFO) *.o *~