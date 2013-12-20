1- Running the torus model in ROSS synch=1 mode (serial)
./torus --sync=1 --end=10000 

optional arguments for torus configuration: 
--traffic=uniform/nearest/diagonal
--link_bandwidth=FLOAT (Bandwidth of a torus link, by default 2.0 for 5D torus, 1.428 for 7D and 1.111 for 9D)
--arrive_rate=DOUBLE (inter-arrival time between packets in nanoseconds, by dafault its 200.0)
--vc_size=LONG (size of virtual channel in flits, by default it is 16384) 
--injection_interval=DOUBLE (packets are injected during this interval only, by default its 20000)

2- An example of running torus model 

mpirun -np 8 ./torus --sync=3 --end=20000 --arrive_rate=50.0 --traffic=uniform --link_bandwidth=1.85 --vc_size=16384

3- Common errors:
 * Torus model related errors: If simulation stops by saying 'Packet queued in line dir: dim: buffer space: ' then the VC has gone out of flow. 
   Solution: Either increase the VC size by using the --vc_size parameter or lower the injection rate by increasing the --arrive_rate parameter. 
  
 * ROSS related errors: If the simulation stops by saying 'avl_head is null' it means the simulation has exceede the ROSS default AVL tree size.  
   Solution: Increase the AVL tree size by doing ccmake ROSS/. Usually a size of 2^19 or 2^20 should work.

4- Default torus bandwidth configuration:
   By default, the link bandwidth is set relative to a 5-D torus. If the torus dimension is greater than a 5-D, the link bandwidth is automatically adjusted for
these configurations. 
