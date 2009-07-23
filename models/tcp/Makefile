CFLAGS = -g -I../../src -L../../lib

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

all:	tcp-par\
	tcp-seq

tcp-par: $(TCP_MAIN_PAR) $(TCP_OBJECTS)
	gcc $(CFLAGS) -o tcp-par $(TCP_OBJECTS) -lROSSpar -lpthread -lm 

tcp-seq: $(TCP_MAIN_SEQ) $(TCP_OBJECTS)
	gcc $(CFLAGS) -o tcp-seq $(TCP_OBJECTS) -lROSSseq -lpthread -lm 

clean:
	rm -f tcp*.o
	rm -f tcp






