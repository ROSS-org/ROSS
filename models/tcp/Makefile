CFLAGS = -g -I../../ross -L../../ross

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

all:	tcp

tcp: $(TCP_OBJECTS)
	gcc $(CFLAGS) -o tcp $(TCP_OBJECTS) -lROSS -lpthread -lm 

clean:
	rm -f tcp*.o
	rm -f tcp






