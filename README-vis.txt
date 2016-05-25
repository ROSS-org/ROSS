README for ROSS data collection

Currenty two different types of data collection have been added to ROSS.  One collects data after each GVT and outputs to file, while the other collects data for time steps specified by the user.  These two parts are independent of each other, so you can run them both, or only one.  The options for the data collection show under the title "ROSS Stats" when you run --help with a ROSS/CODES model. Currently the data is output as a CSV, but will be changed to binary soon. 


GVT data collection:
After each GVT this outputs the following data to file:
"PE id, GVT, Total All Reduce Calls, total events processed, events aborted, events rolled back, event ties detected in PE queues, efficiency,total remote network events processed, total rollbacks, primary rollbacks, secondary roll backs, fossil collect attempts, net events processed, remote sends, remote recvs"

This collection can be turned on by using --enable-stats=1 at runtime.


Time-stepped data collection:
This is turned on using --time-interval=n where n is some integer for the size of the time steps you want.  It bins simulation time based on the specified time interval and collects statistics for each time step.  The data for a time step is output when the full time step falls below GVT. This is now working correctly, but still needs optimizations made for memory management and computation. 
For each time step, the following is output to file:
"PE id, interval, forward events, reverse events, number of GVT comps, all reduce calls, events aborted, event ties detected in PE queues, remote events, network sends, network recvs, events rolled back, primary rollbacks, secondary roll backs, fossil collect attempts"


There are two runtime options that are common to both data collection methods.  --stats-filename option sets a prefix for the output files.  
The gvt collection data is then saved in a directory called <prefix>-gvt-n and the time stepped data is saved in a directory called <prefix>-interval-n. If you don't specify a file prefix, the folders will be ross-stats-gvt-n and ross-stats-interval-n.

The other option is --pe-per-file.  By default (so you don't need to worry about this option), it prints the data of each PE to separate files.  You can print multiple PEs to the same file, but at the moment, this isn't recommended because it isn't using offsets with the MPI IO calls.  This option will probably be removed in the future with all PEs printing to the same file.   The 'n' on the end of each directory name above is an integer.  If there are more than 100 files (i.e., more than 100 PEs), it creates a new directory to store the next 100 PE files and so on.  This is just a hack at the moment because of issues on the BGQ that were encountered recently.   

Work in progress:
- Add in event level data collection
- Add in a buffer for each type of data collection to decrease IO time
- Optimize memory usage and computation time for time-stepped data collection
- Write output in binary
- Write all PEs to same file

