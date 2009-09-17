# Uncomment for verbose output.
#V=1

########################################################################
# for ARCH select from: 
#   1. BLUE_GENE	-- Blue Gene/L
#
# Otherwise, undefined.
#
#
# for DATA_LAYOUT select from:
#   1. byte_per_bit
#
# Otherwise, undefined.
########################################################################
#ARCH=BLUE_GENE
#DATA_LAYOUT=byte_per_bit

########################################################################
# for QUEUE select from: 
#   1. calendar -- Calendar Queue
#   2. heap     -- Push/Down Heap
#   3. splay    -- Splay Tree
#
# for MAPPING select from:
#   1. none	-- Model defined mapping of LPs to KPs to PEs
#   2. linear	-- Linear mapping of LPs to KPs to PEs
#   3. rrobin	-- Round robin mapping of LPs to KPs to PEs
#   
# for RAND select from:
#   1. clcg4	-- Combined Linear Congruential Generator 4
#
# for MEMORY select from:
#   1. MEMORY -- used memory buffer library
#   2. <empty> -- do not use memory buffer library
#
#########################################################################
QUEUE=splay
MAPPING=linear
RAND=clcg4
#MEMORY=MEMORY

#########################################################################
#
# MPI configuration
#
# for NET select from:
#   1. aggressive	-- try to read during event sends
#   2. conservative	-- do not try to read (probe) during sends
#########################################################################
NETWORK=mpi
THREAD=none
CLOCK=none
GVT=mpi_allreduce
NET=conservative

#
# POSIX threads / sockets
#
#THREAD=pthread
#NETWORK=tcp
#NETWORK_ADDTL=socket-tcp.o
#NETWORK_ADDTL_HDR=socket-tcp.h
#CLOCK=amd64
#GVT=7oclock

#
# SMP Only
#
#THREAD=pthread
#NETWORK=none
#CLOCK=amd64
#GVT=7oclock

#
# Sequential execution
#
#THREAD=none
#NETWORK=none
#CLOCK=none
#GVT=none

CFLAGS = -g -Wall #-DROSS_DO_NOT_PRINT
AR     = ar
RANLIB = ranlib

-include config.mak

BASIC_OBJS = \
	clock-$(CLOCK).o  \
	network-$(NETWORK).o \
	$(NETWORK_ADDTL) \
	hash-quadratic.o \
	queue-$(QUEUE).o \
	rand-$(RAND).o \
	ross-global.o \
	signal-sigaction.o \
	thread-$(THREAD).o \
	tw-event.o \
	gvt-$(GVT).o \
	tw-kp.o \
	tw-lp.o \
	tw-opts.o \
	tw-pe.o \
	tw-timer.o \
	tw-memory.o \
	tw-memoryq.o \
	ross-random.o \
	tw-sched.o \
	tw-setup.o \
	tw-signal.o \
	tw-state.o \
	tw-stats.o \
	tw-timing.o \
	tw-util.o \
#end BASIC_OBJS

HEADERS = \
	clock-$(CLOCK).h \
	network-$(NETWORK)1.h \
	network-$(NETWORK)2.h \
	$(NETWORK_ADDTL_HDR) \
	hash-quadratic.h \
	gvt-$(GVT).h \
	rand-$(RAND).h \
	ross-extern.h \
	ross-inline.h \
	ross-network.h \
	ross-thread.h \
	ross-types.h \
	ross.h \
	thread-$(THREAD).h \
	tw-opts.h \
	ross-random.h \
	tw-timing.h \
	tw-eventq.h \
#end HEADERS

OPTIONS = \
	-DROSS_MAPPING_$(MAPPING) \
	-DROSS_QUEUE_$(QUEUE) \
	-DROSS_RAND_$(RAND) \
	-DROSS_NETWORK_$(NETWORK) \
	-DROSS_CLOCK_$(CLOCK) \
	-DROSS_THREAD_$(THREAD) \
	-DROSS_GVT_$(GVT) \
	-DROSS_NET_$(NET) \
	-DROSS_RAND_$(RAND) \
	-DROSS_$(MEMORY) \
	-DARCH_$(ARCH) \
#end OPTIONS

TRACK_VARS = \
	QUEUE=$(QUEUE) \
	RAND=$(RAND) \
	THREAD=$(THREAD) \
	NETWORK=$(NETWORK) \
	NET=$(NET) \
	CLOCK=$(CLOCK) \
	GVT=$(GVT) \
	MEMORY=$(MEMORY) \
	ARCH=$(ARCH) \
	$(subst ','\'',CC=$(CC)) \
	$(subst ','\'',CFLAGS=$(CFLAGS)) \
	$(subst ','\'',OPTIONS=$(OPTIONS)) \
#end TRACK_VARS

LIBS := -lm

ifeq ($(NETWORK),mpi)
ifeq ($(ARCH),BLUE_GENE)
	CC=mpixlc
	CFLAGS += -O5 -DARCH_$(ARCH)
	OPTIONS := -qtune=440 -qarch=440d $(OPTIONS)
else
	CC = mpicc
endif
else
	CC = gcc
	CFLAGS += -D_GNU_SOURCE
	#CFLAGS += -DROSS_DIST_MANUAL
endif

ifeq ($(THREAD),pthread)
	LIBS += -lpthread
endif
ifeq ($(DATA_LAYOUT),byte_per_bit)
	OPTIONS += -DROSS_byte_per_bit
endif

ifndef V
	QUIET_CC = @echo '    ' CC $@;
	QUIET_AR = @echo '    ' AR $@;
endif

ROSS_CFLAGS = \
	$(CFLAGS) \
	$(OPTIONS) \
	-I. \
	-DROSS_INTERNAL \
	-DROSS_OPTION_LIST='"$(OPTIONS)"' \
#end ROSS_CFLAGS

.c.o:
	$(QUIET_CC)$(CC) $(ROSS_CFLAGS) -c $<
.c.s:
	$(QUIET_CC)$(CC) $(ROSS_CFLAGS) -S -c $<

all: libROSS.a ross-config
clean:
	rm -f *.o .CFLAGS *~
	rm -f libROSS.a ross-config

$(BASIC_OBJS): $(HEADERS) .CFLAGS
libROSS.a: $(BASIC_OBJS)
	$(QUIET_AR)rm -f $@;$(AR) cr $@ $(BASIC_OBJS) && $(RANLIB) $@

ross-config: ross-config.c .CFLAGS
	$(QUIET_CC) \
	  srcdir=`pwd` && \
	  libdir=$$srcdir && \
	  cc=`which '$(subst ','\'',$(CC))'` && \
	  $(CC) -o $@ ross-config.c \
		-DCC='"'$$cc'"' \
		-DCFLAGS='"$(OPTIONS) -I'$$srcdir'"' \
		-DLDFLAGS='"-L'$$libdir' -lROSS $(subst ','\'',$(LIBS))"'

.CFLAGS: .FORCE-CFLAGS
	@VARS='$(TRACK_VARS)' ; \
	if test x"$$VARS" != x"`cat $@ 2>/dev/null`" ; then \
		echo 1>&2 "  * new CFLAGS and other settings" ; \
		echo 1>$@ "$$VARS" ; \
	fi

show-defines:
	@touch empty.c
	@$(CC) -c -E -dM empty.c | sort
	@rm -f empty.c

.PHONY: all clean show-defines
.PHONY: .FORCE-CFLAGS
