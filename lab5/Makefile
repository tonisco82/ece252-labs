# Makefile, ECE252
CFLAGS_LIBS = $(shell pkg-config --cflags libxml-2.0 libcurl)
CFLAGS_CURL = $(shell curl-config --cflags)

CFLAGS?=-std=gnu99 -g -Wall $(CFLAGS_LIBS) $(CFLAGS_CURL)
CC=gcc

LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = -lcurl -lz $(LDLIBS_CURL) 

FINDPNG3 = findpng3

default: all

all: $(FINDPNG3)

$(FINDPNG3): $(FINDPNG3).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *~ *.d *.o $(FINDPNG3) *.png *.html