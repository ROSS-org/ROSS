#include "tfmcc.h"


/***********************************
 T F M C C  S O U R C E
 ***********************************/


Agent * TfmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat)
{
  Agent * a = GetAgent (nodeId, (uint32) agentId);
  TfmccSrcInfo * tsi;
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, _nodeTable[nodeId].addr_, 
                     (PktHandler) TfmccSrcPktHandler, 
                     (StartFunc) TfmccSrcStart,
                     (StopFunc) TfmccSrcStop,
                     (TimerFunc) TfmccSrcTimer,
                     (AgentFunc) TfmccSrcPrintStat,
                     NULL);
  tsi = malloc (sizeof (TfmccSrcInfo));
  
  if (tsi == NULL) {
    printf ("Failed to allocate memory for ORMCC source info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) tsi, 0, sizeof (TfmccSrcInfo));
  tsi->pktDstAddr_ = pktDstAddr;
  tsi->pktDstAgentId_ = pktDstAgentId;
  a->info_ = tsi;
  tsi->trackStat_ = trackStat;

  return a;
}


void TfmccSrcStart (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  
  tsi->active_ = 1;
  
  tsi->pktSize_ = TFMCC_SRC_DATA_PKT_SIZE;
  tsi->fairsize_ = TFMCC_SRC_DATA_PKT_SIZE;
  tsi->ndatapack_ = 0;
  tsi->overhead_ = 0;
  tsi->ssmult_ = 2.0;
  tsi->bval_ = 1;
  tsi->t_factor_ = 6;

  tsi->seqno_ = 0;
  tsi->oldrate_ = tsi->rate_ = TFMCC_SRC_INIT_RATE;
  tsi->delta_ = 0;
  tsi->rate_change_ = TFMCC_SRC_STATE_INIT_RATE;
  tsi->last_change_ = 0;
  tsi->round_begin_ = now; // not 0, otherwise "no-feedback reduction" 
                           // in first round
  tsi->maxrate_ = 0;
  tsi->ndatapack_ = 0;
  tsi->round_id = 0;
  tsi->expected_rate = 0 ;
  tsi->inter_packet = 0;
  tsi->max_rtt_ = TFMCC_SRC_INIT_RTT;
  tsi->supp_rate_ = DBL_MAX;

  tsi->rtt_recv_id = -1;
  tsi->rtt_prio = 0;
  tsi->rtt_recv_timestamp = 0;
  tsi->rtt_recv_last_feedback = 0;
  tsi->rtt_rate = 0;

  // CLR
  tsi->clr_id = -1;
  tsi->clr_timestamp = 0;
  tsi->clr_last_feedback = 0;

#ifdef TRACE_TFMCC_THROUGHPUT
  tsi->thruSampleTime_ = tw_now (tw_getlp (agent->nodeId_));
  tsi->totalSentBytes_ = 0;
#endif
  
  // send the first packet
  TfmccSrcSendPkt (agent);
  // ... at initial rate
  tsi->sendPktTimerEvnt_ = ScheduleTimer 
    (agent, TFMCC_SRC_TIMER_SEND_PKT, tsi->pktSize_ / tsi->rate_);
  // ... and start timer so we can cut rate 
  // in half if we do not get feedback
  tsi->noFdbkTimerEvnt_ = ScheduleTimer
    (agent, TFMCC_SRC_TIMER_NO_FDBK, 
     TFMCC_SRC_DEC_NO_REPORT * tsi->pktSize_/ tsi->rate_);
}


void TfmccSrcStop (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  
  tsi->active_ = 0;
  
  CancelTimer (agent, & (tsi->sendPktTimerEvnt_));
  tsi->sendPktTimerEvnt_ = NULL;
  CancelTimer (agent, & (tsi->noFdbkTimerEvnt_));
  tsi->noFdbkTimerEvnt_ = NULL;
}


void TfmccSrcPktHandler (Agent * agent, Packet * p)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  TfmccNak * nck = (TfmccNak *) & (p->type.tn_);  
  int cur_prio;
  int new_clr = 0;
  double adjusted_rate, tmp_rtt;
    
  if (! tsi->active_) return;

  if (p->type_ != TFMCC_NAK) {
    printf ("TFMCC source (%d.%lu) received non-nak packet.\n",
       lp->id, agent->id_);
    tw_exit (-1);
  }
  
  if (tsi->rate_change_ == TFMCC_SRC_STATE_INIT_RATE) {
    tsi->rate_change_ = TFMCC_SRC_STATE_SLOW_START;
  }

  // if we don't get ANY feedback for some time, cut rate in half
  ReschedTimer (agent, TFMCC_SRC_TIMER_NO_FDBK, 
    TFMCC_SRC_DEC_NO_REPORT * tsi->max_rtt_, & (tsi->noFdbkTimerEvnt_));
      
  // compute receiver's current RTT
  tmp_rtt = now - nck->timestamp_echo - nck->timestamp_offset;
  if (tsi->max_rtt_ < tmp_rtt) tsi->max_rtt_ = tmp_rtt;

  adjusted_rate = nck->rate;
  if (nck->have_rtt == 0 && tsi->rate_change_ == TFMCC_SRC_STATE_CONG_AVOID) {
    // recalculate rate with correct RTT
    adjusted_rate = adjusted_rate / tmp_rtt * tsi->max_rtt_;
  }

  if (adjusted_rate < TFMCC_SRC_MIN_RATE) adjusted_rate = TFMCC_SRC_MIN_RATE;

  // adjust suppression rate by G_FACTOR if non-CLR (and correct feedback round)
  // before clr_id is updated so that new CLR also contributes to suppression
  if ((tsi->clr_id != p->srcAddr_)
      && (tsi->supp_rate_ > (1 - TFMCC_SRC_G_FACTOR) * nck->rate)
      && (nck->round_id == tsi->round_id)) {
    tsi->supp_rate_ = (1 - TFMCC_SRC_G_FACTOR) * nck->rate;
  }

  // if we are in slow start and we just saw a loss then come out of slow start
  if (tsi->rate_change_ == TFMCC_SRC_STATE_SLOW_START && nck->have_loss) {
    tsi->rate_change_ = TFMCC_SRC_STATE_CONG_AVOID;
    tsi->oldrate_ = tsi->rate_ = adjusted_rate;
    tsi->clr_id = p->srcAddr_;
    new_clr = 1;
  }

  if (tsi->rate_change_ == TFMCC_SRC_STATE_SLOW_START) {
    // adjust maxrate
    if (tsi->clr_id == p->srcAddr_) {
      tsi->maxrate_ = adjusted_rate;
    } else if ((nck->receiver_leave == 0) 
                && (adjusted_rate < tsi->maxrate_ || tsi->clr_id == -1)) {
      // change CLR
      tsi->maxrate_ = adjusted_rate;
      tsi->clr_id = p->srcAddr_;
      new_clr = 1;
    }
    TfmccSrcSlowStart (agent);
  } else if (tsi->rate_change_ == TFMCC_SRC_STATE_CONG_AVOID) {
    if ((nck->receiver_leave == 0) 
         && (adjusted_rate < tsi->rate_ || tsi->clr_id == -1)) {
      // change CLR
      tsi->clr_id = p->srcAddr_;
      new_clr = 1;
    }
    if (tsi->clr_id == p->srcAddr_) {
      if (adjusted_rate >= 0) {
        tsi->expected_rate = adjusted_rate;
        if (tsi->expected_rate > tsi->rate_) 
          TfmccSrcIncrRate (agent);
        else
          TfmccSrcDecrRate (agent);
      }
    }
  }

  // determine which timestamp to include in data packet
  if (tsi->clr_id == p->srcAddr_) {
    // remember CLR timestamp to include in packets w/o any other RTT feedback
    tsi->clr_timestamp = nck->timestamp;
    tsi->clr_last_feedback = now;
    if (! nck->have_rtt || new_clr)
      cur_prio = TFMCC_SRC_PRIO_FORCE;
    else
      cur_prio = TFMCC_SRC_PRIO_CLR;
  } else {
    if (! nck->have_rtt)
      cur_prio = TFMCC_SRC_PRIO_NO_RTT;
    else
      cur_prio = TFMCC_SRC_PRIO_RECV;
  }
  if (cur_prio > tsi->rtt_prio 
      || (cur_prio == tsi->rtt_prio && nck->rate < tsi->rtt_rate)) {
    tsi->rtt_prio = cur_prio;
    tsi->rtt_recv_id = p->srcAddr_;
    tsi->rtt_recv_timestamp = nck->timestamp;
    tsi->rtt_recv_last_feedback = now;
    tsi->rtt_rate = nck->rate; // use reported rate for suppression
  }
}


void TfmccSrcSlowStart (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);

  if (tsi->rate_ + TFMCC_SMALLFLOAT < tsi->pktSize_ / tsi->max_rtt_ ) {
    /* if this is the first report, change rate to 1 per rtt */
    /* compute delta so rate increases slowly to new value   */
    tsi->oldrate_ = tsi->rate_;
    tsi->rate_ = tsi->fairsize_ / tsi->max_rtt_;
    tsi->delta_ = (tsi->rate_ - tsi->oldrate_) 
                  / (tsi->rate_ * tsi->max_rtt_ / tsi->pktSize_);
    tsi->last_change_ = now;
  } else {
    /* else multiply the rate by ssmult_, and compute delta, */
    /*  so that the rate increases slowly to new value       */
    if (tsi->maxrate_ > 0) {
      if (tsi->ssmult_ * tsi->rate_ < tsi->maxrate_ 
          && now - tsi->last_change_ > tsi->max_rtt_) {
        tsi->rate_ = tsi->ssmult_ * tsi->rate_; 
        tsi->delta_ = (tsi->rate_ - tsi->oldrate_)
                      / (tsi->rate_ * tsi->max_rtt_ / tsi->pktSize_);
        tsi->last_change_ = now;
      } else {
        if (tsi->rate_ > tsi->maxrate_ 
            || now - tsi->last_change_ > tsi->max_rtt_) {
          tsi->rate_ = tsi->maxrate_; 
          tsi->delta_ = (tsi->maxrate_ - tsi->oldrate_)
                        / (tsi->rate_ * tsi->max_rtt_ / tsi->pktSize_);
          if (tsi->delta_ < 0) tsi->delta_ = 0;
          tsi->last_change_ = now; 
        }
      }
    } else {
      if (now - tsi->last_change_ > tsi->max_rtt_) {
        tsi->rate_ = tsi->ssmult_ * tsi->rate_; 
        tsi->delta_ = (tsi->rate_ - tsi->oldrate_)
                      / (tsi->rate_ * tsi->max_rtt_ / tsi->pktSize_);
        tsi->last_change_ = now;
      }
    }
  }
}


void TfmccSrcIncrRate (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp), next;
  double mult = (now - tsi->last_change_) / tsi->max_rtt_;
  if (mult > 2) mult = 2;
  
  tsi->rate_ = tsi->rate_ + (tsi->fairsize_ / tsi->max_rtt_) * mult;
  if (tsi->rate_ > tsi->expected_rate) tsi->rate_ = tsi->expected_rate;

  if (tsi->rate_ < tsi->fairsize_ / tsi->max_rtt_) {
    tsi->rate_ = tsi->fairsize_ / tsi->max_rtt_;
  }

  tsi->last_change_ = now;

  // check if rate increase should impact send timer
  next = tsi->pktSize_ / tsi->rate_;
  if (tsi->inter_packet >= 2 * next) {
    assert(next > TFMCC_SMALLFLOAT);
    ReschedTimer (agent, TFMCC_SRC_TIMER_SEND_PKT, next, 
      & (tsi->sendPktTimerEvnt_));
  }
}


void TfmccSrcDecrRate (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);

  tsi->oldrate_ = tsi->rate_ = tsi->expected_rate;
  tsi->last_change_ = now;
}


void TfmccSrcSendPkt (Agent * agent)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  Packet p;
  TfmccData * data = (TfmccData *) & (p.type.td_);
  
  if (! tsi->active_) return;

  p.type_ = TFMCC_DATA;  
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = tsi->pktDstAddr_;
  p.dstAgent_ = tsi->pktDstAgentId_;
  p.size_ = tsi->pktSize_;
  p.inPort_ = (uint16) -1;
  
  data->seqno = tsi->seqno_++;
  data->timestamp = now;
  data->supp_rate = tsi->supp_rate_;
  data->round_id = tsi->round_id;
  data->max_rtt = tsi->max_rtt_;

  if (tsi->clr_id == tsi->rtt_recv_id || tsi->rtt_recv_id == -1) {
    data->rtt_recv_id = tsi->clr_id;
    data->timestamp_echo =  tsi->clr_timestamp;
    data->timestamp_offset = now - tsi->clr_last_feedback;
    data->is_clr = 1;
  } else {
    data->rtt_recv_id = tsi->rtt_recv_id;
    data->timestamp_echo = tsi->rtt_recv_timestamp;
    data->timestamp_offset = now - tsi->rtt_recv_last_feedback;
    data->is_clr = 0;
  }

  SendPacket (lp, &p);

  tsi->ndatapack_++;
  tsi->rtt_recv_id = -1;
  tsi->rtt_prio = 0;

#ifdef TRACE_TFMCC_THROUGHPUT
  if (! tsi->trackStat_) return;

  tsi->totalSentBytes_ += p.size_;
  if (now - tsi->thruSampleTime_ < TFMCC_THROUGHPUT_SAMPLE_ITVL) {  
    return;
  }
   
  /* For rate tracing */
  printf ("%lf : at %d.%ld , throughput rate = %lf Mbps, itvl = %lf\n",
    now, lp->id, agent->id_, 
    tsi->totalSentBytes_ / (now - tsi->thruSampleTime_) / 125000,
    now - tsi->thruSampleTime_);
  tsi->thruSampleTime_ = now;
  tsi->totalSentBytes_ = 0;
#endif
}


void TfmccSrcTimer (Agent * agent, int timerId, int serial)
{
  TfmccSrcInfo * tsi = (TfmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp), xrate, max_variation, next = 0;
  
  if (! tsi->active_) return;
  
  switch (timerId) {
  case TFMCC_SRC_TIMER_SEND_PKT:
    xrate = -1;
    tsi->sendPktTimerEvnt_ = NULL;
  
    // new feedback round every T = t_factor * max_RTT, 
    //   if at least one non-CLR FBM was received (supp_rate_ < DBL_MAX)
    //   or after T = 2 * t_factor * max_RTT (only one receiver in group)
    if ((now - tsi->round_begin_ >= tsi->t_factor_ * tsi->max_rtt_
          && tsi->supp_rate_ < DBL_MAX)
        || (now - tsi->round_begin_ >= 2 * tsi->t_factor_ * tsi->max_rtt_)) {
      tsi->round_id = (tsi->round_id + 1) % TFMCC_MAX_ROUND_ID;
      tsi->round_begin_ = now;
      tsi->supp_rate_ = DBL_MAX;
      // XXX draft says only inc when max_rtt unchanged from prev round
      tsi->max_rtt_ *= TFMCC_SRC_MAX_RTT_DECAY;
      if (tsi->max_rtt_ < tsi->pktSize_ / tsi->rate_) {
        tsi->max_rtt_ = tsi->pktSize_ / tsi->rate_;
      }
    }
  
    // time out CLR
    if (now - tsi->clr_last_feedback
        > tsi->max_rtt_ * TFMCC_SRC_DEC_NO_REPORT) {
      tsi->clr_id = -1;
    }
  
    TfmccSrcSendPkt (agent);
    
    // during slow start, we increase rate
    // slowly - by amount delta per packet 
    if ((tsi->rate_change_ == TFMCC_SRC_STATE_SLOW_START)
        && (tsi->oldrate_ + TFMCC_SMALLFLOAT < tsi->rate_)) {
      tsi->oldrate_ = tsi->oldrate_ + tsi->delta_;
      xrate = tsi->oldrate_;
    } else {
      xrate = tsi->rate_;
    }
  
    if (xrate > TFMCC_SMALLFLOAT) {
      tsi->inter_packet = tsi->pktSize_ / xrate;
  
      // randomize between +-50% packet interval, but no more than max_rtt gap
      max_variation = Max (0, Min (0.5 * tsi->inter_packet, 
                                    tsi->max_rtt_ - tsi->inter_packet));
                                    
      next = tsi->inter_packet - max_variation
             + 2 * max_variation * (rand () / ((double) RAND_MAX));
      assert (next > TFMCC_SMALLFLOAT);
      tsi->sendPktTimerEvnt_ = ScheduleTimer 
        (agent, TFMCC_SRC_TIMER_SEND_PKT, next);
    }
    break;
  case TFMCC_SRC_TIMER_NO_FDBK:
    tsi->rate_ *= 0.5;
    next = tsi->pktSize_ / tsi->rate_;
    assert (next > TFMCC_SMALLFLOAT);
    ReschedTimer 
      (agent, TFMCC_SRC_TIMER_SEND_PKT, next, & (tsi->sendPktTimerEvnt_));
    tsi->noFdbkTimerEvnt_ = ScheduleTimer
      (agent, TFMCC_SRC_TIMER_NO_FDBK, TFMCC_SRC_DEC_NO_REPORT * tsi->max_rtt_);
    break;
  }
}


void TfmccSrcPrintStat (Agent * agent)
{}
