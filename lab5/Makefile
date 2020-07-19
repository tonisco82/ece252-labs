# Makefile, ECE252
CFLAGS_LIBS = $(shell pkg-config --cflags libxml-2.0 libcurl)
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)

CFLAGS?=-std=gnu99 -g -Wall $(CFLAGS_LIBS) $(CFLAGS_CURL) $(CFLAGS_XML2)
CC=gcc

LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = -lcurl -lz $(LDLIBS_XML2) $(LDLIBS_CURL) 

FINDPNG3 = findpng3

default: all

all: $(FINDPNG3)

$(FINDPNG3): $(FINDPNG3).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *~ *.d *.o $(FINDPNG3) *.png *.html