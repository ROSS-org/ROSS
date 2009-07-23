LIBDIR  := ../../ross
CC	:= $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -g -Wall
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: tmr
clean:
	rm -f tmr
	rm -f *.o *~ core*


tmr: tmr.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) tmr.c -o tmr $(LDFLAGS)
