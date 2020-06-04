CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -O2
CC=gcc
LDLIBS = -lcurl -lz -pthread # "curl-config --libs" output 


PASTER = paster

default: paster

paster: $(PASTER)

$(PASTER): $(PASTER).c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(PASTER) *~