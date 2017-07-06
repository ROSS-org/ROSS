#ifndef INC_io_h
#define INC_io_h

//Elsa Gonsiorowski
//Rensselaer Polytechnic Institute
//Decemeber 13, 2013

// ** Global IO System variables ** //

// Set with command line --io-files
// should be consistent across the system
extern int g_io_number_of_files;

// Register opts with ROSS
extern const tw_optdef io_opts[3];

enum io_load_e {
	NONE,		// default value
	PRE_INIT,	// load LPs then lp->init
	INIT,		// load LPs instead lp->init
	POST_INIT,	// load LPs after lp->init
};
typedef enum io_load_e io_load_type;
extern io_load_type g_io_load_at;
extern char g_io_checkpoint_name[1024];

// Should be set in main, before call to io_init
// Maximum number of events that will be scheduled past end time
extern int g_io_events_buffered_per_rank;

// ** API Functions, Types, and Variables ** //

void io_register_model_version(char *sha1);
void io_init();

void io_load_checkpoint(char * master_filename, io_load_type load_at);
void io_store_checkpoint(char * master_filename, int data_file_number);
void io_appending_job();

// LP type map and function struct
typedef void (*serialize_f)(void * state, void * buffer, tw_lp *lp);
typedef void (*deserialize_f)(void * state, void * buffer, tw_lp *lp);
typedef size_t (*model_size_f)(void * state, tw_lp *lp);

typedef struct {
    serialize_f serialize;
    deserialize_f deserialize;
    model_size_f model_size;
} io_lptype;

extern io_lptype * g_io_lp_types;

// ** Internal IO types, variables, and functions ** //

typedef struct {
	int part;
	int file;
	int offset;
	int size;
	int lp_count;
	int ev_count;
} io_partition;
static int io_partition_field_count = 6;

typedef struct {
	tw_lpid gid;
	int32_t rng[12];
#ifdef RAND_NORMAL
	double tw_normal_u1;
	double tw_normal_u2;
	int tw_normal_flipflop;
#endif
} io_lp_store;

typedef struct {
	tw_bf cv;
	tw_lpid dest_lp;
	tw_lpid src_lp;
	tw_stime recv_ts;
	// NOTE: not storing tw_memory or tw_out
} io_event_store;

extern io_partition * g_io_partitions;

// Functions Called Directly from ROSS
void io_load_events(tw_pe * me);
void io_event_cancel(tw_event *e);
void io_read_checkpoint();

// SERIALIZE FUNCTIONS for LP and EVENT structs
// found in io-serialize.c
size_t io_lp_serialize (tw_lp * lp, void * buffer);
size_t io_lp_deserialize (tw_lp * lp, void * buffer);
size_t io_event_serialize (tw_event * e, void * buffer);
size_t io_event_deserialize (tw_event * e, void * buffer);

// INLINE function for buffering events past end time
extern tw_eventq g_io_buffered_events;
extern tw_eventq g_io_free_events;
extern tw_event * io_event_grab(tw_pe *pe);
#endif
