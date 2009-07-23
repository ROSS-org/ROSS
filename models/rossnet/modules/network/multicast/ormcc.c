#include "ormcc.h"


uint32 _totalOrmccCiSent = 0, _totalOrmccCiSupp = 0;


/***********************************
 O R M C C  S O U R C E
 ***********************************/


Agent * OrmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat)
{
  Agent * a = GetAgent (nodeId, (uint32) agentId);
  OrmccSrcInfo * osi;
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, _nodeTable[nodeId].addr_, 
                     (PktHandler) OrmccSrcPktHandler, 
                     (StartFunc) OrmccSrcStart,
                     (StopFunc) OrmccSrcStop,
                     (TimerFunc) OrmccSrcTimer,
                     (AgentFunc) OrmccSrcPrintStat,
                     NULL);
  osi = malloc (sizeof (OrmccSrcInfo));
  
  if (osi == NULL) {
    printf ("Failed to allocate memory for ORMCC source info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) osi, 0, sizeof (OrmccSrcInfo));
  osi->pktDstAddr_ = pktDstAddr;
  osi->pktDstAgentId_ = pktDstAgentId;
  a->info_ = osi;
  osi->trackStat_ = trackStat;

  return a;
}


void OrmccSrcStart (Agent * agent)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;

  osi->pktSize_ = ORMCC_SRC_DATA_PKT_SIZE;
  osi->crGone_ = 1;
  osi->crAddr_ = (uint32) -1;
  osi->sqn_ = 0;
  osi->rate_ = ORMCC_SRC_INIT_RATE;
  osi->maxRtt_ = osi->rttAvg_ = ORMCC_SRC_INIT_RTT;
  osi->rttDev_ = 0;
  osi->crTracAvg_ = -1;
  osi->crTracDev_ = 0;
  osi->crChkBegTime_ = -1;
  osi->crRespTimeAvg_ = 100; /* Set to large so that it won't time out */
  osi->crRespTimeDev_ = -1;
  osi->active_ = 1;
  osi->ciRcvd_ = 0;

#ifdef TRACE_ORMCC_THROUGHPUT
  osi->thruSampleTime_ = tw_now (tw_getlp (agent->nodeId_));
  osi->totalSentBytes_ = 0;
#endif

  OrmccSrcSendPkt (agent);
  osi->sendPktTimerEvnt_ = ScheduleTimer 
    (agent, ORMCC_SRC_TIMER_SEND_PKT, osi->pktSize_ / osi->rate_);
  osi->rateIncrTimerEvnt_ = ScheduleTimer
    (agent, ORMCC_SRC_TIMER_RATE_INC, osi->rttAvg_);
  osi->crRespTimerEvnt_ = osi->congEpochTimerEvnt_ = NULL;
}


void OrmccSrcStop (Agent * agent)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);

  osi->active_ = 0;
  if (osi->sendPktTimerEvnt_ != NULL) {
    tw_timer_cancel (lp, & (osi->sendPktTimerEvnt_));
    osi->sendPktTimerEvnt_ = NULL;
  }
  if (osi->rateIncrTimerEvnt_ != NULL) {
    tw_timer_cancel (lp, & (osi->rateIncrTimerEvnt_));
    osi->rateIncrTimerEvnt_ = NULL;
  }
  if (osi->congEpochTimerEvnt_ != NULL) {
    tw_timer_cancel (lp, & (osi->congEpochTimerEvnt_));
    osi->congEpochTimerEvnt_ = NULL;
  }
  if (osi->crRespTimerEvnt_ != NULL) {
    tw_timer_cancel (lp, & (osi->crRespTimerEvnt_));
    osi->crRespTimerEvnt_ = NULL;
  }
}


void OrmccSrcSendPkt (Agent * agent)
{
  Packet p;
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  OrmccData * data = & (p.type.od_);
  
  if (! osi->active_) return;
  
  p.type_ = ORMCC_DATA;
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = osi->pktDstAddr_;
  p.dstAgent_ = osi->pktDstAgentId_;
  p.size_ = osi->pktSize_;
  p.inPort_ = (uint16) -1;
  data->sqn_ = osi->sqn_ ++;
  data->ts_ = now;
  data->crAddr_ = osi->crAddr_;
  data->crTracAvg_ = osi->crGone_ ? - 1 : osi->crTracAvg_;
  data->crTracDev_ = osi->crTracDev_;
  data->maxRtt_ = osi->maxRtt_;
    
  SendPacket (lp, &p);

#ifdef TRACE_ORMCC_THROUGHPUT
  if (! osi->trackStat_) return;

  osi->totalSentBytes_ += p.size_;
  if (now - osi->thruSampleTime_ < ORMCC_THROUGHPUT_SAMPLE_ITVL) {
    return;
  }
   
  /* For rate tracing */
  printf ("%lf : at %d.%ld , throughput rate = %lf Mbps, itvl = %lf\n",
    now, lp->id, agent->id_, 
    osi->totalSentBytes_ / (now - osi->thruSampleTime_) / 125000,
    now - osi->thruSampleTime_);
  osi->thruSampleTime_ = now;
  osi->totalSentBytes_ = 0;
#endif
}


void OrmccSrcPktHandler (Agent * agent, Packet * p)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  int cutRate = 0, changeCr = 0;
  double err, rttSample;
  OrmccCi * ci = & (p->type.oc_);

  if (! osi->active_) return;


#ifdef ORMCC_TEST_CI_LOSS
  /* Simulate CI loss */
  if (IsMcastAddr (osi->pktDstAddr_)) {
    if ((rand () / (double) RAND_MAX) <= 0.01) return;
  } 
#endif
  
  if (p->type_ != ORMCC_CI) {
    printf ("ORMCC source (%d.%lu) received non-CI packet.\n",
       lp->id, agent->id_);
    tw_exit (-1);
  }
  
  ++ osi->ciRcvd_;
  rttSample = now - ci->tsEcho_;

#ifdef DEBUG_ORMCC_CI_PKT
  if (IsMcastAddr (p->srcAgent_)) {
    printf ("%lf : CI(%lu, %lf) from %d.%ld arrived. RTT sample = %lf\n",
      now, ci->sqn_, ci->tsEcho_, _addrToIdx[p->srcAddr_], 
      - GetGrpAddr4Print (p->srcAgent_), rttSample);
  }
  else {
    printf ("%lf : CI(%lu, %lf) from %d.%d arrived. RTT sample = %lf\n", 
      now, ci->sqn_, ci->tsEcho_, _addrToIdx[p->srcAddr_], p->srcAgent_,
      rttSample);
  }
#endif  
  
  /*
   * Fileter CIs, update CR / cut rate if necessary
   */  
  do {
    ++ osi->ciFromCr_;

    /* Initialization or CCI is gone. */
    if (osi->crGone_ && p->srcAddr_ != osi->crAddr_) {
      changeCr = 1;
      break;
    }
    
    /* CI is from CR */
    if (p->srcAddr_ == osi->crAddr_) { 
      osi->crGone_ = 0;
      err = ci->trac_ - osi->crTracAvg_;
      osi->crTracAvg_ += ORMCC_TRAC_EWMA_FACTOR * err;
      osi->crTracDev_ += ORMCC_TRAC_EWMA_FACTOR 
                         * (fabs (err) - osi->crTracDev_);
      cutRate = 1;
      OrmccSrcUpdateRtt (agent, rttSample);
      
      if (osi->crChkBegTime_ < 0) break;
      
      if (osi->crRespTimeDev_ < 0) {
        osi->crRespTimeAvg_ = now - osi->crChkBegTime_;
        osi->crRespTimeDev_ = 0;
      }
      else {
        err = (now - osi->crChkBegTime_) - osi->crRespTimeAvg_;
        osi->crRespTimeAvg_ += 0.125 * err;
        osi->crRespTimeDev_ += 0.125 * (fabs (err) - osi->crRespTimeDev_);
      }
      osi->crChkBegTime_ = -1;
      if (osi->crRespTimerEvnt_ != NULL) {
        tw_timer_cancel (lp, &(osi->crRespTimerEvnt_));
        osi->crRespTimerEvnt_ = NULL;
      }
      break;
    }
    
    if (ci->trac_ + SMALL_FLOAT
        < osi->crTracAvg_ - osi->crTracDev_ / ORMCC_COMPARE_TRAC_FACTOR) {
      changeCr = 1;
      break;
    }

    -- osi->ciFromCr_;
  } while (0);
  
  /*
   * Update CR
   */
  if (changeCr) {
    if (osi->crTracAvg_ < 0) {
      printf ("%lf : at %d.%ld CR is initialized as %d\n",
        now, lp->id, agent->id_,(int) _addrToIdx[p->srcAddr_]);
    }
    else {
      printf ("%lf : at %d.%ld CR is changed from %d to %d\n",
        now, lp->id, agent->id_, _addrToIdx[osi->crAddr_],
        _addrToIdx[p->srcAddr_]);
    }
    
    /*
    if (osi->crChkBegTime_ > 0 
        && now - osi->crChkBegTime_ > osi->rttAvg_ + 4 * osi->rttDev_) {
      if (osi->crRespTimeDev_ < 0) {
        osi->crRespTimeAvg_ = now - osi->crChkBegTime_;
        osi->crRespTimeDev_ = 0;
      }
      else {
        err = (now - osi->crChkBegTime_) - osi->crRespTimeAvg_;
        osi->crRespTimeAvg_ += 0.125 * err;
        osi->crRespTimeDev_ += 0.125 * (fabs (err) - osi->crRespTimeDev_);
      }
    }
    */

    osi->crChkBegTime_ = -1;
    if (osi->crRespTimerEvnt_ != NULL) {
      tw_timer_cancel (lp, &(osi->crRespTimerEvnt_));
      osi->crRespTimerEvnt_ = NULL;
    }
    
    OrmccSrcUpdateRtt (agent, rttSample);    
    osi->crGone_ = 0;    
    osi->crAddr_ = p->srcAddr_;
    cutRate = 1;
    osi->crTracAvg_ = ci->trac_;
  }
  
  if (osi->congEpochTimerEvnt_ != NULL) {
    if (cutRate) ++ osi->notCut_;
    cutRate = 0;
  }
        
  if (cutRate) {
    if (osi->rate_ > 0.75 * ci->trac_) osi->rate_ = 0.75 * ci->trac_;
    if (osi->rate_ < ORMCC_SRC_MIN_RATE) osi->rate_ = ORMCC_SRC_MIN_RATE;
    osi->congEpochTimerEvnt_ = ScheduleTimer (agent,
      ORMCC_SRC_TIMER_CONG_EPOCH, osi->rttAvg_ + 4 * osi->rttDev_);    

    if (osi->rateIncrTimerEvnt_ != NULL) {
      tw_timer_cancel (lp, & (osi->rateIncrTimerEvnt_));
      osi->rateIncrTimerEvnt_ = NULL;
    }

#ifdef TRACE_ORMCC_SRC_RATE_DYN
    /* For rate tracing */
    if (osi->trackStat_) {
      printf ("%lf : at %d.%ld , Rate decreased = %lf Mbps\n",
        now, lp->id, agent->id_, osi->rate_ / 125000);
    }
#endif      
  }
}


void OrmccSrcTimer (Agent * agent, int timerId, int serial)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;
  double srtt, now, oldRate;

  now = tw_now (tw_getlp (agent->nodeId_));
  
  switch (timerId) {
  case ORMCC_SRC_TIMER_RATE_INC:
    if (osi->congEpochTimerEvnt_ != NULL) {
      osi->rateIncrTimerEvnt_ = NULL;
      break;
    }
    srtt = osi->rttAvg_ + 2 * osi->rttDev_;
    osi->rateIncrTimerEvnt_ = ScheduleTimer 
      (agent, ORMCC_SRC_TIMER_RATE_INC, srtt);

    oldRate = osi->rate_;
    osi->rate_ += osi->pktSize_ / srtt;
        
    if (osi->crTracAvg_ > 0 
        && oldRate < osi->crTracAvg_ + 4 * osi->crTracDev_
        && osi->rate_ >= osi->crTracAvg_ + 4 * osi->crTracDev_
        && osi->crRespTimerEvnt_ == NULL) {
      osi->crChkBegTime_ = now;
      osi->crRespTimerEvnt_ = ScheduleTimer (agent, 
        ORMCC_SRC_TIMER_CR_RESP, osi->crRespTimeAvg_ + 8 * osi->crRespTimeDev_);
        
        
      if (IsMcastAddr (osi->pktDstAddr_)) {
        printf ("%lf : at %d.%ld , CR resp timer %lf, %lf, %lf\n",
          now, agent->nodeId_, agent->id_, osi->crRespTimeAvg_ + 8 * osi->crRespTimeDev_, osi->crRespTimeAvg_, osi->crRespTimeDev_);
      }
    }
    
#ifdef TRACE_ORMCC_SRC_RATE_DYN 
    /* For rate tracing */
    if (osi->trackStat_) {
      printf ("%lf : at %d.%ld , Rate increased = %lf Mbps\n",
        now, agent->nodeId_, agent->id_, osi->rate_ / 125000);
    }
#endif      
    break;
  case ORMCC_SRC_TIMER_CONG_EPOCH:
    osi->congEpochTimerEvnt_ = NULL;
    if (osi->rateIncrTimerEvnt_ == NULL) {
      osi->rateIncrTimerEvnt_ = ScheduleTimer (agent,
        ORMCC_SRC_TIMER_RATE_INC, osi->rttAvg_ + 2 * osi->rttDev_);
    }  
    break;
  case ORMCC_SRC_TIMER_SEND_PKT:
    OrmccSrcSendPkt (agent);
    osi->sendPktTimerEvnt_ = ScheduleTimer 
      (agent, ORMCC_SRC_TIMER_SEND_PKT, osi->pktSize_ / osi->rate_);
    break;
  case ORMCC_SRC_TIMER_CR_RESP:
    osi->crGone_ = 1;
    osi->crRespTimerEvnt_ = NULL;
    break;
  default:
    printf ("Unknow ORMCC source timer.\n");
    tw_exit (-1);
  }
}


void OrmccSrcUpdateRtt (Agent * agent, double sample)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;
  double err = sample - osi->rttAvg_;

  if (sample > osi->maxRtt_) osi->maxRtt_ = sample;
  osi->rttAvg_ += 0.125 * err;
  osi->rttDev_ += 0.125 * (fabs (err) - osi->rttDev_);

#ifdef TRACE_ORMCC_RTT
  if (! osi->trackStat_) return;
  
  printf (
    "%lf : at %d.%ld , RTT updated = %lf , RTT dev = %lf , sample = %lf\n",
    tw_now ( tw_getlp (agent->nodeId_)), agent->nodeId_, agent->id_,
    osi->rttAvg_, osi->rttDev_, sample);
#endif    
}


void OrmccSrcPrintStat (Agent * agent)
{
  OrmccSrcInfo * osi = (OrmccSrcInfo *) agent->info_;

  printf ("ORMCC source statistics: CI received = %lu, from CR = %lu, ",
    osi->ciRcvd_, osi->ciFromCr_);
  printf ("Rate cut omitted = %lu\n", osi->notCut_);
}


/***********************************
 O R M C C  R E C E I V E R
 ***********************************/


Agent * OrmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr)
{
  Agent * a;
  OrmccRcvInfo * ori;
  
  if (IsMcastAddr (listenAddr)) {
    a = GetAgent (nodeId, listenAddr);
  }
  else {
    a = GetAgent (nodeId, (uint32) agentId);
  }
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, listenAddr, 
                     (PktHandler) OrmccRcvPktHandler, 
                     (StartFunc) OrmccRcvStart,
                     (StopFunc) OrmccRcvStop,
                     (TimerFunc) OrmccRcvTimer,
                     (AgentFunc) OrmccRcvPrintStat,
                     NULL);

  ori = malloc (sizeof (OrmccRcvInfo));
  
  if (ori == NULL) {
    printf ("Failed to allocate memory for ORMCC receiver info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) ori, 0, sizeof (OrmccRcvInfo));
  a->info_ = ori;
             
  return a;                             
}


void OrmccRcvStart (Agent * agent)
{
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;
  
  ori->active_ = 1;
  ori->crAddr_ = _nodeTable[agent->nodeId_].addr_;  /* Init CR as this node */
  ori->rate_ = ori->tracAvg_ = ori->crTracAvg_ = ori->lastPktTime_ = -1;
  ori->crTracDev_ = 0;
  ori->lastContSqn_ = 0;
  
  RateSmoothInit (& (ori->rsd_), RATE_SMOOTH_TIME);
}


void OrmccRcvStop (Agent * agent)
{
  int aid = agent->id_;
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;
  
  ori->active_ = 0;
  
  if (IsMcastAddr (aid)) aid = - GetGrpAddr4Print (aid);
  
  CancelTimer (agent, & (ori->ciBackoffTimerEvnt_));
}


void OrmccRcvPktHandler (Agent * agent, Packet * p)
{
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  OrmccData * dp = &(p->type.od_);
  double now = tw_now (lp);
  double rateSample = 0, backoff;
    
  if (! ori->active_) return;  

  if (p->type_ != ORMCC_DATA) {
    printf ("ORMCC receiver (%d.%lu) received non-data packet.\n",
       lp->id, agent->id_);
    tw_exit (-1);
  }
    
  if (ori->lastPktTime_ < 0) {  /* Initialization */
    ori->lastContSqn_ = dp->sqn_ - 1;
    rateSample = p->size_ / (ori->lastPktTime_ = now);
  }
  else {
    rateSample = p->size_ / (now - ori->lastPktTime_);
    ori->lastPktTime_ = now;
  }
  ori->rate_ = RateSmoothAddSample (& (ori->rsd_), rateSample, now);
  
#ifdef DEBUG_ORMCC_RCV_RATE
  printf ("%lf : Node %d.%ld recv rate = %lf Mbps, avg rate = %lf Mbps\n", 
    now, lp->id, agent->id_, rateSample / 125000, ori->rate_ / 125000);
#endif    

  if (ori->crAddr_ != _nodeTable[agent->nodeId_].addr_
      && dp->crAddr_ == _nodeTable[agent->nodeId_].addr_) {
    ori->crBegTime_ = now;
  }
  
  ori->crAddr_ = dp->crAddr_;
  ori->crTracAvg_ = dp->crTracAvg_;
  ori->crTracDev_ = dp->crTracDev_;

  /*
   * Check whether should cancel scheduled CI 
   */
  if (ori->ciBackoffTimerEvnt_ != NULL 
      && ori->crAddr_ != _nodeTable[lp->id].addr_
      && ori->crTracAvg_ >= 0
      && ori->tracAvg_ + SMALL_FLOAT 
         >= ori->crTracAvg_ - ori->crTracDev_ / ORMCC_COMPARE_TRAC_FACTOR) {
    tw_timer_cancel (lp, & (ori->ciBackoffTimerEvnt_));
    ++ ori->ciSupp_;
    if (IsMcastAddr (agent->listenAddr_))  ++ _totalOrmccCiSupp;
  }
  
  if (dp->sqn_ == ori->lastContSqn_ + 1) {
    ++ ori->lastContSqn_;
    return;
  }

  /* For simplicity, assume no out-of-order packets */
  if (dp->sqn_ < ori->lastContSqn_) {
    return;
  }

#ifdef DEBUG_ORMCC_DATA_PKT
  printf ("%lf : pkt(%lu, %lf) from %d.%d arrived. last sqn = %lu\n", 
    now, dp->sqn_, dp->ts_,
    _addrToIdx[p->srcAddr_], p->srcAgent_, ori->lastContSqn_);
#endif    

  /* Now there are some losses */
  ori->lastContSqn_ = dp->sqn_;
  
  if (ori->tracAvg_ < 0) {
    /* Use crTracAvg_ to avoid unnecessary CR switch */
    ori->tracAvg_ = ori->crTracAvg_ < 0 ? ori->rate_ : ori->crTracAvg_;
  }
  else {
    ori->tracAvg_ = (1 - ORMCC_TRAC_EWMA_FACTOR) * ori->tracAvg_
      + ORMCC_TRAC_EWMA_FACTOR * ori->rate_;
  }
  
  if (ori->crAddr_ == _nodeTable[lp->id].addr_) {
    if (ori->ciBackoffTimerEvnt_ != NULL) {
      tw_timer_cancel (lp, & (ori->ciBackoffTimerEvnt_));
      ++ ori->ciSupp_;
    }

#ifdef ORMCC_TEST_CR_SWITCH
    if (IsMcastAddr (agent->listenAddr_) && now - ori->crBegTime_ > 20) {
      return;
    }
#endif    
    
    ori->ciSqn_ = dp->sqn_;
    ori->ciTsEcho_ = dp->ts_;
    ori->ciSchedTime_ = now;
    ori->senderAddr_ = p->srcAddr_;
    ori->senderAgent_ = p->srcAgent_;
    OrmccRcvSendPkt (agent);
    
    return;
  }
  
  if (ori->ciBackoffTimerEvnt_ == NULL
      && (ori->crTracAvg_ < 0 
          || ori->tracAvg_ + SMALL_FLOAT 
             < ori->crTracAvg_ - ori->crTracDev_ / ORMCC_COMPARE_TRAC_FACTOR)) {
    ori->ciSqn_ = dp->sqn_;
    ori->ciTsEcho_ = dp->ts_;
    ori->ciSchedTime_ = now;
    ori->senderAddr_ = p->srcAddr_;
    ori->senderAgent_ = p->srcAgent_;
    backoff = 2 * dp->maxRtt_ / 10 
      * log ((exp (10) - 1) * (rand () / (double) RAND_MAX) + 1);

    ori->ciBackoffTimerEvnt_ = ScheduleTimer 
      (agent, ORMCC_RCV_CI_BACKOFF_TIMER, backoff);
      
    return;
  }

  ++ ori->ciSupp_;
  if (IsMcastAddr (agent->listenAddr_))  ++ _totalOrmccCiSupp;
}


void OrmccRcvSendPkt (Agent * agent)
{
  Packet p;
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  
  if (! ori->active_) return;

  p.type_ = ORMCC_CI;  
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = ori->senderAddr_;
  p.dstAgent_ = ori->senderAgent_;
  p.size_ = ORMCC_RCV_CI_PKT_SIZE;
  p.inPort_ = (uint16) -1;
  
  p.type.oc_.sqn_ = ori->ciSqn_;
  p.type.oc_.tsEcho_ = ori->ciTsEcho_ + tw_now (lp) - ori->ciSchedTime_;
  p.type.oc_.trac_ = ori->rate_;
    
  SendPacket (lp, &p);
  ++ ori->ciSent_;
  if (IsMcastAddr (agent->listenAddr_))  ++ _totalOrmccCiSent;
}


void OrmccRcvTimer (Agent * agent, int timerId, int serial)
{
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;

  switch (timerId) {
  case ORMCC_RCV_CI_BACKOFF_TIMER:
    ori->ciBackoffTimerEvnt_ = NULL;
    OrmccRcvSendPkt (agent);
    break;
  default:
    printf ("Unknown ORMCC receiver timer type.\n");
    tw_exit (-1);
  }
}


void OrmccRcvPrintStat (Agent * agent)
{
  OrmccRcvInfo * ori = (OrmccRcvInfo *) agent->info_;

  printf 
    ("ORMCC receiver at %d.%lu statistics: CI sent = %u, CI suppressed = %u, ",
    agent->nodeId_, GetGrpAddr4Print (agent->id_), ori->ciSent_, ori->ciSupp_);
    
  printf ("total CI sent = %lu, total CI suppressed = %lu\n",
    _totalOrmccCiSent, _totalOrmccCiSupp);
}


/***********************************
 R A T E   S M O O T H E R
 ***********************************/


void RateSmoothInit (RateSmoothData * rsd, double span)
{
  rsd->span_ = span;
  rsd->totalData_ = 0;
  rsd->head_ = rsd->tail_ = 0;
  memset ((void *) rsd->sample_, 0, 
          sizeof (RateSample) * RATE_SMOOTH_RING_SIZE);
}


double RateSmoothAddSample (RateSmoothData * rsd, double rate, double time)
{
  int tmp;

  rsd->sample_[rsd->tail_].time_ = time;
  rsd->sample_[rsd->tail_].rate_ = rate;

  if (rsd->head_ == rsd->tail_) {  /* was empty */
    rsd->tail_ = (rsd->tail_ + 1) % RATE_SMOOTH_RING_SIZE;  
    rsd->totalData_ = 0;
    return rate;
  }

  rsd->tail_ = (rsd->tail_ + 1) % RATE_SMOOTH_RING_SIZE;
  tmp = (rsd->tail_ + RATE_SMOOTH_RING_SIZE - 2) % RATE_SMOOTH_RING_SIZE;
  rsd->totalData_ += rate * (time - rsd->sample_[tmp].time_);

  while (time - rsd->sample_[rsd->head_].time_ > rsd->span_ + SMALL_FLOAT
         || rsd->tail_ == rsd->head_) {
    tmp = (rsd->head_ + 1) % RATE_SMOOTH_RING_SIZE;
    rsd->totalData_ -= 
      (rsd->sample_[tmp].time_ - rsd->sample_[rsd->head_].time_) 
      * rsd->sample_[tmp].rate_;
    rsd->head_ = tmp;
  }
  
  if ((rsd->head_ + 1) % RATE_SMOOTH_RING_SIZE == rsd->tail_) {
    /* has only one recod */
    rsd->totalData_ = 0;
    return rate;
  }
  
  return rsd->totalData_ / (time - rsd->sample_[rsd->head_].time_);
}
