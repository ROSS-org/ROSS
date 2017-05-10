## README for ROSS Instrumentation

Currently several different types of instrumentation have been added to ROSS that can be used to collect data on the simulation engine and/or the model being simulated: GVT-based, real time sampling, and event tracing.  For GVT-based and real time-based sampling modes you can now collect simulation engine data and model-level data.  For collection of simulation engine data, the models should not require any code changes.  Obviously to collect model-level data, this will require adding in some code to the model(s), but we've attempted to keep this as simple as possible.

All 3 instrumentation types can be used independently or together.  The command line options for the instrumentation are shown under the title "ROSS Instrumentation" when you run `--help` with a ROSS/CODES model.  Just make sure to update ROSS for the instrumentation and then rebuild your model (including CODES if necessary).  For all instrumentation types, you can use `--stats-filename` option to set a prefix for the output files.  All of the output files are stored in a directory named `stats-output` that is created in the running directory.

### GVT-based Instrumentation
This collects data immediately after each GVT and can be turned on by using `--enable-gvt-stats=1` at runtime. By default, the data is collected on a PE basis, but some metrics can be changed to tracking on a KP or LP basis (depending on the metric).  To turn on instrumentation for the KP/LP granularity, use `--granularity=1`.    

When collecting only on a PE basis (i.e., `--granularity=0`), this is the format of the data:

```
PE_ID, GVT, all_reduce_count, events_processed, events_aborted, events_rolled_back, event_ties, 
total_rollbacks, secondary_rollbacks, fossil_collects, network_sends,
network_recvs, remote_events, efficiency
```

When collecting on a KP/LP basis, this is the format:

```
PE_ID, GVT, all_reduce_count, events_aborted, event_ties, fossil_collect, efficiency, 
total_rollbacks_KP0, secondary_rollbacks_KP0, ..., total_rollbacks_KPi, secondary_rollbacks_KPi, 
events_processed_LP0, events_rolled_back_LP0, network_sends_LP0, network_recvs_LP0, remote_events_LP0, 
..., events_processed_LPj, events_rolled_back_LPj, network_sends_LPj,network_recvs_LPj,
remote_events_LPj
```

where i,j are the total number of KPs and LPs per PE, respectively.  

You can also choose to sample less often at GVT using the `--num-gvt=n` command, where n is the number of GVT computations to complete between each sampling point.  

### Real Time Sampling
This collects data at real time intervals specified by the user.  
It is turned on using `--real-time-samp=n`, where n is the number of milliseconds per interval.  This collects all of the same data as the GVT-based instrumentation, as well as some other metrics, which is the difference in GVT and local virtual time for each KP as well as the cycle counters for the PEs (e.g., how much time is spent in event processing, roll back processing, GVT, etc). The difference in GVT and KP virtual time is always recorded per KP, regardless of how `--granularity` is set. For the other metrics, the granularity can be switched as described in the section on the GVT-based instrumentation.

When collecting on a PE basis, this is the data format:

```
PE_ID, current_real_time, current_GVT, time_ahead_GVT_KP0, ..., time_ahead_GVT_KPi,
network_read_CC, gvt_CC, fossil_collect_CC, event_abort_CC, event_processing_CC,
priority_queue_CC, rollbacks_CC, cancelq_CC, avl_CC, buddy_CC, lz4_CC,
events_aborted, pq_size, remote_events, network_sends, network_recvs,
event_ties, fossil_collects, num_GVT, events_processed, events_rolled_back,
total_rollbacks, secondary_rollbacks, 
```

For the KP/LP granularity:
```
PE_ID, current_real_time, current_GVT, time_ahead_GVT_KP0, ..., time_ahead_GVT_KPi,
network_read_CC, gvt_CC, fossil_collect_CC, event_abort_CC, event_processing_CC,
priority_queue_CC, rollbacks_CC, cancelq_CC, avl_CC, buddy_CC, lz4_CC,
events_aborted, pq_size, event_ties, fossil_collects, num_GVT,
total_rollbacks_KP0, secondary_rollbacks_KP0, ..., total_rollbacks_KPi, secondary_rollbacks_KPi,
events_processed_LP0, events_rolled_back_LP0, network_sends_LP0, network_recvs_LP0, remote_events_LP0,
..., events_processed_LPj, events_rolled_back_LPj, network_sends_LPj,network_recvs_LPj,
remote_events_LPj
```


### Event Tracing
There are two ways to collect the event trace.  One is to collect data only about events that are causing rollbacks. When an event that should have been processed in the past is received, data about this event is collected (described below).  The other method of event tracing is to collect data for all events.  For event tracing, ROSS can directly access the source and destination LP IDs for each event, as well as the sent and received virtual timestamps.  It will also record the real time that the event is computed at. To turn on the event tracing with either `--event-trace=1` for full event trace or `--event-trace=2` for tracing only events that cause rollbacks. 

Because event types are determined by the model developer, ROSS cannot directly access this.
The user can create a callback function that ROSS will use to collect the event type, and ROSS will then handle storing
the data in the buffer and the I/O.  The benefit to this is that the user can choose to collect other data about the event,
and ROSS will handle this as well.  It's important to remember that this data collection will result in a lot of output, since it's happening per event.  The details on using event tracing with model-level data is described in the next section on Model-level sampling

### Model-level data sampling

Registering the function pointers necessary for the callback functions required for the model-level sampling is implemented similarly to how the LP type function pointers are implemented.  (As a side note, it is a different struct, so that non-instrumented ROSS can still be run without requiring any additions to models).  

##### Function pointers:
```C
typedef void (*rbev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*model_stat_f) (void *sv, tw_lp *lp, char *buffer);
```

The first two functions are for the event tracing functionality.  `msg` is the message being passed, `lp` is the LP pointer.  `buffer` is the pointer to where the data needs to be copied for ROSS to manage. `collect_flag` is a pointer to a ROSS flag.  By default `*collect_flag == 1`.  This means that the event will be collected.  This flag was added to let you control whether specific events for an LP are collected or not. Change the value to 0 for any events you do not want to show in the trace.  This means even the ROSS level data will not be collected for that event (e.g., src_LP, dest_LP, etc).  You can use this feature to turn off event tracing for certain events, reducing the amount of data you will be storing.

The third function is for model-level data to be sampled at the same time as the simulation engine data (at GVT or based on real time). `sv` is the pointer to the LP state. 


##### Model types struct for function pointers
```C
typedef struct st_model_types st_model_types;
struct st_model_types {
    rbev_trace_f rbev_trace; /* function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;          /* size of data collected from model about events causing rollbacks */
    ev_trace_f ev_trace;     /* function pointer to collect data about all events for given LP */
    size_t ev_sz;            /* size of data collected from model for each event */
    model_stat_f model_stat_fn; /* function pointer to collect model-level data */
    size_t mstat_sz;         /* size of data collected from model at sampling points */
};
```
This is the struct where we provide the function pointers to ROSS.  `rbev_trace` is the pointer for the event collection for only events causing rollbacks, while `ev_trace` is for the full event collection.  `rbev_sz` and `ev_sz` are the sizes of the data that need to be pushed to the buffer for the rollback causing events, or for all events, respectively.  `model_stat_fn` is the pointer for collecting model-level data with the GVT-based and real time sampling modes.  Similarly, `mstat_sz` is the size of the data that will be saved at each sampling point for that LP.

There are two ways of registering your pointer functions with ROSS depending on the way you register your LP types with ROSS:  

###### Method 1
If you call the `tw_lp_settype()` for LP type registration with ROSS, you should follow this method.

To register the function pointers with ROSS, call `st_model_settype(tw_lpid i, st_trace_type *trace_types)` right after you call the `tw_lp_settype()` function when initializing your LPs.  You can also choose to turn event tracing on for only certain LPs.  To do this, you only need to call `st_model_settype()` with the appropriate arguments for the LPs you want event tracing turned on.

For example, here is how it is done in PHOLD in `main()`:
```C
for (i = 0; i < g_tw_nlp; i++)
{
    tw_lp_settype(i, &mylps[0]);
    st_model_settype(i, &model_types[0]);
}
```

`mylps` is the `tw_lptype` struct containing the usual ROSS function pointers for initialization, event handlers, etc.  `model_types` is the `st_model_types` struct described above.  

If your model is a part of CODES, the CODES mapping will handle this for you.  See the [CODES repo](https://xgitlab.cels.anl.gov/codes/codes) and documentation for more details.  

###### Method 2
If you call the `tw_lp_setup_types()` function for LP registration, follow this method.  This requires that you set `g_tw_lp_types` and `g_tw_lp_typemap`.

Similarly for registering the model sampling functions, you'll need to set `st_model_types *g_st_model_types`.  This needs to be set *before* the call to `tw_lp_setup_types()`.  If you have multiple LP types that you are registering, the array of `st_model_types` should be in the same order as the `tw_lptypes` array that you set `g_tw_lp_types` to point to.  

That's it.  The call to `tw_lp_setup_types()` will make the call to `st_model_setup_types()` if you have anything requiring model-level sampling turned on.  



### Output formatting
All collected data is pushed to a buffer as it is collected, in order to reduce the amount of I/O accesses.  Currently the buffer is per PE.  If multiple instrumentation modes are used, each has its own buffer. The default buffer size is 8 MB but this can be changed using `--buffer-size=n`, where n is the size of the buffer in bytes. 

After GVT, the buffer's free space is checked.  By default, if there is less than 15% free space, then it is dumped to file in a binary format.  This can be changed using `--buffer-free=n`, where n is the percentage of free space it checks for before writing out.  

The output is in binary and right now it outputs one file per simulation per instrumentation type (e.g., if you run both GVT-based and real time instrumentation, you get a file with the GVT data and a 2nd file for the real time sampling). ROSS will create a directory called stats-output that these files will be placed in.

There is a basic binary reader for all of the instrumentation modes being developed in the 
[ROSS Binary Reader repo](https://github.com/caitlinross/ross-binary-reader). For the time being, ROSS will output a README file in the stats-output directory with the given filename prefix.  The file contains some general information about values of input parameters, but also has data that the reader can use to correctly read the instrumentation data.

### Other notes
There are a couple of other options that show up in the ROSS stats options. One is `--disable-output=1`.  This is for use when examining the perturbation of the data collection on the simulation.  It means that data (for GVT and real time collections) will be pushed to the buffer, but the buffer will never be dumped, so it will just keep overwriting data. This is so we can measure the effects of the instrumentation layer itself without the I/O, otherwise you'll want to leave this turned off.  At some point in the future, this will probably be converted into allowing data to be streamed to an in situ analysis system.  


