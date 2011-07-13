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

enum event_t
{
  GENERATE,
  ARRIVAL,
  SEND,
  PROCESS
};

enum message_t
{
  DATA,
  ACK,
  CONT
};

enum block_t
{
  ComputeNode,
  IONode,
  FileServer,
  Disk
};

struct compute_node_state
{
  /////////////////////////////
  int upID;
  int * dwonID;
  ///////////////////////////////

  int CN_ID_in_tree;
  int FS_group_id;
  int ION_group_id;

  int tree_next_hop_id;
  int tree_previous_hop_id;
  int * torus_neighbour_minus_id;
  int * torus_neighbour_plus_id;

  ////////////////////////////////

  unsigned long long packet_counter;

  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;

};

struct io_node_state
{  
  int file_server_id;
  int root_CN_id;

  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;

  int collective_round_counter;
  
};

struct file_server_state
{
  int * previous_ION_id;
  int controller_id;
  
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct controller_state
{
  int * previous_FS_id;
  int ddn_id;
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct ddn_state
{
  int * previous_CON_id;
  int file_server_id;
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct nodes_message
{
  ///////////////////////////////
  int collective_msg_tag;
  int msg_size;
  BlockType MsgSrc;
  MsgType message_type;

  ///////////////////////////////
  tw_stime   travel_start_time;
  EventType  type;

};

#endif
