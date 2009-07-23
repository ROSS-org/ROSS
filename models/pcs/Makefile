LIBDIR  := ../../projects/ross-4.0/
CC	:= $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -g -Wall
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: pcs
clean:
	rm -f pcs
	rm -f *.o *~ core*


pcs: pcs.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) pcs.c -o pcs $(LDFLAGS)
