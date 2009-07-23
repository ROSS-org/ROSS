#include "pgmcc.h"

uint32 _totalPgmccAck = 0, _totalPgmccNak = 0;

/***********************************
 P G M C C  S O U R C E
 ***********************************/

Agent * PgmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId, int8 trackStat)
{
  Agent * a = GetAgent (nodeId, (uint32) agentId);
  PgmccSrcInfo * psi;
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, _nodeTable[nodeId].addr_, 
                     (PktHandler) PgmccSrcPktHandler, 
                     (StartFunc) PgmccSrcStart,
                     (StopFunc) PgmccSrcStop,
                     (TimerFunc) PgmccSrcTimer,
                     (AgentFunc) PgmccSrcPrintStat,
                     NULL);
  psi = malloc (sizeof (PgmccSrcInfo));
  
  if (psi == NULL) {
    printf ("Failed to allocate memory for PGMCC source info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) psi, 0, sizeof (PgmccSrcInfo));
  psi->pktDstAddr_ = pktDstAddr;
  psi->pktDstAgentId_ = pktDstAgentId;
  a->info_ = psi;
  psi->trackStat_ = trackStat;

  return a;
}


void PgmccSrcStart (Agent * agent)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;

  psi->pktSize_ = PGMCC_SRC_DATA_PKT_SIZE;
  psi->active_ = 1;
  psi->txwLead_ = (uint32) -1;

  psi->ackerAddr_ = (uint32) -1;
  psi->window_ = 1;
  psi->token_ = 1;
  psi->dupAcks_ = 0;
  psi->ackLead_ = 0;
  psi->ignoreCong_ = 0;
  psi->ackBitMask_ = ~0;
  psi->ackerMrtt_ = 0;
  psi->ackerLoss_ = 0;

  psi->ccTimerEvnt_
    = ScheduleTimer (agent, PGMCC_SRC_TIMER_CC, PGMCC_SRC_CC_TIMEOUT);

#ifdef TRACE_PGMCC_THROUGHPUT
  psi->thruSampleTime_ = tw_now (tw_getlp (agent->nodeId_));
  psi->totalSentBytes_ = 0;
#endif

  PgmccSrcSendPkt (agent);
}


void PgmccSrcStop (Agent * agent)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;

  psi->active_ = 0;
  CancelTimer (agent, & (psi->ccTimerEvnt_));
}


void PgmccSrcSendPkt (Agent * agent)
{  
  Packet p;
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  int sent = 0;
  
  if (! psi->active_) return;
  
  while (psi->token_ >= 1) {
    p.type_ = PGMCC_DATA;
    p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
    p.srcAgent_ = agent->id_;
    p.dstAddr_ = psi->pktDstAddr_;
    p.dstAgent_ = psi->pktDstAgentId_;
    sent += (p.size_ = psi->pktSize_);
    p.inPort_ = (uint16) -1;
    p.type.pd_.sqn_ = ++ psi->txwLead_;
    p.type.pd_.ackerAddr_ = psi->ackerAddr_;

    -- psi->token_;
    
    SendPacket (lp, &p);
  }

#ifdef TRACE_PGMCC_THROUGHPUT
  if (! psi->trackStat_) return;

  psi->totalSentBytes_ += sent;
  if (now - psi->thruSampleTime_ < PGMCC_THROUGHPUT_SAMPLE_ITVL) {
    return;
  }
   
   /* For rate tracing */
  printf ("%lf : at %d.%ld , throughput rate = %lf Mbps, itvl = %lf\n",
    now, lp->id, agent->id_, 
    psi->totalSentBytes_ / (now - psi->thruSampleTime_) / 125000,
    now - psi->thruSampleTime_);
  psi->thruSampleTime_ = now;
  psi->totalSentBytes_ = 0;
#endif
}


void PgmccSrcTimer (Agent * agent, int timerId, int serial)
{
  switch (timerId) {
  case PGMCC_SRC_TIMER_CC:
    PgmccSrcStall (agent);
    break;
  default:
    printf ("Unknow PGMCC source timer.\n");
    tw_exit (-1);
  }
}


void PgmccSrcPktHandler (Agent * agent, Packet * p)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;

  if (! psi->active_) return;
  
  switch (p->type_) {
  case PGMCC_ACK:
    PgmccSrcAckProc (agent, p);
    break;
  case PGMCC_NAK:
    PgmccSrcNakProc (agent, p);
    break;
  default:
    printf ("Unknown PGMCC feedback type <%d>.\n", p->type_);
    tw_exit (-1);
  }
}


void PgmccSrcAckProc (Agent * agent, Packet * p)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;
  PgmccAck * ack = (PgmccAck *) & (p->type.pa_);
  double deltaW;
  uint32 lead, bitmask;
  int delta;
  uint32 newAcks;
  int total, missing;
  uint32 l; 

  ReschedTimer (agent, PGMCC_SRC_TIMER_CC, PGMCC_SRC_CC_TIMEOUT, 
    & (psi->ccTimerEvnt_));

  lead = ack->rxwLead_;
  bitmask = ack->bitmask_;
  
  if (p->srcAddr_ == psi->ackerAddr_) {
    psi->ackerMrtt_ = psi->txwLead_ - lead;
    psi->ackerLoss_ = psi->ackerMrtt_ * psi->ackerMrtt_ * ack->rxLoss_;
  }

  /*
    * using bitmasks, determine new acks carried by this pkt.
    * ack_bitmask_ is the old state, bitmask is the one in the ack.
    * This section is necessary because we dont have cumulative acks
    */
  delta = (int) (lead - psi->ackLead_);
  if (delta > 32) // very new, reset old state 
     psi->ackBitMask_ = 0;
  else if (delta > 0)
     psi->ackBitMask_ <<= delta;
  else if (delta < -32)
     bitmask = 0;
  else
     bitmask <<= -delta;

  // compute the new acks 
  newAcks = bitmask & ~ (psi->ackBitMask_);
  // and update state 
  psi->ackBitMask_ |= bitmask ;
  if (lead > psi->ackLead_) psi->ackLead_ = lead;

  // compute total acks in this packet
  for (total = 0, l = newAcks; l; l >>= 1) {
    if (l & 1) ++ total;
  }

  // count missing packets 
  for (missing = 0 , l = ~ (psi->ackBitMask_); l; l >>= 1) {
    if (l & 1) ++ missing;
  }

  if (total == 0) {
    // fully duplicate acks 
    return ;
  }

  // Do not count dups for ACKs within an rtt from prev. cong.
  if (lead <= psi->ignoreCong_) {
    missing = 0 ;
    psi->ackBitMask_ = ~0;
  }
 
  if (missing == 0) {
    total += psi->dupAcks_ ; // recover previous acks 
    psi->dupAcks_ = 0 ;
    if (psi->token_ < 0)
        deltaW = 0 ;         // do not increase window
    else if (psi->window_ < PGMCC_SRC_SS_THRESH)
        deltaW = 1 ;         // exp. increase for small windows 
    else
        deltaW = 1. / psi->window_ ;
    psi->token_ += total * (1 + deltaW) ;
    psi->window_ += total * deltaW ;  
  } else {
    psi->dupAcks_ += total ;
    if (psi->dupAcks_ >= PGMCC_SRC_DUP_THRESH) {
      /*
       * re-sync window estimate with reality.
       * Since we know the real window, we also know how
       * many tokens will arrive (if no loss), and how
       * many we should ignore.
       */
      psi->window_ = (psi->txwLead_ - lead + 1 );
      // mimic TCP slowstart 
      psi->ignoreCong_ = psi->txwLead_;
      if (psi->token_ < 0)
        ++ psi->token_;
      else if (psi->window_ < 4 )  // small window
        psi->token_ = 1 ;
      else {
        psi->window_ /= 2 ;
        psi->token_ = - psi->window_ + 1;
      } 
      psi->ackBitMask_ = ~0 ; // not count anymore these losses 
      psi->dupAcks_ = 0 ;
    } else if (psi->txwLead_ == lead ) 
    /* 
     * XXX if there are no flying-packets we forcely send a new 
     *      one to avoid a stall
     */
   psi->token_ = 1; 
 }  

  PgmccSrcSendPkt (agent);
}


void PgmccSrcNakProc (Agent * agent, Packet * p)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  PgmccNak * nak = (PgmccNak *) & (p->type.pn_);
  double now = tw_now (lp);
  double nackerLoss;
  double rttn = psi->txwLead_ - nak->rxwLead_;  
  
  rttn -= 1 + (PGMCC_SRC_SS_THRESH  >= (int) (psi->window_)) ;
  if (rttn < 0) rttn = 0 ;
  
  nackerLoss = rttn * rttn * nak->rxLoss_; 

  if (psi->ackerAddr_ == (uint32) -1) {
    printf ("%lf : at %d.%ld acker is set as %d\n",
      now, lp->id, agent->id_, _addrToIdx[p->srcAddr_]);

    psi->ackerAddr_ = p->srcAddr_;
    psi->ackerMrtt_ = rttn;
    ++ psi->token_;
    psi->ackLead_ = psi->txwLead_;
    psi->ackBitMask_ = ~0;
    PgmccSrcSendPkt (agent);
  } else if (psi->ackerAddr_ != p->srcAddr_ 
             &&  nackerLoss > psi->ackerLoss_ * PGMCC_SRC_HYST) {

    printf ("%lf : at %d.%ld acker is changed from %d to %d\n",
      now, lp->id, agent->id_, _addrToIdx[psi->ackerAddr_],
      _addrToIdx[p->srcAddr_]);

    psi->ackerAddr_ = p->srcAddr_;
    psi->ackerMrtt_ = rttn;
  }

  if (psi->ackerAddr_ == p->srcAddr_) { 
    psi->ackerLoss_ = psi->ackerMrtt_ * psi->ackerMrtt_ * nak->rxLoss_;
  }
}


void PgmccSrcStall (Agent * agent)
{
  PgmccSrcInfo * psi = (PgmccSrcInfo *) agent->info_;

  psi->ignoreCong_ = psi->txwLead_;
  psi->ackLead_ = psi->txwLead_;
  psi->token_ = 1;
  psi->window_ = 1;
  psi->dupAcks_ = 0;
  psi->ackBitMask_ = ~0;

  psi->ccTimerEvnt_ = ScheduleTimer 
    (agent, PGMCC_SRC_TIMER_CC, PGMCC_SRC_CC_TIMEOUT);
  PgmccSrcSendPkt (agent);
}


void PgmccSrcPrintStat (Agent * agent)
{}


/***********************************
 P G M C C  R E C E I V E R
 ***********************************/


Agent * PgmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr)
{
  Agent * a;
  PgmccRcvInfo * pri;
  
  if (IsMcastAddr (listenAddr)) {
    a = GetAgent (nodeId, listenAddr);
  }
  else {
    a = GetAgent (nodeId, (uint32) agentId);
  }
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, listenAddr, 
                     (PktHandler) PgmccRcvPktHandler, 
                     (StartFunc) PgmccRcvStart,
                     (StopFunc) PgmccRcvStop,
                     (TimerFunc) PgmccRcvTimer,
                     (AgentFunc) PgmccRcvPrintStat,
                     NULL);

  pri = malloc (sizeof (PgmccRcvInfo));
  
  if (pri == NULL) {
    printf ("Failed to allocate memory for PGMCC receiver info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) pri, 0, sizeof (PgmccRcvInfo));
  a->info_ = pri;
             
  return a;                             
}


void PgmccRcvStart (Agent * agent)
{
  PgmccRcvInfo * pri = (PgmccRcvInfo *) agent->info_;

  pri->active_ = 1;
  pri->rxwLead_ = (uint32) -1;
  pri->ackBitMask_ = 0;
  pri->rxLoss_ = 0;
  pri->rxDoAck_ = 0;
}


void PgmccRcvStop (Agent * agent)
{
  PgmccRcvInfo * pri = (PgmccRcvInfo *) agent->info_;

  pri->active_ = 0;
}


void PgmccRcvPktHandler (Agent * agent, Packet * p)
{
  PgmccRcvInfo * pri = (PgmccRcvInfo *) agent->info_;
  PgmccData * data = (PgmccData *) &(p->type.pd_);
  tw_lp * lp = tw_getlp (agent->nodeId_);
  uint32 i, sqn;
  int delta;

  if (! pri->active_) return;

  if (p->type_ != PGMCC_DATA) {
    printf ("PGMCC receiver (%d.%lu) received non-data packet.\n",
       lp->id, agent->id_);
    tw_exit (-1);
  }

  if (pri->rxwLead_ == (uint32) -1) pri->rxwLead_ = data->sqn_;
  
  if (data->ackerAddr_ != (uint32) -1) {
    pri->rxDoAck_ = (data->ackerAddr_ == _nodeTable[lp->id].addr_);
  }
  else {
    pri->rxDoAck_ = 0;
    PgmccRcvSendPkt (agent, p, PGMCC_NAK);
  }    
    
  // update the bitmask 
  sqn = data->sqn_;
  delta = (int) (sqn - pri->rxwLead_);
  if (sqn > pri->rxwLead_) {
    if (delta > 31)
      pri->ackBitMask_ = 0 ;
    else
      pri->ackBitMask_ <<= delta;
    pri->ackBitMask_ |= 1 ;
  } 
  else { /* old, out of sequence ? */
    delta = -delta ;
    if (delta < 31) pri->ackBitMask_ |= (1 << delta) ;
  }
 
  // update rx_loss_, assuming no out-of-order packets 
  if (sqn > pri->rxwLead_) {
    pri->rxLoss_ *= pow (PGMCC_RCV_WEIGHT, sqn - pri->rxwLead_);

    for (i = sqn - 1 ; i > pri->rxwLead_; -- i) {
      pri->rxLoss_ += pow (PGMCC_RCV_WEIGHT, i - pri->rxwLead_)
         * (1. - PGMCC_RCV_WEIGHT); 
    }
  }

  if (sqn > pri->rxwLead_ + 1) PgmccRcvSendPkt (agent, p, PGMCC_NAK);  
  if (sqn > pri->rxwLead_) pri->rxwLead_ = sqn;
  if (pri->rxDoAck_) PgmccRcvSendPkt (agent, p, PGMCC_ACK);
}


void PgmccRcvSendPkt (Agent * agent, Packet * srcPkt, int16 type)
{
  Packet p;
  PgmccRcvInfo * pri = (PgmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  PgmccAck * ack;
  PgmccNak * nak;
  
  if (! pri->active_) return;
  
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = srcPkt->srcAddr_;
  p.dstAgent_ = srcPkt->srcAgent_;  
  p.size_ = PGMCC_RCV_FDBK_PKT_SIZE;
  p.inPort_ = (uint16) -1;
  p.type_ = type;

  switch (type) {
  case PGMCC_ACK:
    ack = (PgmccAck *) &(p.type.pa_);
    ack->bitmask_ = pri->ackBitMask_;
    ack->rxLoss_ = pri->rxLoss_;
    ack->rxwLead_ = pri->rxwLead_;
    
    ++ pri->ackSent_;

    if (IsMcastAddr (agent->listenAddr_)) ++ _totalPgmccAck;
    break;
  case PGMCC_NAK:
    nak = (PgmccNak *) &(p.type.pn_);
    nak->rxLoss_ = pri->rxLoss_;
    nak->rxwLead_ = pri->rxwLead_;

    ++ pri->nakSent_;
    
    if (IsMcastAddr (agent->listenAddr_)) ++ _totalPgmccNak;
    break;
  default:
    printf ("Unknown PGMCC packet type to send. %d\n", type);
    tw_exit (-1);    
  }
  
  SendPacket (lp, &p);
}


void PgmccRcvTimer (Agent * agent, int timerId, int serial)
{}


void PgmccRcvPrintStat (Agent * agent)
{
  PgmccRcvInfo * pri = (PgmccRcvInfo *) agent->info_;
  
  printf 
    ("PGMCC receiver at %d.%lu statistics: ACK sent = %lu, NAK sent = %lu, ",
    agent->nodeId_, GetGrpAddr4Print (agent->id_), pri->ackSent_,
    pri->nakSent_);
    
  printf ("total ACK sent = %lu, total NAK sent = %lu\n",
    _totalPgmccAck, _totalPgmccNak);
}
