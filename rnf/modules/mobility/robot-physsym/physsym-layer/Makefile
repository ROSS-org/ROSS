BULLETDIR = /home/binh/bulletSVN
BULLETINC = $(BULLETDIR)/src
BULLETLIB = $(BULLETDIR)/lib

PATHDIR = /home/binh/pathlib
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/linux64/static

CC    = gcc
CFLAGS = -Wall -I. -I.. -I$(PATHINC) -I$(BULLETINC) -O3

all: 
	$(CC) $(CFLAGS) -c physsym.cpp -o physsym.o

# -L$(PATHLIB) -L$(BULLETLIB) -lpath46 -lBulletDynamics -lBulletCollision -lLinearMath -lm -lstdc++

