#ifndef ROSS_MCAST_GMCC_RCV_H
#define ROSS_MCAST_GMCC_RCV_H

#include "gmcc-snd.h"
#include "gmcc-util.h"


#define GMCC_CI_PKT_SIZE                40
#define GMCC_RCV_TAF_EWMA               0.125
#define GMCC_RCV_JUMP_FACTOR            2.58
#define GMCC_RCV_JUMP_FACTOR2           2
#define GMCC_RCV_LESS_CG2_CO            1.1
#define GMCC_RCV_MORE_CG_FACTOR         3.5
#define GMCC_RCV_CHANGE_CR_FACTOR       1.64
#define GMCC_RCV_CHANGE_CR_FACTOR2      1.25
#define GMCC_RCV_DEF_OS_TOLERENCE       1.64
#define GMCC_RCV_SAMPLE_SIZE            30
#define GMCC_RCV_REQD_SAMPLE_SIZE       30
#define GMCC_RCV_REQD_SAMPLE_SIZE2      10
#define GMCC_RCV_CG_ITVL_SAMPLE_SIZE    30
#define GMCC_RCV_CONFIDENT_NUM          30
#define GMCC_RCV_CONFIDENT_NUM2         30
#define GMCC_RCV_CONFIDENT_NUM3         10
#define GMCC_RCV_TTL_RATE_SAMPLE_ITVL   0.5
#define GMCC_RCV_OVS_TOLR_DECR_FACTOR   0.98
#define GMCC_RCV_OVS_TOLR_INCR_FACTOR   1.02


typedef struct GmccRcvInfoStruct {
  int8 active_, trackStat_;

  RateAvgrData rcvRateAvg_[GMCC_MAX_LAYER_NUM];
  AvgCalcData  cgItvlAvgCalc_[GMCC_MAX_LAYER_NUM],
               itafAvg_[GMCC_MAX_LAYER_NUM],
               itafAvg2_[GMCC_MAX_LAYER_NUM];
  StatEstimatorData 
    itafEst_[GMCC_MAX_LAYER_NUM],
    tafEst_[GMCC_MAX_LAYER_NUM],
    itafEst2_[GMCC_MAX_LAYER_NUM];  /* For oversent packets only */
  
  int layerNum_,    /* Number of layers/groups from the source */
      topActLayer_, /* Top active layer, no matter joined or not */
      topJoinedLayer_,
      topJoinedActLayer_,
      lastJoinFrom_,
      lastJoinedLayer_; /* -1: invalid */
  short activeLayer_[GMCC_MAX_LAYER_NUM], /* Record which layer is active */ 
        joinedLayer_[GMCC_MAX_LAYER_NUM], /* Record which layer this receiver
                                           * has joined. */
        joinResult_;                 /* Result of last join:
                                      * -1: fail, 0: unknown, 1: successful */
  uint32 senderAddr_, senderAgent_;
  
  uint32 lastContSqn_[GMCC_MAX_LAYER_NUM],  /* Last continuous sqn */
         contPktNum_[GMCC_MAX_LAYER_NUM],   /* Continuous pkt number */
         totalRcvd_, itvlRcvd_, ciSent_;
  double crTafAvg_[GMCC_MAX_LAYER_NUM],
         crTafDev_[GMCC_MAX_LAYER_NUM],      /* TAF avg and dev of CRs */
         lastRcvTime_[GMCC_MAX_LAYER_NUM],
         lastCongTime_[GMCC_MAX_LAYER_NUM],
         lastResetTime_[GMCC_MAX_LAYER_NUM],
         lastCiTime_[GMCC_MAX_LAYER_NUM],
         lastSndRate_[GMCC_MAX_LAYER_NUM],
         lastCor_[GMCC_MAX_LAYER_NUM],
         osTolr_[GMCC_MAX_LAYER_NUM],   /* Oversend tolerence factor */
         lastJoinOsTolr_,               /* Last oversend tolerence factor
                                         * triggering the join */
         grttAvg_, grttDev_,
         srtt_[GMCC_MAX_LAYER_NUM],
         begTime_, lastRateRecTime_;
  int sampleNum_[GMCC_MAX_LAYER_NUM], ovsdSampleNum_[GMCC_MAX_LAYER_NUM],
      lessCgNum_[GMCC_MAX_LAYER_NUM], moreCgNum_[GMCC_MAX_LAYER_NUM],
      lessCgNum2_[GMCC_MAX_LAYER_NUM]; /* For comparison at ctrl pkt arrival
                                        * when there is no loss */

  uint32 cr_[GMCC_MAX_LAYER_NUM];
  uint32 grpAddrs_[GMCC_MAX_LAYER_NUM];  /* Group addresses for layers */
         
  /* Configuarable parameters */       
  double tafEwma_,            /* TAF EWMA factor */
         jumpFactor_, jumpFactor2_, lessCg2Co_,
         changeCrFactor_, changeCrFactor2_,
         moreCgFactor_,
         ttlRateSampleItvl_,  /* For total rcv rate output */
         defOsTolerence_,     /* Defautl tolerance factor while sender ignors
                               * congestion (oversend) */
         ovsTolrIncrFactor_,  /* Increase when a join failed */
         ovsTolrDecrFactor_;  /* Decrease when a join succeeded */  

  int sampleSize_, /* Used to calculate mean and dev of TAF */
      cgItvlSampleSize_,
      reqdSampleSize_,        /* Required sample size before comparision */
      reqdSampleSize2_,       /* Required sample size before comparision 
                               * for oversend */
      confidentNum_,          /* If the TAF is less or greater than that of CR
                               * for this number of samples, then consider
                               * it is ture */
      confidentNum2_,         /* For lessCgNum2_ */
      confidentNum3_;         /* For moreCgNum_ */
} GmccRcvInfo;

Agent * GmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr, 
                     int8 trackStat);
void GmccRcvStart (Agent * agent);
void GmccRcvStop (Agent * agent);
void GmccRcvPktHandler (Agent * agent, Packet * p);
void GmccRcvTimer (Agent * agent, int timerId);
void GmccRcvPrintStat (Agent * agent);

void GmccRcvProcCtrlPkt (Agent * agent, Packet * p);  
void GmccRcvProcDataPkt (Agent * agent, Packet * p);
void GmccRcvInitLayer (Agent * agent, int layer);
void GmccRcvInitLayer2 (Agent * agent, int layer, int refLayer);
int GmccRcvJump (Agent * agent, double timestamp, int reason);
int GmccRcvLeave (Agent * agent, int layer, int reason);
void GmccRcvCheckGoodJoin (Agent * agent);
void GmccRcvUpdateCgCmp (Agent * agent, int i);
void GmccRcvSendCiPkt  (Agent * agent, double timestamp, int layer, 
                        double rcvRate, short mature, short keepalive);


#endif
