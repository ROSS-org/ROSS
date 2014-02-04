cxx := mpicxx
cc := mpicc

all: regress_test

regress_test:  regress_test.o phastaIO.o
	$(cxx)  regress_test.o phastaIO.o -g -o regress_test 
phastaIO.o:
	$(cxx)  -g -DMPICH_IGNORE_CXX_SEEK -c ../phastaIO.cc -o phastaIO.o
regress_test.o:
	$(cxx)  -g -DMPICH_IGNORE_CXX_SEEK  -c regress_test.cc -o regress_test.o 
clean:
	rm -rf *.o regress_test
