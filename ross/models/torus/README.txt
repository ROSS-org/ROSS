1- Running the torus model in ROSS synch=1 mode (serial)
./torus --sync=1 --end=10000 
- By default this configuration injects 10 packets per node with uniform random traffic
on 512 torus nodes of a 5D torus.

2- Running the torus model in synch=3/2 (optimistic or conservative) modes
mpirun -np 8 ./torus --sync=3 --end=10000 --traffic=1 (uniform random traffic, default) 
mpirun -np 8 ./torus --sync=3 --end=10000 --traffic=2 (matrix transpose traffic)
mpirun -np 8 ./torus --sync=3 --end=10000 --traffic=3 ( nearest neighbor traffic)
mpirun -np 8 ./torus --sync=3 --end=10000 --traffic=4 (diagonal traffic pattern)
