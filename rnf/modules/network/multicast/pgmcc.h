#ifndef ROSS_MCAST_PGMCC_H
#define ROSS_MCAST_PGMCC_H

#include "Transport.h"
#include <math.h>

#define TRACE_PGMCC_THROUGHPUT              1

#ifdef TRACE_PGMCC_THROUGHPUT
#define PGMCC_THROUGHPUT_SAMPLE_ITVL        0.5
                                                                /* Seconds */
#endif

#define PGMCC_SRC_DATA_PKT_SIZE             1000
#define PGMCC_RCV_FDBK_PKT_SIZE             40

/* pgmcc options from source code */
#define PGMCC_OPT_LOSSRATE        0x00010000    // loss report present
#define PGMCC_OPT_SEND_NAK        0x00020000    // receivers, please send a NAK
#define PGMCC_OPT_ELECT           0x00040000    // node X is elected as acker
#define PGMCC_OPT_TIMESTAMP       0x00080000    // timestamp present

#define PGMCC_SRC_SS_THRESH       6
#define PGMCC_SRC_DUP_THRESH      3
#define PGMCC_SRC_WEIGHT          0.992
#define PGMCC_SRC_CC_TIMEOUT      4.0
#define PGMCC_SRC_HYST            1.5

#define PGMCC_RCV_WEIGHT          0.992


enum {
  PGMCC_SRC_TIMER_CC
};


typedef struct PgmccSrcInfoStruct {
  uint32 pktDstAddr_;       /* Packet destination address */
  uint16 pktDstAgentId_;
  uint32 pktSize_;

  int8 trackStat_;
  int8 active_;
  uint32 txwLead_;          /* Last sent sequence number */
  uint32 ackerAddr_;
  
  double window_, token_;
  int16  dupAcks_;
  uint32 ackLead_;
  int32  ignoreCong_;
  uint32 ackBitMask_;
  double ackerMrtt_;
  double ackerLoss_;

  tw_event * ccTimerEvnt_;

#ifdef TRACE_PGMCC_THROUGHPUT
  double thruSampleTime_;
  uint32 totalSentBytes_;
#endif
} PgmccSrcInfo;


typedef struct PgmccRcvInfoStruct {
  int8 active_;
  
  uint32 ackBitMask_;
  uint32 rxwLead_;
  double rxLoss_;
  int    rxDoAck_;
  
  uint32 ackSent_, nakSent_;
} PgmccRcvInfo;


Agent * PgmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat);
void PgmccSrcStart (Agent * agent);
void PgmccSrcStop (Agent * agent);
void PgmccSrcSendPkt (Agent * agent);
void PgmccSrcTimer (Agent * agent, int timerId, int serial);
void PgmccSrcPktHandler (Agent * agent, Packet * p);
void PgmccSrcAckProc (Agent * agent, Packet * p);
void PgmccSrcNakProc (Agent * agent, Packet * p);
inline void PgmccSrcStall (Agent * agent);
void PgmccSrcPrintStat (Agent * agent);

Agent * PgmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr);
void PgmccRcvStart (Agent * agent);
void PgmccRcvStop (Agent * agent);
void PgmccRcvPktHandler (Agent * agent, Packet * p);
void PgmccRcvSendPkt (Agent * agent, Packet * srcPkt, int16 type);
void PgmccRcvTimer (Agent * agent, int timerId, int serial);
void PgmccRcvPrintStat (Agent * agent);


#endif
