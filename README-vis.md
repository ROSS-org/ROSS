## README for ROSS data collection

Currenty 3 different types of data collection have been added to ROSS: GVT, virtual time sampling, and real time sampling.  At the moment, each part shouldn't be used together, but eventually they will be able to be used in conjunction with each other.  The options for the data collection show under the title "ROSS Stats" when you run --help with a ROSS/CODES model.

There are two runtime options that are common to all data collection methods.  --stats-filename option sets a prefix for the output files.  

### GVT data collection
This collects data after each GVT.  This is output to file for each PE.  Eventually there will also be some data collected at KP and LP granularities.  So far the following data is collected per PE at each GVT:
PE id, GVT, Total All Reduce Calls, total events processed, events aborted, events rolled back, event ties detected in PE queues, efficiency,total remote network events processed, total rollbacks, primary rollbacks, secondary roll backs, fossil collect attempts, net events processed, remote sends, remote recvs

This collection can be turned on by using --enable-gvt-stats=1 at runtime.

### Real Time Sampling
This collects data at real time intervals specified by the user.  It is turned on using --real-time-samp=n, where n is the number of milliseconds per interval.  Right now this collects a lot of information about events (such as total events, net events, rollbacks, remote events), number of GVTs computed, cycle counters for all of the components of ROSS (e.g., for event processing, computing gvt, fossil collection, etc), amount of virtual time that LPs/KPs are ahead of GVT, and memory usage.  

### Virtual Time Sampling
This collects data for virtual time steps specified by the user.  This is turned on using --time-interval=n where n is some integer for the size of the time steps you want.  It bins simulation time based on the specified time interval and collects statistics for each time step.  The data for a time step is output when the full time step falls below GVT. This is essentially working correctly, but still needs a lot of optimization in regards to both memory management and computation. This data collection is a bit more complex than the other two, so it is on hold at the moment.  Currently this can probably be run with small models.  

For each time step, the following is output to file:
PE id, interval, forward events, reverse events, number of GVT comps, all reduce calls, events aborted, event ties detected in PE queues, remote events, network sends, network recvs, events rolled back, primary rollbacks, secondary roll backs, fossil collect attempts

### Output formatting
The GVT and real time collections are pushed to a buffer as they are collected, in order to reduce the amount of I/O accesses.  Currently the buffer is per PE and is 8 MB in size (will eventually be configurable).  After GVT, the buffer's free space is checked.  If there is less than 15% free space, then it is dumped to file in a binary format.  Right now it is just one file of output and a reader for it is being developed in the CODES-vis repo.  In the future we may switch to an already established format, or just further develop what is being used currently.  There is only one file written out per simulation run.

The virtual time collection is output as a human-readable csv and data can be written out at any GVT.  It will eventually be changed to use the buffer and output in binary as well.

### Other notes
There are a couple of other options that show up in the ROSS stats options.
One is --disable-output.  This is for use when examining the perturbation of the data collection on the simulation.  It means that data (for GVT and real time collections) will be pushed to the buffer, but the buffer will never be dumped, so it will just keep overwriting data.  This is so we can measure the effects of the computation of data collection itself without the I/O.

Another option listed is --pe-per-file.  Now this is only relevant for the virtual time data collection and will be removed soon, so it can be ignored.  

Work in progress:
- Add in event level data collection
- Optimize memory usage and computation time for virtual time data collection, and make it work with buffer, output in binary

