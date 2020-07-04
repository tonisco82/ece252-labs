# Makefile, ECE252
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)

CFLAGS?=-std=gnu99 -g -Wall $(CFLAGS_XML2) $(CFLAGS_CURL)
CC=gcc

LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = -lcurl -pthread -lz $(LDLIBS_XML2) $(LDLIBS_CURL) 

FINDPNG2 = findpng2

default: all

all: $(FINDPNG2)


$(FINDPNG2): $(FINDPNG2).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *~ *.d *.o $(FINDPNG2) *.png *.html