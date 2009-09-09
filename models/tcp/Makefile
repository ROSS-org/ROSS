LIBDIR  := ../../ross/
CC      := $(shell $(LIBDIR)/ross-config --cc)
CFLAGS  := -g -Wall
OPTIONS := $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS := $(shell $(LIBDIR)/ross-config --ldflags)

TCP_OBJECTS=\
	tcp-main.o\
	tcp-globals.o \
	tcp-init.o \
	new-rf-to-ross.o \
	tcp-host.o \
	tcp-host-rc.o \
	tcp-router.o \
	tcp-router-rc.o \
	tcp-util.o 

.c.o:
	$(QUIET_CC)$(CC) $(CFLAGS) $(OPTIONS) -c $<

all:	tcp

tcp: $(TCP_OBJECTS)
	$(CC) $(CFLAGS) $(OPTIONS) -o tcp $(TCP_OBJECTS) $(LDFLAGS)

clean:
	rm -f tcp*.o
	rm -f tcp






