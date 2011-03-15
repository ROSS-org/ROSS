LIBDIR  := ../../ross/
CC      := $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -I. -Wall -g
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: memory
clean:
	rm -f memory
	rm -f *.o *~ core*

memory: memory.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) memory.c -o memory $(LDFLAGS)
