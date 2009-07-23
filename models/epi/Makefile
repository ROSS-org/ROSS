LIBDIR	:= ../../ross
CFLAGS	:= -g -I.
CC	:= $(shell $(LIBDIR)/ross-config --cc)
OPTIONS	:= $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS	:= $(shell $(LIBDIR)/ross-config --ldflags)

-include config.mak

BASIC_OBJS = \
	queue-heap.o	\
	epi-timer.o	\
	epi-agent.o	\
	epi-global.o	\
	epi-stats.o	\
	epi-model.o	\
	epi-seir.o	\
	epi-report.o	\
	epi.o		\

HEADERS = \
	epi-agent.h	\
	epi-types.h	\
	epi-extern.h	\
	epi.h		\

ifndef V
	QUIET_CC = @echo '    ' CC $@;
endif

.c.o:
	$(QUIET_CC)$(CC) $(CFLAGS) $(OPTIONS) -c $<

all: epi
epi: $(BASIC_OBJS) $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(CC) $(CFLAGS) $(OPTIONS) -o epi $(BASIC_OBJS) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f epi
