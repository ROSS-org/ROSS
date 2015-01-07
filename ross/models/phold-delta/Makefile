CFLAGS  := -g -Wall
LIBDIR  := ../../ross/
CC	:= $(shell $(LIBDIR)/ross-config --cc)
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

all: phold
clean:
	rm -f phold
	rm -f *.o *~ core*


phold: phold.c $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) phold.c -o phold $(LDFLAGS)
