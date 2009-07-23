LIBDIR	:= ../../../ross
CC	:= $(shell $(LIBDIR)/ross-config --cc)
CFLAGS	:= -g -Wall -I. -std=c99
OPTIONS	:= $(shell $(LIBDIR)/ross-config --cflags)
LDFLAGS	:= $(shell $(LIBDIR)/ross-config --ldflags)
AR	:= ar
RANLIB	:= ranlib

-include config.mak

BASIC_OBJS = \
	rm-helper.o	\
	rm-interface.o	\
	rm-global.o	\
	rm-particle.o	\
	rm-pe.o		\
	rm-proximity.o	\
	rm-cell.o	\
	rm-grid.o	\
	rc-rm-cell.o	\
	rm.o		\

HEADERS = \
	rm.h		\
	rm-extern.h	\
	rm-types.h	\

TRACK_VARS = \
	$(subst ','\'',CC=$(CC)) \
	$(subst ','\'',CFLAGS=$(CFLAGS)) \
	$(subst ','\'',OPTIONS=$(OPTIONS)) \
	
LIBS	:= $(LDFLAGS)

ifndef V
	QUIET_CC = @echo '    ' CC $@;
	QUIET_AR = @echo '    ' AR $@;
endif

RM_CFLAGS = \
	$(CFLAGS)	\
	$(OPTIONS)	\

.c.o:
	$(QUIET_CC)$(CC) $(RM_CFLAGS) -c $<
.c.s:
	$(QUIET_CC)$(CC) $(RM_CFLAGS) -S -c $<

all: libRM.a rm-config

clean:
	rm -f *.o .CFLAGS
	rm -f libRM.a rm-config

$(BASIC_OBJS): $(HEADERS) .CFLAGS
libRM.a: $(BASIC_OBJS) $(LIBDIR)/libROSS.a $(LIBDIR)/ross-config
	$(QUIET_AR)rm -f $@;$(AR) cr $@ $(BASIC_OBJS) && $(RANLIB) $@

rm-config: rm-config.c .CFLAGS
	$(QUIET_CC) \
		srcdir=`pwd` && \
		libdir=$$srcdir && \
		cc=`which '$(subst ','\'',$(CC))'` && \
		gcc -o $@ rm-config.c \
			-DCC='"'$$cc'"' \
			-DCFLAGS='"-I'$$srcdir' $(OPTIONS)"' \
			-DLDFLAGS='"-L'$$libdir' -lRM $(subst ','\'',$(LIBS))"'

.CFLAGS: .FORCE-CFLAGS
	@VARS='$(TRACK_VARS)' ; \
	if test x"$$VARS" != x"`cat $@ 2>/dev/null`"; then \
		echo 1>&2 "  * new CFLAGS and other settings" ; \
		echo 1>$@ "$$VARS" ; \
	fi

show-defines:
	@touch empty.c
	@$(CC) -c -E -dM empty.c | sort
	@rm -f empty.c

.PHONY: all clean show-defines
.PHONY: .FORCE-CFLAGS
