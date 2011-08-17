/*                                                                              
 *  Blue Gene/P model                                                           
 *  Header file                                                                 
 *  by Ning Liu                                                                 
 */
#ifndef BGP_TYPES_H
#define BGP_TYPES_H

#include "bgp.h"

/* type define */
typedef struct compute_node_state CN_state;
typedef struct io_node_state ION_state;
typedef struct file_server_state FS_state;
typedef struct controller_state CON_state;
typedef struct ddn_state DDN_state;

typedef struct nodes_message MsgData;

typedef enum   event_t EventType;
typedef enum   block_t BlockType;
typedef enum   message_t MsgType;
// new version
typedef enum   iorequest_t IOType;
// end

enum event_t
{
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

  APP_IO_REQUEST,
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
    WRITE_COLLECTIVE,
    WRITE_UNALIGNED,
    WRITE_INDIVIDUAL,
    READ_COLLECTIVE,
    READ_UNALIGNED,
    READ_INDIVIDUAL
  };

struct compute_node_state
{
  int CN_ID;
  int CN_ID_in_tree;
  int tree_next_hop_id;
  tw_stime sender_next_available_time;

};

struct io_node_state
{  
  tw_stime cn_sender_next_available_time;
  tw_stime fs_sender_next_available_time;
  tw_stime processor_next_available_time;
  tw_stime cn_receiver_next_available_time;
  tw_stime fs_receiver_next_available_time;

  int * file_server;
  int * io_node;
  int myID_in_ION;

  int* io_counter;

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

};

#endif
