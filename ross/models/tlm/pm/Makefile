LIBDIR	:= ../rm
CFLAGS	:= -g -I. -std=c99
CC	:= $(shell $(LIBDIR)/rm-config --cc)
OPTIONS := $(shell $(LIBDIR)/rm-config --cflags)
LDFLAGS	:= $(shell $(LIBDIR)/rm-config --ldflags)

-include config.mak

BASIC_OBJECTS=\
	pm-global.o	\
	pm-init.o	\
	pm-stats.o	\
	pm-node.o	\
	pm-helper.o	\
	pm-pe.o		\
	pm.o		\

HEADERS=\
	pm.h		\
	pm-extern.h	\
	pm-types.h	\

	QUIET_CC = @echo '    ' CC $@;

ifndef V
	QUIET_CC = @echo '    ' CC $@;
endif

.c.o:
	$(QUIET_CC)$(CC) $(CFLAGS) $(OPTIONS) -c $<

all: pm
clean:
	rm -f *.o
	rm -f tlm

pm: $(BASIC_OBJECTS)
	$(CC) $(CFLAGS) $(OPTIONS) $(BASIC_OBJECTS) -o tlm $(LDFLAGS)
