LIBDIR  := ../../ross/
CC	:= $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -g -Wall
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: udp
clean:
	rm -f 
	rm -f *.o *~ core*


udp: udp-exp.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) udp-exp.c -o udp $(LDFLAGS)
