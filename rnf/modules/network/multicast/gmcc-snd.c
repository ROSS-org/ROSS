#include "gmcc-snd.h"
#include "gmcc-util.h"


Agent * GmccSrcInit (int nodeId, uint16 agentId, 
                      uint32 pktDstAddr, uint16 pktDstAgentId)
{
  Agent * a = GetAgent (nodeId, (uint32) agentId);
  GmccSrcInfo * gsi;
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, _nodeTable[nodeId].addr_, 
                     (PktHandler) GmccSrcPktHandler, 
                     (StartFunc) GmccSrcStart,
                     (StopFunc) GmccSrcStop,
                     (TimerFunc) GmccSrcTimer,
                     (AgentFunc) GmccSrcPrintStat,
                     NULL);
  gsi = malloc (sizeof (GmccSrcInfo));
  
  if (gsi == NULL) {
    printf ("Failed to allocate memory for ORMCC source info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) gsi, 0, sizeof (GmccSrcInfo));
  gsi->pktDstAddr_ = pktDstAddr;
  gsi->pktDstAgentId_ = pktDstAgentId;
  gsi->layerNum_ = GMCC_SRC_LAYER_NUM;
  a->info_ = gsi;

  return a;
}


void GmccSrcInitLayer (Agent * agent, int i)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;

  gsi->activeLayer_[i] = 0;
  gsi->sqn_[i] = 0;
  gsi->rate_[i] = 2 * GMCC_DATA_PKT_SIZE / gsi->grttAvg_; /* 2 pkts per RTT */
  gsi->rttAvg_[i] = gsi->grttAvg_;
  gsi->rttDev_[i] = 0;  /* Set to -1 for abrupt change at the receipt of f
                         * irst sample, otherwise 0 */
  gsi->crTafAvg_[i] = -1;
  gsi->crTafDev_[i] = 0;
  gsi->cr_[i] = (uint32) -1;
  gsi->debtNextLayer_[i] = 0;

  RateAvgrInit (gsi->sndRateAvg_ + i, 2);
}


void GmccSrcResetLayer (Agent * agent, int i)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
    
  RateAvgrReset (gsi->sndRateAvg_ + i);
  tw_timer_cancel (lp, gsi->rateIncrTimerEvnt_ + i);
  tw_timer_cancel (lp, gsi->sendPktTimerEvnt_ + i);
  tw_timer_cancel (lp, gsi->congEpochTimerEvnt_ + i);
  tw_timer_cancel (lp, gsi->crCheckTimerEvnt_ + i);
  tw_timer_cancel (lp, gsi->crExtSearchTimerEvnt_ + i);
  tw_timer_cancel (lp, gsi->keepaliveTimerEvnt_ + i);  
}


void GmccSrcStart (Agent * agent)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  double now = tw_now (tw_getlp (agent->nodeId_));
  int i;

  gsi->active_ = 1;
  
  memset ((void *) gsi->rateIncrTimerEvnt_, 0, 
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);
  memset ((void *) gsi->sendPktTimerEvnt_, 0, 
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);
  memset ((void *) gsi->congEpochTimerEvnt_, 0, 
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);
  memset ((void *) gsi->crCheckTimerEvnt_, 0,
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);
  memset ((void *) gsi->crExtSearchTimerEvnt_, 0, 
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);
  memset ((void *) gsi->keepaliveTimerEvnt_, 0,
          sizeof (tw_event *) * GMCC_MAX_LAYER_NUM);

  /* Allocate multicast address for layers */
  gsi->grpAddrs_[0] = gsi->pktDstAddr_; /* The basic layer is assigned before
                                         * starting */
  for (i = 1; i < gsi->layerNum_; ++ i) { 
    AddGroup (gsi->grpAddrs_[i] = AllocGroupAddr (), agent->nodeId_);
  }

  gsi->ctrlPktId_ = 1;
  StatEstimatorInit (& (gsi->grttEst_), 30, 0.125);
  gsi->grttAvg_ = GMCC_SRC_INIT_RTT;
  gsi->grttDev_ = 0;
  for (i = 0; i < gsi->layerNum_; ++ i) {
    GmccSrcInitLayer (agent, i);
  }
  gsi->activeLayer_[0] = 1; /* The base layer is always active */
  gsi->topActLayer_ = 0;
  gsi->oversendLayer_ = -1;
   
  gsi->lastCtrlPktTime_ = now;

  GmccSrcSendCtrlPkt (agent, -1);
  GmccSrcSendDataPkt (agent, 0);

  gsi->sendPktTimerEvnt_[0] = ScheduleTimerWithSn
    (agent, GMCC_SRC_SEND_PKT_TIMER, 0, GMCC_DATA_PKT_SIZE / gsi->rate_[0]);
  gsi->rateIncrTimerEvnt_[0] = ScheduleTimerWithSn
    (agent, GMCC_SRC_RATE_INCR_TIMER, 0, gsi->rttAvg_[0]);
}


void GmccSrcStop (Agent * agent)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  int i;
  
  gsi->active_ = 0;
  
  for (i = 0; i < gsi->layerNum_; ++ i) {
    tw_timer_cancel (lp, gsi->rateIncrTimerEvnt_ + i);
    tw_timer_cancel (lp, gsi->sendPktTimerEvnt_ + i);
    tw_timer_cancel (lp, gsi->congEpochTimerEvnt_ + i);
    tw_timer_cancel (lp, gsi->crCheckTimerEvnt_ + i);
    tw_timer_cancel (lp, gsi->crExtSearchTimerEvnt_ + i);
    tw_timer_cancel (lp, gsi->keepaliveTimerEvnt_ + i);  
  }  
}


void GmccSrcSendCtrlPkt (Agent * agent, int layer)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  Packet p;
  GmccCtrlExt gce;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  int i;
 
  if (! gsi->active_) return;
  
  p.type_ = GMCC_CTRL;
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = gsi->grpAddrs_[0];
  p.dstAgent_ = 0;
  p.size_ = GMCC_CTRL_PKT_SIZE;
  p.inPort_ = (uint16) -1;
  p.id_ = ++ gsi->ctrlPktId_;

  for (i = 0; i < gsi->layerNum_; ++ i) {
    if (gsi->activeLayer_[i]) {
      gce.crTafAvg_[i] = gsi->crTafAvg_[i];
      gce.crTafDev_[i] = gsi->crTafDev_[i];
      gce.srtt_[i] = gsi->rttAvg_[i] 
                     + (gsi->rttDev_[i] < 0 ? 0 : GMCC_SRC_RATE_INCR_RTT_FACTOR)
                       * gsi->rttDev_[i];
    }
    else {
      gce.crTafAvg_[i] = 0;
      gce.crTafDev_[i] = 0;
      gce.srtt_[i] = -1;
    }
    gce.activeLayer_[i] = gsi->activeLayer_[i];
    gce.grpAddrs_[i] = gsi->grpAddrs_[i];
    gce.cr_[i] = gsi->cr_[i];
  }
  gce.layerNum_ = gsi->layerNum_;
  gce.timestamp_ = now;
  gce.grttAvg_ = gsi->grttAvg_;
  gce.grttDev_ = gsi->grttDev_;
  gce.layer_ = layer;
    
  SendExtPkt (lp, &p, (void *) &gce, sizeof (GmccCtrlExt));
}


void GmccSrcSendDataPkt (Agent * agent, int layer)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  Packet p;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  GmccData * data = & (p.type.gd_);
  
  if (! gsi->active_) return;

  /* Check whether there is the layer below ignoring congestion. 
   * If yes, does not send this packet to balance the total sending rate.
   * At any moment, there is at most one layer ignoring congestion.
   */
  if (layer > 0 && gsi->debtNextLayer_[layer - 1] >= 1) {
    gsi->debtNextLayer_[layer - 1] -= 1;
    return;
  }
  
  p.type_ = GMCC_DATA;
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = gsi->grpAddrs_[layer];
  p.dstAgent_ = 0;
  p.size_ = GMCC_DATA_PKT_SIZE;
  p.inPort_ = (uint16) -1;

  data->sqn_ = gsi->sqn_[layer] ++;
  data->rate_ = RateAvgrAddSample (gsi->sndRateAvg_ + layer, now, p.size_);
  
  data->timestamp_ = now;
  data->layer_ = layer;

  if (gsi->oversendLayer_ == layer) {
    data->oversend_ = 1;
    gsi->debtNextLayer_[layer] += GMCC_SRC_OVERSEND_FRAC;
  }

  SendPacket (lp, &p);
  
  if (layer == 0 && now - gsi->lastCtrlPktTime_ > gsi->rttAvg_[0]) {
    GmccSrcSendCtrlPkt (agent, -1);
    gsi->lastCtrlPktTime_ = now;
  }
}


void GmccSrcPktHandler (Agent * agent, Packet * p)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  double err, t, d;
  GmccCi * ci = & (p->type.gci_);
  int i, layer = ci->layer_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);

  if (! gsi->active_) return;
  
  /* Update Global RTT */
  StatEstimatorAddSample (& (gsi->grttEst_), t = now - ci->tsEcho_);
  gsi->grttAvg_ = gsi->grttEst_.mean_;
  gsi->grttDev_ = gsi->grttEst_.dev_;
  
  if (layer < 0 || layer >= gsi->layerNum_) {
    printf ("GMCC: Invalid layer number in CI packet.\n");
    tw_exit (-1);
  }

  if (! gsi->activeLayer_[layer]) { 
    /* Activate the layer */
    GmccSrcInitLayer (agent, layer);
    gsi->activeLayer_[layer] = 1;
    gsi->cr_[layer] = p->srcAddr_;
     
    GmccSrcSendDataPkt (agent, layer);
    GmccSrcSendCtrlPkt (agent, layer);
    
    gsi->crCheckTimerEvnt_[layer] = ScheduleTimerWithSn (agent,
      GMCC_SRC_CR_CHECK_TIMER, layer, 
      8 * (gsi->rttAvg_[layer] + 4 * gsi->rttDev_[layer]));
    gsi->sendPktTimerEvnt_[layer] = ScheduleTimerWithSn (agent,
      GMCC_SRC_SEND_PKT_TIMER, layer, GMCC_DATA_PKT_SIZE / gsi->rate_[layer]);
    gsi->rateIncrTimerEvnt_[layer] = ScheduleTimerWithSn (agent,
      GMCC_SRC_RATE_INCR_TIMER, layer, gsi->rttAvg_[layer]);

    /*
     * May need to restart rate increasing timer or even reactivate 
     * the layers below
     */
    if (layer > gsi->topActLayer_) gsi->topActLayer_ = layer;
    for (i = 0; i < gsi->topActLayer_; ++ i) {
      if (i == layer) continue;
      if (gsi->activeLayer_[i]) {
        if (gsi->crExtSearchTimerEvnt_[i] != NULL) {
          tw_timer_cancel (lp, gsi->crExtSearchTimerEvnt_ + i);
        }
        else if (gsi->keepaliveTimerEvnt_[i] != NULL) {
          tw_timer_cancel (lp, gsi->keepaliveTimerEvnt_ + i);
          GmccSrcSendDataPkt (agent, i);
          /* Restart rate increasing */
          ReschedTimerWithSn (agent, GMCC_SRC_RATE_INCR_TIMER, i, 
            gsi->rttAvg_[i] + (gsi->rttDev_[i] < 0 ? 0 : 2) * gsi->rttDev_[i],
            gsi->rateIncrTimerEvnt_ + i);
        }
        continue;
      }
      /* Reactivate */
      GmccSrcInitLayer (agent, i);
      gsi->activeLayer_[i] = 1;
      GmccSrcSendDataPkt (agent, i);
      GmccSrcSendCtrlPkt (agent, i);
      
      ReschedTimerWithSn (agent, GMCC_SRC_SEND_PKT_TIMER, i,
        GMCC_DATA_PKT_SIZE / gsi->rate_[i], gsi->sendPktTimerEvnt_ + i);
      ReschedTimerWithSn (agent, GMCC_SRC_RATE_INCR_TIMER, i,
        gsi->rttAvg_[i], gsi->rateIncrTimerEvnt_ + i);
    }

    return;
  }

  ReschedTimerWithSn (agent, GMCC_SRC_CR_CHECK_TIMER, layer,
    8 * (gsi->rttAvg_[layer] + 4 * gsi->rttDev_[layer]),
    gsi->crCheckTimerEvnt_ + layer);  

  /* 
   * Switch CR if necessary 
   */
  if (gsi->cr_[layer] == (uint32) -1) {
    gsi->cr_[layer] = p->srcAddr_;
  }
  else {
    /* Prevent a receiver from being CR for more than 1 layers */  
    for (i = 0; i < gsi->layerNum_; ++ i) {
      if (i == layer) continue;
      if (gsi->cr_[i] == p->srcAddr_) {
        return;
      }
    }  
  
    d = GMCC_SRC_CHANGE_CR_FACTOR
        * sqrt ((ci->tafDev_ * ci->tafDev_
                  + GMCC_SRC_CHANGE_CR_FACTOR2 * GMCC_SRC_CHANGE_CR_FACTOR2 *
                    gsi->crTafDev_[layer] * gsi->crTafDev_[layer]) 
                / GMCC_SRC_SAMPLE_SIZE);
  
    if (p->srcAddr_ != gsi->cr_[layer]
        && ci->mature_  /* Only a receiver with enough sample can preempt */
        && ci->tafAvg_ > 0
        && ci->tafAvg_ 
           > GMCC_SRC_CHANGE_CR_FACTOR2 * gsi->crTafAvg_[layer] + d) {
      gsi->cr_[layer] = p->srcAddr_;
    }
  }
   
  if (gsi->cr_[layer] != p->srcAddr_) return;
  
  if (ci->keepalive_) {
    /* Update RTT */
    t = now - ci->tsEcho_;
    if (gsi->rttDev_[layer] < 0) {
      gsi->rttAvg_[layer] = t;
      gsi->rttDev_[layer] = 0;
    }
    else {
      err = t - gsi->rttAvg_[layer];
      gsi->rttAvg_[layer] += GMCC_SRC_RTT_EWMA * err;
      gsi->rttDev_[layer]
        += GMCC_SRC_RTT_EWMA * (fabs (err) - gsi->rttDev_[layer]);
    }
    return;
  }

  gsi->crTafAvg_[layer] = ci->tafAvg_;
  gsi->crTafDev_[layer] = ci->tafDev_;
    
  tw_timer_cancel (lp, gsi->crExtSearchTimerEvnt_ + layer);

  if (gsi->keepaliveTimerEvnt_[layer] != NULL) {
    tw_timer_cancel (lp, gsi->keepaliveTimerEvnt_ + layer);
    GmccSrcSendDataPkt (agent, layer);
    /* Restart rate increasing */
    ReschedTimerWithSn (agent, GMCC_SRC_RATE_INCR_TIMER, layer,
      gsi->rttAvg_[layer] 
        + (gsi->rttDev_[layer] < 0 ? 0 : 2) * gsi->rttDev_[layer],
      gsi->rateIncrTimerEvnt_ + layer);
  }

  GmccSrcSendCtrlPkt (agent, layer);
  
  /* Update RTT */
  t = now - ci->tsEcho_;
  if (gsi->rttDev_[layer] < 0) {
    gsi->rttAvg_[layer] = t;
    gsi->rttDev_[layer] = 0;
  }
  else {
    err = t - gsi->rttAvg_[layer];
    gsi->rttAvg_[layer] += GMCC_SRC_RTT_EWMA * err;
    gsi->rttDev_[layer]
      += GMCC_SRC_RTT_EWMA * (fabs (err) - gsi->rttDev_[layer]);
  }

  if (gsi->congEpochTimerEvnt_[layer] != NULL) {
    /* In congestion epoch. Do not reduce rate */
    return;
  }  

  /* Cut rate */
  if (gsi->oversendLayer_ == layer) {
    gsi->rate_[layer] = ci->rcvRate_
      * GMCC_SRC_RATE_CUT_FACTOR 
      / (1 + GMCC_SRC_OVERSEND_FRAC);
    gsi->oversendLayer_ = -1;
  }
  else {
    gsi->rate_[layer] = ci->rcvRate_ * GMCC_SRC_RATE_CUT_FACTOR;
  }
  if (gsi->rate_[layer] < GMCC_SRC_MIN_RATE) {
    gsi->rate_[layer] = GMCC_SRC_MIN_RATE;
  }

  gsi->congEpochTimerEvnt_[layer] = 
    ScheduleTimerWithSn (agent, GMCC_SRC_CONG_EPOCH_TIMER, layer,
      gsi->rttAvg_[layer] + 4 * gsi->rttDev_[layer]);
  tw_timer_cancel (lp, gsi->rateIncrTimerEvnt_ + layer);

  printf ("%lf : at %d.%ld layer %d , Rate decreased = %lf Mbps\n",
    now, agent->nodeId_, agent->id_, layer, gsi->rate_[layer] / 125000);
}


void GmccSrcTimer (Agent * agent, int timerId, int layer)
{
  GmccSrcInfo * gsi = (GmccSrcInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  double oldRate;
  int i;
  
  if (! gsi->active_) return;

  switch (timerId) {
  case GMCC_SRC_RATE_INCR_TIMER:
    gsi->rateIncrTimerEvnt_[layer] = NULL;
    if (gsi->congEpochTimerEvnt_[layer] != NULL) {
      break;
    }
    
    gsi->rateIncrTimerEvnt_[layer] 
      = ScheduleTimerWithSn (agent, GMCC_SRC_RATE_INCR_TIMER, layer,
          gsi->rttAvg_[layer]
            + (gsi->rttDev_[layer] < 0 ? 0 : GMCC_SRC_RATE_INCR_RTT_FACTOR)
            * gsi->rttDev_[layer]);
    oldRate = gsi->rate_[layer];
    gsi->rate_[layer] += GMCC_DATA_PKT_SIZE
      / (gsi->rttAvg_[layer] 
         + (gsi->rttDev_[layer] < 0 ? 0 : GMCC_SRC_RATE_INCR_RTT_FACTOR)
           * gsi->rttDev_[layer]);

    printf ("%lf : at %d.%ld layer %d , Rate increased = %lf Mbps\n",
      now, agent->nodeId_, agent->id_, layer, gsi->rate_[layer] / 125000);
      
    break;
  case GMCC_SRC_SEND_PKT_TIMER:
    GmccSrcSendDataPkt (agent, layer);
    if (gsi->oversendLayer_ == layer) {
      gsi->sendPktTimerEvnt_[layer] = 
        ScheduleTimerWithSn (agent, GMCC_SRC_SEND_PKT_TIMER, layer,
          GMCC_DATA_PKT_SIZE / (gsi->rate_[layer] 
          * (1 + GMCC_SRC_OVERSEND_FRAC)));
    }
    else {
      gsi->sendPktTimerEvnt_[layer] = 
        ScheduleTimerWithSn (agent, GMCC_SRC_SEND_PKT_TIMER, layer,
          GMCC_DATA_PKT_SIZE / gsi->rate_[layer]);
    }
    break;
  case GMCC_SRC_CONG_EPOCH_TIMER:
    gsi->congEpochTimerEvnt_[layer] = NULL;
    ReschedTimerWithSn (agent, GMCC_SRC_RATE_INCR_TIMER, layer,
      gsi->rttAvg_[layer] + 2 * gsi->rttDev_[layer],
      gsi->rateIncrTimerEvnt_ + layer);
        
    if (gsi->oversendLayer_ < 0
        && layer < gsi->topActLayer_
        && gsi->debtNextLayer_[layer] < 1
        && (layer == 0 || gsi->debtNextLayer_[layer - 1] < 1)
        && (rand () / (double) (RAND_MAX)) < GMCC_SRC_OVERSEND_PROB) {
      gsi->oversendLayer_ = layer;
    }
    
    break;
  case GMCC_SRC_CR_CHECK_TIMER:
    gsi->crCheckTimerEvnt_[layer] = NULL;
    gsi->cr_[layer] = (uint32) -1;
    if (gsi->oversendLayer_ == layer) gsi->oversendLayer_ = -1;

    /* Layer 0 will never become inactive 
     * And only top active layer can become inactive */

    if (layer == 0 || layer < gsi->topActLayer_) break; 

    ReschedTimerWithSn (agent, GMCC_SRC_CR_EXT_SEARCH_TIMER, layer,
      GMCC_SRC_CR_EXT_SEARCH_SPAN * (gsi->grttAvg_ + 4 * gsi->grttDev_),
      gsi->crExtSearchTimerEvnt_ + layer);
    break;
  case GMCC_SRC_CR_EXT_SEARCH_TIMER:
    gsi->crExtSearchTimerEvnt_[layer] = NULL;
    if (gsi->oversendLayer_ == layer) gsi->oversendLayer_ = -1;
    tw_timer_cancel (lp, gsi->rateIncrTimerEvnt_ + layer);
    gsi->rate_[layer] = GMCC_DATA_PKT_SIZE
      / gsi->grttAvg_; /* Set rate to 1 pkt / GRTT */
    ReschedTimerWithSn (agent, GMCC_SRC_KEEPALIVE_TIMER, layer,
      GMCC_SRC_KEEPALIVE_SPAN * (gsi->grttAvg_ + 4 * gsi->grttDev_),
      gsi->keepaliveTimerEvnt_ + layer);
    break;
  case GMCC_SRC_KEEPALIVE_TIMER:
    gsi->keepaliveTimerEvnt_[layer] = NULL;
    /* This layer now becomes inactive */
    gsi->activeLayer_[layer] = 0; 
    if (gsi->oversendLayer_ == layer) gsi->oversendLayer_ = -1;
    GmccSrcResetLayer (agent, layer);

    for (i = 0; i < gsi->layerNum_; ++ i) {
      if (gsi->activeLayer_[i]) gsi->topActLayer_ = i;
    }

    layer = gsi->topActLayer_;
    /* If the new top active layer does not have a CR, and its CR check
     * timer has expired, then enter layer keepalive stage. */
    if (gsi->crCheckTimerEvnt_[layer] == NULL
        && gsi->crExtSearchTimerEvnt_[layer] == NULL) {    
      tw_timer_cancel (lp, gsi->rateIncrTimerEvnt_  + layer);
      gsi->rate_[layer] = GMCC_DATA_PKT_SIZE / gsi->grttAvg_; 
                                                /* Set rate to 1 pkt / GRTT */         ReschedTimerWithSn (agent, GMCC_SRC_KEEPALIVE_TIMER, layer,
        GMCC_SRC_KEEPALIVE_SPAN * (gsi->grttAvg_ + 4 * gsi->grttDev_),
        gsi->keepaliveTimerEvnt_ + layer);
    }
     
    GmccSrcSendCtrlPkt (agent, layer);
    break;
  default:
    printf ("Unknown GMCC source timer.\n");
    tw_exit (-1);
  };
}


void GmccSrcPrintStat (Agent * agent)
{}
