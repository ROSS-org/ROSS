This is a test program for the ROSS memory interface.  It will only be
compiled as part the ROSS build if the -DMEMORY=yes option is provided to
cmake.

To execute the program in parallel mode using 16 MPI tasks, do:

mpirun -np 16 ./memory --nlp=16384 --end=1024.0 --synch=3 --memory=65536 --batch=8 --gvt-interval=512 --report-interval=0.10 --clock-rate=2100000000.0
