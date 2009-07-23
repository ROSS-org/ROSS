CFLAGS=-g -Wall -I. -Iross -Lross
#CFLAGS=-O6 -Wall -I.  -Iross -Lross

OBJECTS=\
	tcp-global.c \
	tcp-main.c \
	tcp-process.c \
	tcp-rc-process.c \
	tcp-rc-timeout.c \
	tcp-routing.c \
	tcp-timeout.c \
	tcp-update.c \
	tcp-util.c \
	tcp-init.c \
	tcp.c \
	ip.c \
	ip-downstream.c \
	ip-global.c \
	ip-main.c \
	ip-packet.c \
	ip-rc-downstream.c \
	ip-upstream.c \
	ip-init.c \
	tcp-ip-global.c \
	ip-routing.c \
	tcp-timer.c \
	tw-util.c

all: tcp-ip

tcp-ip: $(OBJECTS)
	gcc $(CFLAGS) -o main tcp-ip.c $(OBJECTS) -lROSS -lm -lpthread

clean:
	rm -f *.o
	rm -f main
