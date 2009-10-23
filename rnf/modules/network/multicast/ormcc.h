#ifndef ROSS_MCAST_ORMCC_H
#define ROSS_MCAST_ORMCC_H

#include "Transport.h"
#include <math.h>

#define ORMCC_TEST_CR_SWITCH                1
//#define ORMCC_TEST_CI_LOSS                  1


#define ORMCC_SRC_DATA_PKT_SIZE             1000
#define ORMCC_SRC_MIN_RATE                  ORMCC_SRC_DATA_PKT_SIZE 
                                                                 /* 1 pkt/sec */
#define ORMCC_SRC_INIT_RTT                  0.1
#define ORMCC_SRC_INIT_RATE   (2 * ORMCC_SRC_DATA_PKT_SIZE / ORMCC_SRC_INIT_RTT)
                                                                 /* 2 pkt/rtt */

//#define TRACE_ORMCC_SRC_RATE_DYN            1
#define TRACE_ORMCC_RTT                     1

#define TRACE_ORMCC_THROUGHPUT              1

#ifdef TRACE_ORMCC_THROUGHPUT
#define ORMCC_THROUGHPUT_SAMPLE_ITVL        0.5
                                                                /* Seconds */
#endif

#define ORMCC_TRAC_EWMA_FACTOR              0.05
#define ORMCC_COMPARE_TRAC_FACTOR           1

#define ORMCC_RCV_CI_PKT_SIZE               40

#define RATE_SMOOTH_RING_SIZE               11
#define RATE_SMOOTH_TIME                    1
                                                                
//#define DEBUG_ORMCC_DATA_PKT                1
//#define DEBUG_ORMCC_CI_PKT                  1
//#define DEBUG_ORMCC_RCV_RATE                1


typedef struct RateSampleStruct {
  double time_, rate_;
  struct RateSampleStruct * next_;
} RateSample ;


typedef struct RateSmoothDataStruct {
  RateSample    sample_[RATE_SMOOTH_RING_SIZE];
  int16         head_, tail_;
  double        span_,       /* Span of the sampling time */
                totalData_;  /* Total volume of traffic */
} RateSmoothData;


typedef enum {
  ORMCC_SRC_TIMER_SEND_PKT,
  ORMCC_SRC_TIMER_RATE_INC,
  ORMCC_SRC_TIMER_CR_RESP,
  ORMCC_SRC_TIMER_CONG_EPOCH
} OrmccSrcTimerType;


typedef struct OrmccSrcInfoStruct {
  uint32 pktDstAddr_;       /* Packet destination address */
  uint16 pktDstAgentId_;
  uint32 sqn_;              /* Sequence number */
  uint32 pktSize_;

  int8   trackStat_;

  int8   active_,
         crGone_;           /* Indicates whether CR is inactive */
  uint32 crAddr_;           /* Congestion representative */
    
  double rate_,             /* Sending rate in bytes/sec */
         rttAvg_,           /* RTT estimation */
         rttDev_,           /* RTT deviation */
         maxRtt_,
         crTracAvg_,        /* TRAC of CR */
         crTracDev_,        /* TRAC deviation of CR */
         crChkBegTime_,     /* Time beginning CR check */
         crRespTimeAvg_,    /* Response timer of CR when the bottleneck is
                             * fully loaded */
         crRespTimeDev_;    /* Deviation of above */

  uint32 ciRcvd_, ciFromCr_;     /* Total number of CI received */         
  uint32 notCut_;
  
#ifdef TRACE_ORMCC_THROUGHPUT
  double thruSampleTime_;
  uint32 totalSentBytes_;
#endif

  tw_event * sendPktTimerEvnt_,
           * rateIncrTimerEvnt_,
           * congEpochTimerEvnt_,
           * crRespTimerEvnt_;
} OrmccSrcInfo;


enum {
  ORMCC_RCV_CI_BACKOFF_TIMER
};


typedef struct OrmccRcvInfoStruct {
  int8 active_;
  uint32 crAddr_,           /* Addr of the current CR */
         senderAddr_,
         senderAgent_;
  double rate_,             /* Receiving rate */
         tracAvg_,
         crTracAvg_,
         crTracDev_,
         lastPktTime_;      /* Last packet arrival time */
  unsigned int ciSent_,     /* CI sent */
               ciSupp_;     /* CI suppressed */
  uint32 lastContSqn_;      /* Last continous seq number */
         
  /* For scheduled CI */
  uint32 ciSqn_;
  double ciTsEcho_, ciSchedTime_;
  
  double crBegTime_;
         
  RateSmoothData  rsd_;
  tw_event * ciBackoffTimerEvnt_;
} OrmccRcvInfo;


Agent * OrmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat);
void OrmccSrcStart (Agent * agent);
void OrmccSrcStop (Agent * agent);
void OrmccSrcPktHandler (Agent * agent, Packet * p);
void OrmccSrcSendPkt (Agent * agent);
void OrmccSrcTimer (Agent * agent, int timerId, int serial);
inline void OrmccSrcUpdateRtt (Agent * agent, double sample);
void OrmccSrcPrintStat (Agent * agent);

Agent * OrmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr);
void OrmccRcvStart (Agent * agent);
void OrmccRcvStop (Agent * agent);
void OrmccRcvPktHandler (Agent * agent, Packet * p);
void OrmccRcvSendPkt (Agent * agent);
void OrmccRcvTimer (Agent * agent, int timerId, int serial);
void OrmccRcvPrintStat (Agent * agent);


void RateSmoothInit (RateSmoothData * rsd, double span);
double RateSmoothAddSample (RateSmoothData * rsd, double rate, double time);


#endif
