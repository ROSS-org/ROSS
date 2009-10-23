#ifndef ROSS_MCAST_GMCC_SND_H
#define ROSS_MCAST_GMCC_SND_H

#include "Packet.h"
#include "Transport.h"
#include "gmcc-util.h"

#define GMCC_DATA_PKT_SIZE              1000
#define GMCC_CTRL_PKT_SIZE              40

#define GMCC_INIT_CONG_ITVL             1
#define GMCC_TRAC_EWMA                  0.1
#define GMCC_CR_CHECK_EWMA              0.1

#define GMCC_SRC_MIN_RATE               GMCC_DATA_PKT_SIZE
#define GMCC_SRC_INIT_RTT               0.1
#define GMCC_SRC_RTT_EWMA               0.125
#define GMCC_SRC_GRTT_EWMA              0.02
#define GMCC_SRC_SAMPLE_SIZE            30
#define GMCC_SRC_LAYER_NUM              16
#define GMCC_SRC_RATE_INCR_RTT_FACTOR   2
#define GMCC_SRC_RATE_CUT_FACTOR        0.75
#define GMCC_SRC_CHANGE_CR_FACTOR       1.64
#define GMCC_SRC_CHANGE_CR_FACTOR2      1.25
#define GMCC_SRC_OVERSEND_FRAC          0.1
//#define GMCC_SRC_OVERSEND_PROB          0.5
#define GMCC_SRC_OVERSEND_PROB          0
#define GMCC_SRC_CR_EXT_SEARCH_SPAN     4
#define GMCC_SRC_KEEPALIVE_SPAN         20


enum {
  GMCC_SRC_RATE_INCR_TIMER,
  GMCC_SRC_SEND_PKT_TIMER,
  GMCC_SRC_CONG_EPOCH_TIMER,
  GMCC_SRC_CR_CHECK_TIMER,
  GMCC_SRC_CR_EXT_SEARCH_TIMER,
  GMCC_SRC_KEEPALIVE_TIMER
}; 


typedef struct GmccSrcInfoStruct {
  uint32 pktDstAddr_;       /* Packet destination address */
  uint16 pktDstAgentId_;
  uint32 pktSize_;

  int8   active_;

  int layerNum_,    /* Number of layers/groups this source can 
                     * support */
      topActLayer_, /* Top active layer */
      sampleSize_,
      oversendLayer_;
  uint32 ctrlPktId_;
  short activeLayer_[GMCC_MAX_LAYER_NUM];  /* Indicate which layer is active,
                                            * i.e. has data in transmission */

  uint32 grpAddrs_[GMCC_MAX_LAYER_NUM];    /* Group addresses for layers */

  uint32 sqn_[GMCC_MAX_LAYER_NUM];         /* Sequence numbers, increasing
                                            * monotonouslly */

  double grttAvg_, grttDev_, grttEwma_,    /* Global RTT */
         lastCtrlPktTime_;
         
  double rate_[GMCC_MAX_LAYER_NUM],
         rttAvg_[GMCC_MAX_LAYER_NUM], 
         rttDev_[GMCC_MAX_LAYER_NUM],  /* Sending rate in bytes/sec */
         crTafAvg_[GMCC_MAX_LAYER_NUM], crTafDev_[GMCC_MAX_LAYER_NUM], 
         debtNextLayer_[GMCC_MAX_LAYER_NUM]; /* Traffic volume to be balanced 
                                              * by next layer */
         
  RateAvgrData sndRateAvg_[GMCC_MAX_LAYER_NUM];
  StatEstimatorData grttEst_;

  tw_event * rateIncrTimerEvnt_[GMCC_MAX_LAYER_NUM],
           * sendPktTimerEvnt_[GMCC_MAX_LAYER_NUM],
           * congEpochTimerEvnt_[GMCC_MAX_LAYER_NUM],
           * crCheckTimerEvnt_[GMCC_MAX_LAYER_NUM],
           * crExtSearchTimerEvnt_[GMCC_MAX_LAYER_NUM],
           * keepaliveTimerEvnt_[GMCC_MAX_LAYER_NUM];

  uint32 cr_[GMCC_MAX_LAYER_NUM];  /* Congestion representatives */
} GmccSrcInfo;



Agent * GmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId);
void GmccSrcStart (Agent * agent);
void GmccSrcStop (Agent * agent);
void GmccSrcPktHandler (Agent * agent, Packet * p);
void GmccSrcTimer (Agent * agent, int timerId, int serial);
void GmccSrcPrintStat (Agent * agent);

void GmccSrcInitLayer (Agent * agent, int layer);
void GmccSrcResetLayer (Agent * agent, int layer);
void GmccSrcSendCtrlPkt (Agent * agent, int layer);
void GmccSrcSendDataPkt (Agent * agent, int layer);

#endif
