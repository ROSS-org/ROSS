LIBDIR  := ../../ross
CC	:= $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -g -Wall -I.
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: airport
clean:
	rm -f airport
	rm -f *.o *~ core*


airport: airport.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) airport.c -o airport $(LDFLAGS)
