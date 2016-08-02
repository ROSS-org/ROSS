## README for ROSS data collection

Currently several different types of data collection have been added to ROSS: GVT, real time sampling, and event-level collection.  
There is also a virtual time sampling that may be available in the future, but I removed the calls
for it within ROSS, since it needs a lot of work before it can really be used. 

Both the GVT and real time collections can be used independently or together.  The options for 
the data collection show under the title "ROSS Stats" when you run `--help` with a ROSS/CODES model.

For all data collection types, you can use `--stats-filename` option to set a prefix for the output files.  
All of the output files are stored in a directory named `stats-output` that is created in the running directory.

### GVT data collection
This collects data after each GVT. The data is currently collected per PE, but eventually there 
will also be some data collected at KP and LP granularities.  
So far the following data is collected per PE at each GVT (in order of output):

PE_ID, GVT, all_reduce_count, events_processed, events_aborted, events_rolled_back, event_ties, total_rollbacks, 
primary_rollbacks, secondary_rollbacks, fossil_collects, network_sends, network_recvs, remote_events, net_events, efficiency

This collection can be turned on by using `--enable-gvt-stats=1` at runtime.

### Real Time Sampling
This collects data at real time intervals specified by the user.  It is turned on using 
`--real-time-samp=n`, where n is the number of milliseconds per interval.  
Right now this collects a lot of information about events (such as total events, net events, 
rollbacks, remote events), number of GVTs computed, cycle counters for all of the components 
of ROSS (e.g., for event processing, computing gvt, fossil collection, etc), 
amount of virtual time that LPs/KPs are ahead of GVT, and memory usage.  

### Event-level data collection
There are two ways to collect events.  One is to collect data only about events that are causing rollbacks.
When an event that should have been processed in the past is received, data about this event is collected (described below).  The other event collection is for all events, which can be turned on for only specific LP types.  
For this collection, ROSS can directly access the source and destination LP IDs for each event, as well as the 
received virtual timestamp of the event.  It will also record the real time that the event is computed at.
Because event types are determined by the model developer, ROSS cannot directly access this.
The user can create a callback function that ROSS will use to collect the event type, and ROSS will then handle storing
the data in the buffer and the I/O.  The benefit to this is that the user can choose to collect other data about the event,
and ROSS will handle this as well.  It's important to remember that this data collection will result in a lot of output, since it's happening per event.  

This is implemented similarly to how the LP type function pointers are implemented.  (As a side note, it is a different
struct, so that non-instrumented ROSS can still be run without requiring any additions to models).

##### Function pointers:
```C
typedef void (*rbev_col_f) (void *msg, char *buffer);
typedef void (*ev_col_f) (void *msg, char *buffer);
```
`msg` is the message being passed.  `buffer` is the pointer to where the data needs to be copied for ROSS to manage.
For instance in the dragonfly CODES model, we can do:
```C
  int type = (int) msg->type;
  memcpy(buffer, &type, sizeof(type));
```
This is just a simple example; we could of course get more complicated with this and save other data.

##### Event type struct for function pointers
```C
typedef struct st_event_collect st_event_collect;
struct st_event_collect{
    rbev_col_f rbev_col; /* function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;      /* size of data collected from model about events causing rollbacks */
    ev_col_f ev_col;     /* function pointer to collect data about all events for given LP */
    size_t ev_sz;        /* size of data collected from model for each event */
};
```
This is the struct where we provide the function pointers to ROSS.  
`rbev_col_f` is the pointer for the event collection for only events causing rollbacks, while `ev_col_f` is for
the full event collection.  
`rbev_sz` and `ev_sz` are the sizes of the data that need to be pushed to the buffer for the rollback causing events, or for all events, respectively.

### Virtual Time Sampling
NOTE: Function calls related to this data collection have been commented out for now.

This collects data for virtual time steps specified by the user.  This is turned on using 
`--time-interval=n` where n is some integer for the size of the time steps you want.  
It bins simulation time based on the specified time interval and collects statistics for each time step.  
The data for a time step is output when the full time step falls below GVT. 
This is essentially working correctly, but still needs a lot of optimization in regards to both memory 
management and computation. This data collection is a bit more complex than the other two, 
so it is on hold at the moment.  

For each time step, the following is output to file:
PE id, interval, forward events, reverse events, number of GVT comps, all reduce calls, events aborted, 
event ties detected in PE queues, remote events, network sends, network recvs, events rolled back, 
primary rollbacks, secondary roll backs, fossil collect attempts

### Output formatting
The GVT and real time collections are pushed to a buffer as they are collected, in order to reduce 
the amount of I/O accesses.  Currently the buffer is per PE.  If both GVT and real time sampling
are used, each has it's own buffer.  
The default buffer size is 8 MB but this can be changed using `--buffer-size=n`, where n is the size 
of the buffer in bytes.  
After GVT, the buffer's free space is checked.  By default, if there is less than 15% free space, 
then it is dumped to file in a binary format.  This can be changed using `--buffer-free=n`, where n 
is the percentage of free space it checks for before writing out.  

The output is in binary and right now it outputs one file per simulation per type of data collection 
(e.g., if you run both GVT and real time collections, you get a file with the GVT data and a 2nd file
for the real time sampling). ROSS will create a directory called stats-output that these files will be
placed in.

There is a basic reader for both types of output being developed in the 
CODES-vis repo (ross-reader branch).  
In the future we may switch to an already established file format (perhaps something like XDMF), 
or just further develop what is being used currently.  

### Other notes
There are a couple of other options that show up in the ROSS stats options.
One is `--disable-output=1`.  This is for use when examining the perturbation of the data collection 
on the simulation.  
It means that data (for GVT and real time collections) will be pushed to the buffer, but the buffer 
will never be dumped, so it will just keep overwriting data.  
This is so we can measure the effects of the computation of data collection itself without the I/O.

Another option listed is `--pe-per-file`.  
Now this is only relevant for the virtual time data collection and will be removed soon, 
so it can be ignored.  

Work in progress:
- Add in event level data collection
- Optimize memory usage and computation time for virtual time data collection, and make it work with buffer, output in binary

