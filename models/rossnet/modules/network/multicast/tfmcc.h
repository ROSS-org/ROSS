#ifndef ROSS_MCASH_TFMCC_H
#define ROSS_MCASH_TFMCC_H

#include "Transport.h"
#include <float.h>
#include <assert.h>

#define TRACE_TFMCC_THROUGHPUT              1

#ifdef TRACE_TFMCC_THROUGHPUT
#define TFMCC_THROUGHPUT_SAMPLE_ITVL        0.1
                                                                /* Seconds */
#endif

#ifndef Max
#define Max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef Min
#define Min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define TFMCC_SMALLFLOAT 0.0000001


/* modes of rate change */
enum {
  TFMCC_SRC_STATE_INIT_RATE = 0,
  TFMCC_SRC_STATE_SLOW_START = 1,
  TFMCC_SRC_STATE_CONG_AVOID = 2
};


/* cut sending rate after DEC_NO_REPORT rtts without receiver report */
#define TFMCC_SRC_DEC_NO_REPORT     10
#define TFMCC_SRC_G_FACTOR          0.1
#define TFMCC_MAX_ROUND_ID          1000000
#define TFMCC_SRC_MAX_RTT_DECAY     0.9

enum {
  TFMCC_SRC_PRIO_FORCE = 4, 
  TFMCC_SRC_PRIO_NO_RTT = 3, 
  TFMCC_SRC_PRIO_RECV = 2,
  TFMCC_SRC_PRIO_CLR = 1
};


/* Timer types */
enum {
  TFMCC_SRC_TIMER_SEND_PKT,
  TFMCC_SRC_TIMER_NO_FDBK
};


#define TFMCC_SRC_DATA_PKT_SIZE             1000

#define TFMCC_SRC_MIN_RATE                  TFMCC_SRC_DATA_PKT_SIZE
#define TFMCC_SRC_INIT_RTT                  0.5
#define TFMCC_SRC_INIT_RATE                 (2 * TFMCC_SRC_DATA_PKT_SIZE \
                                             / TFMCC_SRC_INIT_RTT)


typedef struct TfmccSrcInfoStruct {
  /* Standard */
  
  uint32 pktDstAddr_;       /* Packet destination address */
  uint16 pktDstAgentId_;
  uint32 pktSize_;

  int8   active_, trackStat_;

#ifdef TRACE_TFMCC_THROUGHPUT
  double thruSampleTime_;
  uint32 totalSentBytes_;
#endif

  /* Custom */
  
  tw_event * sendPktTimerEvnt_, * noFdbkTimerEvnt_;
    
  uint32 seqno_;
  int fairsize_;            // packet size of competing flow
  double rate_;             // send rate
  double oldrate_;          // allows rate to be changed gradually
  double delta_;            // allows rate to be changed gradually
  int rate_change_;         // slow start, cong avoid, decrease ...
  double expected_rate;     // TCP friendly rate based on current RTT 
                            //  and recever-provded loss estimate
  double maxrate_;          // maximum rate during slowstart (prevents
                            //  sending at more than 2 times the 
                            //  rate at which the receiver is _receiving_)

  double inter_packet;      // inter packet gap

  double max_rtt_;          // (assumed) maximum RTT of all receivers
  double t_factor_;         // T = t_factor_ * max_rtt_
  double round_begin_;      // time when fb round started

  double last_change_;      // time last change in rate was made
  double ssmult_;           // during slow start, increase rate by this 
                            //  factor every rtt
  int bval_;                // value of B for the formula
  double overhead_;         // if > 0, dither outgoing packets 
  int ndatapack_;     // number of packets sent
  int round_id;             // round id

  double supp_rate_;

  // extensions for multicast
  int    rtt_recv_id;
  int    rtt_prio;
  double rtt_recv_timestamp;
  double rtt_recv_last_feedback;
  double rtt_rate;

  // CLR
  int    clr_id;
  double clr_timestamp;
  double clr_last_feedback;
} TfmccSrcInfo;


Agent * TfmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat);
void TfmccSrcStart (Agent * agent);
void TfmccSrcStop (Agent * agent);
void TfmccSrcPktHandler (Agent * agent, Packet * p);
void TfmccSrcSlowStart (Agent * agent);
inline void TfmccSrcIncrRate (Agent * agent);
inline void TfmccSrcDecrRate (Agent * agent);
void TfmccSrcSendPkt (Agent * agent);
void TfmccSrcTimer (Agent * agent, int timerId, int serial);
void TfmccSrcPrintStat (Agent * agent);

#endif
