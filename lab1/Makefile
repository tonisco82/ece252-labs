CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc

CATPNG = catpng
FINDPNG = findpng
PNGINFO = pnginfo

default: all

all: $(FINDPNG) $(PNGINFO) $(CATPNG)

$(FINDPNG): $(FINDPNG).c
	$(CC) $(CFLAGS) -o $@ $<

$(PNGINFO): $(PNGINFO).c
	$(CC) $(CFLAGS) -o $@ $<

$(CATPNG): $(CATPNG).c
	$(CC) $(CFLAGS) -o $@ $< -lz

clean:
	rm -f $(FINDPNG) $(PNGINFO) $(CATPNG) *.o *~