/*                                                                              
 *  Blue Gene/P model                                                           
 *  Header file                                                                 
 *  by Ning Liu                                                                 
 */
#ifndef BGP_TYPES_H
#define BGP_TYPES_H

#include "bgp.h"
#include "CodesIOKernelTypes.h"
#include "CodesIOKernelParser.h"

//#define N_ION_msg 16384
#define N_ION_msg 131072 
#define N_sample 3000

typedef struct bgp_app_cf_info
{
  int gid;
  int min;
  int max;
  int lrank;
  int num_lrank;
} bgp_app_cf_info_t;

typedef struct codeslang_bgp_inst
{
  int event_type;
  int64_t num_var;
  int64_t var[10];
} codeslang_bgp_inst;

/* type define */
typedef struct compute_node_state CN_state;
typedef struct io_node_state ION_state;
typedef struct file_server_state FS_state;
typedef struct controller_state CON_state;
typedef struct ddn_state DDN_state;

typedef struct nodes_message MsgData;
typedef struct msg_content MsgContent;

typedef enum   event_t EventType;
typedef enum   block_t BlockType;
typedef enum   message_t MsgType;
// new version
typedef enum   iorequest_t IOType;
// end

enum event_t
{
  CL_GETSIZE=1,
  CL_GETRANK,
  CL_WRITEAT,
  CL_OPEN,
  CL_CLOSE,
  CL_SYNC,
  CL_EXIT,
  CL_NOOP,
  CL_DELETE,
  CL_SLEEP,

  LOOKUP_SEND,
  LOOKUP_ARRIVE,
  LOOKUP_PROCESS,
  LOOKUP_ACK,
  LOOKUP_END,
  
  CREATE_SEND,
  CREATE_ARRIVE,
  CREATE_PROCESS,
  CREATE_ACK,
  CREATE_END,

  PRE_CONFIGURE,

  HANDSHAKE_SEND,
  HANDSHAKE_ARRIVE,
  HANDSHAKE_PROCESS,
  HANDSHAKE_ACK,
  HANDSHAKE_END,

  DATA_SEND,
  DATA_ARRIVE,
  DATA_PROCESS,
  DATA_ACK,
  DATA_END,

  CLOSE_SEND,
  CLOSE_ARRIVE,
  CLOSE_PROCESS,
  CLOSE_ACK,
  CLOSE_END,

  CONFIGURE,
  BGP_SYNC,
  WRITE_SYNC,
  CHECKPOINT,
  APP_IO_REQUEST,
  
  APP_OPEN,
  APP_OPEN_DONE,
  APP_WRITE,
  APP_WRITE_DONE,
  APP_READ,
  APP_READ_DONE,
  APP_CLOSE,
  APP_CLOSE_DONE,
  APP_SYNC,
  APP_SYNC_DONE,
  APP_SLEEP,
  APP_SLEEP_DONE,

};

enum block_t
{
  ComputeNode,
  IONode,
  FileServer,
  Controller,
  DDN
};

enum iorequest_t
  {
    WRITE_ALIGNED,
    WRITE_UNALIGNED,
    WRITE_UNIQUE,
    READ_ALIGNED,
    READ_UNALIGNED,
    READ_UNIQUE
  };

struct msg_content
{
  int bb_flag;
  unsigned long long sleep_time;
  int Local_CN_root;
  int group_id;
  int num_in_group;
};

struct nodes_message
{
  unsigned long long io_offset;
  unsigned long long io_payload_size;
  
  int io_tag;
  int collective_group_size;
  int collective_group_rank;
  int collective_master_node_id;

  int IsLastPacket;

  tw_stime travel_start_time;
  EventType  event_type;  
  IOType io_type;

  tw_lpid message_CN_source;
  tw_lpid message_ION_source;
  tw_lpid message_FS_source;

  MsgContent MC;
};


struct compute_node_state
{
  unsigned long long cumulated_data_size;

  int num_getsize;
  int num_getrank;
  int num_writeat;
  int num_open;
  int num_close;
  int num_sync;

  CodesIOKernelContext c;
  CodesIOKernel_pstate * ps;
  codeslang_bgp_inst next_event;

  int CN_ID;
  int CN_ID_in_tree;
  int tree_next_hop_id;
  tw_stime sender_next_available_time;

  int * compute_node;
  double * time_stamp;

  double bandwidth;

  int sync_counter;
  int checkpoint_counter;
  int write_counter;
  int w_counter;

  tw_lpid FS_source;

  MsgData SavedMessage;

  unsigned long long committed_size1[N_sample];
  unsigned long long committed_size2[N_sample];
  unsigned long long committed_size3[N_sample];
  /* job mapping info */
  bgp_app_cf_info_t task_info;

  int local_root_gid;
};

struct io_node_state
{  
  tw_stime cn_sender_next_available_time;
  tw_stime fs_sender_next_available_time;
  tw_stime processor_next_available_time;
  tw_stime cn_receiver_next_available_time;
  tw_stime fs_receiver_next_available_time;

  unsigned long long buffer_pool_leftover;
  unsigned long long bb_record1[N_sample];
  unsigned long long bb_record2[N_sample];
  unsigned long long bb_record3[N_sample];

  unsigned long long cumulated_data_size;

  unsigned long long bb_size;
  int bb_status;

  int piece_tag[N_ION_msg];
  unsigned long long saved_message_size[N_ION_msg];
  int msg_counter;
  int ack_counter;

  int * file_server;
  int * io_node;
  int myID_in_ION;

  int* io_counter;

  int close_counter;
  MsgData SavedMessage;
  int close_flag;
};

struct file_server_state
{
  tw_stime ion_receiver_next_available_time;
  tw_stime ddn_receiver_next_available_time;
  tw_stime processor_next_available_time;
  tw_stime ion_sender_next_available_time;
  tw_stime ddn_sender_next_available_time;

  int * previous_ION_id;
  int controller_id;

};

struct controller_state
{
  tw_stime fs_receiver_next_available_time;
  tw_stime ddn_receiver_next_available_time;
  tw_stime processor_next_available_time;
  tw_stime fs_sender_next_available_time;
  tw_stime ddn_sender_next_available_time;

  unsigned long long committed_size[N_sample];

  int * previous_FS_id;
  int ddn_id;
};

struct ddn_state
{
  int * previous_CON_id;
  int file_server_id;

  tw_stime processor_next_available_time;
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};


#endif
