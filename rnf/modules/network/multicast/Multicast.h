#ifndef ROSS_MULTICAST_H
#define ROSS_MULTICAST_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <rossnet.h>

#include "Util.h"
#include "Topo.h"
#include "Routing.h"
#include "Packet.h"

//#define DEBUG_PKT_FWD                    1
//#define DEBUG_PKT_DISCARD                1

#define EXTRA_EVENT_PER_NODE             20          /* include timers etc */
#define KP_NUM_PER_PE                    1
//#define AGENT_TABLE_SIZE                 1009
#define AGENT_TABLE_SIZE                 100109

#define SMALL_FLOAT                      ((double) 0.0000000001)

typedef void (* PktHandler)     (void * agent, void * packet);
typedef void (* StartFunc)      (void * agent);
typedef void (* StopFunc)       (void * agent);
typedef void (* TimerFunc)      (void * agent, int timerId, int serial);
typedef void (* AgentFunc)      (void * agent);

typedef enum {
  GROUP_OP = 0, DATA_PKT = 1, SEND_PKT, START_AGENT, STOP_AGENT, AGENT_TIMER
} MessageType;
typedef enum {CREATE = -1, LEAVE = 0, JOIN = 1} GroupOpType;


typedef struct AgentStruct {
  int nodeId_;      /* ID of the node where the agent is on */
  uint32 listenAddr_;  /* The dest addr which the agent is listening to */
  uint32 id_;        /* Agent ID. */
  PktHandler handler_;
  StartFunc startf_;
  StopFunc stopf_;
  
  TimerFunc timerf_;
  AgentFunc statf_;
  void * info_;     /* Control data of this agent */
  uint8 used_;
} Agent;


typedef struct AgentHashStruct {
  int16 head_, tail_;
  Agent agent_[HASH_BUCKET_DEPTH];  
} AgentHash;


typedef struct StatAgentListStruct {
  Agent * agent_;
  struct StatAgentListStruct * next_;
} StatAgentList;


typedef struct GroupOpStruct {
  uint32 group_;
  GroupOpType op_;
} GroupOp;  /* Group operation */


typedef struct TimeOutStruct {
  Agent * agent_;
  int timerId_;
  int serial_;      /* Serial number of the same type timers */
} TimeOut;


typedef struct EventMsgStruct {
  MessageType type_;
  union {
    Packet pkt_;
    GroupOp grpOp_;
    Agent * agent_;
    TimeOut timeOut_;
  } content_;
} EventMsg;  /* Used for ROSS event */


typedef struct NodeStateStruct {
} NodeState;

/*
 * Functions 
 */

void McastStartUp(NodeState * sv, tw_lp *lp);
void McastCollectStats(NodeState * sv, tw_lp *lp);
void McastEventHandler(NodeState * sv, tw_bf * cv, EventMsg * em, tw_lp * lp);
void McastRcEventHandler(NodeState * sv, tw_bf * cv, EventMsg * em, tw_lp * lp);
void ReadGroupOp (char * fileName);

inline int ForwardPacket (tw_lp * srcLp, tw_lp * dstLp, Packet * p, 
                           tw_stime t);
inline int ForwardPktOnLink (tw_lp * srcLp, Link * lnk, Packet * p);
int ForwardMulticastPkt (tw_lp * lp, Packet * p, MapUnitType * portMap);
int ForwardUnicastPkt (tw_lp * lp, Packet * p);
void PassPktToUpperLayer (tw_lp * lp, Packet * p);

Agent * RegisterAgent (int nodeId, uint32 agentId, uint32 listenAddr, 
                       PktHandler h, 
                       StartFunc startf, StopFunc stopf, TimerFunc timerf,
                       AgentFunc statf,
                       void * info);
void RegisterAgentCopy (Agent * agent, uint32 listenAddr);
inline Agent * GetAgent (int nodeId, uint32 agentId);
void UnregisterAgent (int nodeId, uint32 agentId);
void UnregisterAgentCopy (int nodeId, uint32 agentId);

#define TW_MCAST 1

extern tw_lptype _mylps[];
extern AgentHash _agentTable[];
extern char * _groupOpFileName;
extern StatAgentList * statAgentListHead_, * statAgentListTail_;

#endif
