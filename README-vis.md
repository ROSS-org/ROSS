## README for ROSS Instrumentation

Currently several different types of instrumentation have been added to ROSS: GVT-based, real time sampling, and event tracing.  
All 3 instrumentation types can be used independently or together.  The options for 
the data collection show under the title "ROSS Stats" when you run `--help` with a ROSS/CODES model.  
No instrumentation type requires changes to the model-level code in order to run.  Just make sure you've built ROSS using the Vis2 branch and then rebuild your model (including CODES if necessary).  However, the event tracing does require some minor changes to model code in order to collect event type information.  The details are described in the event tracing section below.

For all instrumentation types, you can use `--stats-filename` option to set a prefix for the output files.  
All of the output files are stored in a directory named `stats-output` that is created in the running directory.

### GVT-based Instrumentation
This collects data immediately after each GVT and can be turned on by using `--enable-gvt-stats=1` at runtime. By default, the data is collected on a PE basis, but some metrics can be changed to tracking on a KP or LP basis (depending on the metric).  To turn on instrumentation for the KP/LP granularity, use `--granularity=1`.    

When collecting only on a PE basis (i.e., `--granularity=0`), this is the format of the data:

```
PE_ID, GVT, all_reduce_count, events_processed, events_aborted, events_rolled_back, event_ties, 
total_rollbacks, primary_rollbacks, secondary_rollbacks, fossil_collects, network_sends,
network_recvs, remote_events, net_events, efficiency
```

When collecting on a KP/LP basis, this is the format:

```
PE_ID, GVT, all_reduce_count, events_aborted, event_ties, fossil_collect, efficiency, 
total_rollbacks_KP0, secondary_rollbacks_KP0, ..., total_rollbacks_KPi, secondary_rollbacks_KPi, 
events_processed_LP0, events_rolled_back_LP0, network_sends_LP0, network_recvs_LP0,
..., events_processed_LPj, events_rolled_back_LPj, network_sends_LPj,network_recvs_LPj,
remote_events_LP0, ..., remote_events_LPj
```

where i,j are the total number of KPs and LPs for the PE, respectively.  

### Real Time Sampling
This collects data at real time intervals specified by the user.  It is turned on using 
`--real-time-samp=n`, where n is the number of milliseconds per interval.  
This collects all of the same data as the GVT-based instrumentation, as well as some other metrics, which is the difference in GVT and virtual time for each KP and cycle counters for the PEs. The difference in GVT and KP virtual time is always recorded per KP, regardless of how `--granularity` is set.
The granularity can be switched as described in the section on the GVT-based instrumentation.

When collecting on a PE basis, this is the data format:

```
PE_ID, current_real_time, current_GVT, time_ahead_GVT_KP0, ..., time_ahead_GVT_KPi,
network_read_CC, gvt_CC, fossil_collect_CC, event_abort_CC, event_processing_CC,
priority_queue_CC, rollbacks_CC, cancelq_CC, avl_CC, buddy_CC, lz4_CC,
events_aborted, pq_size, remote_events, network_sends, network_recvs,
event_ties, fossil_collects, num_GVT, events_processed, events_rolled_back,
total_rollbacks, secondary_rollbacks, net_events, primary_rollbacks
```

For the KP/LP granularity:
```
PE_ID, current_real_time, current_GVT, time_ahead_GVT_KP0, ..., time_ahead_GVT_KPi,
network_read_CC, gvt_CC, fossil_collect_CC, event_abort_CC, event_processing_CC,
priority_queue_CC, rollbacks_CC, cancelq_CC, avl_CC, buddy_CC, lz4_CC,
events_aborted, pq_size, event_ties, fossil_collects, num_GVT,
total_rollbacks_KP0, secondary_rollbacks_KP0, ..., total_rollbacks_KPi, secondary_rollbacks_KPi,
events_processed_LP0, events_rolled_back_LP0, network_sends_LP0, network_recvs_LP0,
..., events_processed_LPj, events_rolled_back_LPj, network_sends_LPj,network_recvs_LPj,
remote_events_LP0, ..., remote_events_LPj
```


### Event Tracing
There are two ways to collect the event trace.  One is to collect data only about events that are causing rollbacks.
When an event that should have been processed in the past is received, data about this event is collected (described below).  The other event collection is for all events.  
For this collection, ROSS can directly access the source and destination LP IDs for each event, as well as the 
received virtual timestamp and real time duration of the event.  It will also record the real time that the event is computed at.
Because event types are determined by the model developer, ROSS cannot directly access this.
The user can create a callback function that ROSS will use to collect the event type, and ROSS will then handle storing
the data in the buffer and the I/O.  The benefit to this is that the user can choose to collect other data about the event,
and ROSS will handle this as well.  It's important to remember that this data collection will result in a lot of output, since it's happening per event.  

This is implemented similarly to how the LP type function pointers are implemented.  (As a side note, it is a different
struct, so that non-instrumented ROSS can still be run without requiring any additions to models).  Once you have functions implemented and their pointers set to be registered with ROSS, you can turn on the event tracing with either `--event-trace=1` for full event trace or `--event-trace=2` for tracing only events that cause rollbacks. 

##### Function pointers:
```C
typedef void (*rbev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
```
`msg` is the message being passed, `lp` is the LP pointer.  `buffer` is the pointer to where the data needs to be copied for ROSS to manage.
`collect_flag` is a pointer to a ROSS flag.  By default `*collect_flag == 1`.  This means that the event will be collected.  Change the value to 0 for any events you do not want to show in the trace.  This means even the ROSS level data will not be collected (e.g., src_LP, dest_LP, etc).  

For instance in the dragonfly CODES model, we can do:
```C
void dragonfly_event_trace(terminal_message *msg, tw_lp *lp, char *buffer, int *collect_flag)
{
    int type = (int) msg->type;
    memcpy(buffer, &type, sizeof(type));
}
```
This is just a simple example; we could of course get more complicated with this and save other data.

##### Event type struct for function pointers
```C
typedef struct st_trace_type st_trace_type;
struct st_trace_type {
    rbev_trace_f rbev_trace; /* function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;          /* size of data collected from model about events causing rollbacks */
    ev_trace_f ev_trace;     /* function pointer to collect data about all events for given LP */
    size_t ev_sz;            /* size of data collected from model for each event */
};
```
This is the struct where we provide the function pointers to ROSS.  
`rbev_trace_f` is the pointer for the event collection for only events causing rollbacks, while `ev_trace_f` is for
the full event collection.  
`rbev_sz` and `ev_sz` are the sizes of the data that need to be pushed to the buffer for the rollback causing events, or for all events, respectively.

Going back to the CODES dragonfly example, we could implement the event tracing functions with:

```C
st_trace_type trace_types[] = {
    {(rbev_trace_f) NULL,
    0,
    (ev_trace_f) dragonfly_event_trace,
    sizeof(int)},
    {0}
};
```
This example assumes that we want to use the same `dragonfly_event_trace()` for both the terminal and router LPs in the dragonfly model and we'll use trace_types[0], when registering the trace types for both LP types.  

To register the function pointers with ROSS, call `st_evtrace_settype(tw_lpid i, st_trace_type *trace_types)` right after you call the `tw_lp_settype()` function when initializing your LPs.  You can also choose to turn event tracing on for only certain LPs.  To do this, you only need to call `st_evtrace_settype()` with the appropriate agruments for the LPs you want event tracing turned on.

If your model is a part of CODES, the CODES mapping will handle this for you.  Right now the model net base LPs, the dragonfly router and terminal LPs, and dragonfly synthetic workload LPs have this implemented, but it's in my [forked CODES repo](https://xgitlab.cels.anl.gov/caitlinross/codes) (event-collection branch) at the moment.  See that repo for more details on making event tracing changes on CODES models.  



### Output formatting
All collected data is pushed to a buffer as it is collected, in order to reduce 
the amount of I/O accesses.  Currently the buffer is per PE.  If multiple instrumentation types
are used, each has its own buffer.
The default buffer size is 8 MB but this can be changed using `--buffer-size=n`, where n is the size 
of the buffer in bytes. 
After GVT, the buffer's free space is checked.  By default, if there is less than 15% free space, 
then it is dumped to file in a binary format.  This can be changed using `--buffer-free=n`, where n 
is the percentage of free space it checks for before writing out.  

The output is in binary and right now it outputs one file per simulation per instrumentation type 
(e.g., if you run both GVT and real time instrumentation, you get a file with the GVT data and a 2nd file
for the real time sampling). ROSS will create a directory called stats-output that these files will be
placed in.

There is a basic reader for both types of output being developed in the 
[CODES-vis repo](https://xgitlab.cels.anl.gov/codes/codes-vis) (ross-reader branch).  
In the future we may switch to an already established file format (perhaps something like XDMF), 
or just further develop what is being used currently.  For the time being, ROSS will output a README file in 
the stats-output directory with the given filename prefix.  The file contains some general information about 
values of input parameters, but also has data that the reader in the CODES-vis repo can use to correctly read the
instrumentation data.

### Other notes
There are a couple of other options that show up in the ROSS stats options.
One is `--disable-output=1`.  This is for use when examining the perturbation of the data collection 
on the simulation.  
It means that data (for GVT and real time collections) will be pushed to the buffer, but the buffer 
will never be dumped, so it will just keep overwriting data.  
This is so we can measure the effects of the computation of data collection itself without the I/O, otherwise
you'll want to leave this turned off.  At some point in the future, this will probably be converted into allowing
data to be streamed to an in situ analysis system.  


