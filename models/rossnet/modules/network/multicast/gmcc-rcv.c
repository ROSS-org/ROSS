#include "gmcc-rcv.h"
#include "gmcc-util.h"


Agent * GmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr, 
                     int8 trackStat)
{
  Agent * a;
  GmccRcvInfo * gri;
  
  if (IsMcastAddr (listenAddr)) {
    a = GetAgent (nodeId, listenAddr);
  }
  else {
    a = GetAgent (nodeId, (uint32) agentId);
  }
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, listenAddr, 
                     (PktHandler) GmccRcvPktHandler, 
                     (StartFunc) GmccRcvStart,
                     (StopFunc) GmccRcvStop,
                     (TimerFunc) GmccRcvTimer,
                     (AgentFunc) GmccRcvPrintStat,
                     NULL);

  gri = malloc (sizeof (GmccRcvInfo));
  
  if (gri == NULL) {
    printf ("Failed to allocate memory for TFMCC receiver info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) gri, 0, sizeof (GmccRcvInfo));
  a->info_ = gri;
  gri->ciSent_ = 0;
  gri->trackStat_ = trackStat;
             
  return a;                             
}


void GmccRcvStart (Agent * agent)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  int i;
  
  gri->active_ = 1;

  gri->grttAvg_ = 0.5;
  gri->grttDev_ = 0;  
  for (i = 0; i < GMCC_MAX_LAYER_NUM; ++ i) {
    GmccRcvInitLayer (agent, i);
    gri->activeLayer_[i] = gri->joinedLayer_[i] = 0;
    gri->osTolr_[i] = GMCC_RCV_DEF_OS_TOLERENCE;
    gri->cr_[i] = (uint32) -1;
    gri->lastCiTime_[i] = -1;
  }
  gri->activeLayer_[0]  = gri->joinedLayer_[0] = 1;
  gri->begTime_ = tw_now (tw_getlp (agent->nodeId_));
  gri->lastRateRecTime_ = -1;
  gri->itvlRcvd_ = gri->totalRcvd_ = 0; 

  gri->layerNum_ = 1;
  gri->joinResult_ = 1;
  gri->topActLayer_ = gri->topJoinedLayer_ = gri->topJoinedActLayer_ = 0;
  gri->lastJoinedLayer_ = -1;
}


void GmccRcvStop (Agent * agent)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  
  gri->active_ = 0;
}


void GmccRcvInitLayer (Agent * agent, int layer)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);

  gri->sampleNum_[layer] = gri->ovsdSampleNum_[layer] = 0;
  gri->lastRcvTime_[layer] = gri->lastCongTime_[layer] 
    = gri->lastCor_[layer] = -1;
  gri->srtt_[layer] = gri->grttAvg_;
  RateAvgrInit (gri->rcvRateAvg_ + layer, 2);
  AvgCalcInit (gri->cgItvlAvgCalc_ + layer, GMCC_RCV_CG_ITVL_SAMPLE_SIZE);
  AvgCalcInit (gri->itafAvg_ + layer, GMCC_RCV_SAMPLE_SIZE);
  AvgCalcInit (gri->itafAvg2_ + layer, GMCC_RCV_SAMPLE_SIZE);
  StatEstimatorInit (gri->itafEst_ + layer, 
    GMCC_RCV_SAMPLE_SIZE, GMCC_RCV_TAF_EWMA);
  StatEstimatorInit (gri->itafEst2_ + layer, 
    GMCC_RCV_SAMPLE_SIZE, GMCC_RCV_TAF_EWMA);
  StatEstimatorInit (gri->tafEst_ + layer, 
    GMCC_RCV_SAMPLE_SIZE, GMCC_RCV_TAF_EWMA);

  gri->lessCgNum_[layer] = gri->lessCgNum2_[layer] = gri->moreCgNum_[layer] = 0;
  gri->contPktNum_[layer] = 0;
  gri->lastSndRate_[layer] = 0;
  gri->lastResetTime_[layer] = now;
}


void GmccRcvInitLayer2 (Agent * agent, int layer, int refLayer)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  int i, n[4];
  double s[4], a;

  GmccRcvInitLayer (agent, layer);
  
  for (i = 0; i < 4; ++ i) {
    s[i] = n[i] = 0;
  }
  for (i = 0; i <= refLayer; ++ i) {
    if (! gri->activeLayer_[i]) continue;
    if (gri->tafEst_[i].sampleNum_ > 0) {
      s[2] += gri->tafEst_[i].mean_;
      ++ n[2];
    }
    
    if (gri->cgItvlAvgCalc_[i].sampleNum_ > 0) {
      s[3] += AvgCalcGetMean (gri->cgItvlAvgCalc_ + i);
      ++ n[3];
    }    
  }
    
  StatEstimatorAddSample (gri->tafEst_ + layer, 
    n[2] == 0 ? 0 : (s[2] / n[2]));
  
  if (n[3] == 0) return;
  a = s[3] / n[3];
  for (i = 0; i < GMCC_RCV_SAMPLE_SIZE; ++ i) {
    AvgCalcAddSample (gri->cgItvlAvgCalc_ + layer, a);
  }
}


void GmccRcvPktHandler (Agent * agent, Packet * p)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  
  if (! gri->active_) return;

  switch (p->type_) {
  case GMCC_DATA:
    GmccRcvProcDataPkt (agent, p);
    return;
  case GMCC_CTRL:
    GmccRcvProcCtrlPkt (agent, p);
    return;
  default:
    printf ("Unknown packet receiverd by GMCC receiver.\n");
    tw_exit (-1);
  }
}


void GmccRcvProcCtrlPkt (Agent * agent, Packet * p)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  GmccCtrlExt * gce = GetExtPkt (p->srcAddr_, p->srcAgent_, p->id_);
  int i, layer;
  double testTaf, d;
  
  if (gce == NULL) {
    printf ("Invalid extended GMCC control packet.\n");
    tw_exit (-1);
  }
  
  layer = gce->layer_;
  gri->senderAddr_ = p->srcAddr_;
  gri->senderAgent_ = p->srcAgent_;

  gri->layerNum_ = gce->layerNum_;
  //gri->totalRcvd_ += p->size_;
  
  /* Update info for all layers */  
  gri->grttAvg_ = gce->grttAvg_;
  gri->grttDev_ = gce->grttDev_;
  for (i = 0; i < gri->layerNum_; ++ i) {
    gri->crTafAvg_[i] = gce->crTafAvg_[i];
    gri->crTafDev_[i] = gce->crTafDev_[i];
    gri->grpAddrs_[i] = gce->grpAddrs_[i];
    gri->srtt_[i] = gce->srtt_[i];
    
    if (gri->cr_[i] != gce->cr_[i]) {
      gri->cr_[i] = gce->cr_[i];

      /* CR of this layer has changed. Previous results may be invalidated. */
      gri->lessCgNum_[i] = gri->lessCgNum2_[i] = gri->moreCgNum_[i] = 0;
      GmccRcvUpdateCgCmp (agent, i);
    }    
    
    if (gri->activeLayer_[i] && ! gce->activeLayer_[i]
        && (i != gri->lastJoinedLayer_ || gri->joinResult_ != 0)) {
      GmccRcvInitLayer2 (agent, i, i - 1);

      /* The higher layer is inactive, so the oversend statistics
       * is invalidated. */
      AvgCalcInit (gri->itafAvg2_ + i - 1, GMCC_RCV_SAMPLE_SIZE);
      StatEstimatorInit (gri->itafEst2_ + i - 1, GMCC_RCV_SAMPLE_SIZE,
        GMCC_RCV_TAF_EWMA);
    }
        
    if (gri->activeLayer_[i] == gce->activeLayer_[i]) {
      gri->topActLayer_ = i;      
      if (gri->joinedLayer_[i]) gri->topJoinedActLayer_ = i;
    }
  }  
  
  /* If last joined layer is still inactive, and there is not result of 
   * last join, probably the activation pkt has been lost. Send again. */
  if (gri->joinResult_ == 0 && ! gri->activeLayer_[gri->lastJoinedLayer_]) {
    GmccRcvSendCiPkt (agent, gce->timestamp_, gri->lastJoinedLayer_, 
      RateAvgrGetMean (gri->rcvRateAvg_ + gri->lastJoinedLayer_), 0, 0);
  }
  
  /* If this receiver is chosen as CR for the last joined layer, deem the
   * join is successful */
  if (gri->joinResult_ == 0 
      && gri->cr_[gri->lastJoinedLayer_] == _nodeTable[lp->id].addr_) {
    /* activeLayer_[lastJoinedLayer_] implied
     * Since this success may not be a true one, don't update oversend
     * tolerence factor */
    gri->joinResult_ = 1;
  }  
  
  /* This ctrl pkt cannot trigger congestion check */
  if (layer < 0 || ! gri->joinedLayer_[layer] || ! gri->activeLayer_[layer]
      || gri->cr_[layer] == _nodeTable[lp->id].addr_) return;

  if (gri->itafEst_[layer].sampleNum_ + gri->itafEst2_[layer].sampleNum_ == 0) {
    /* No loss after last join */
    if (++ gri->lessCgNum2_[layer] < gri->confidentNum2_) return ;
  }
  else {
    if (gri->itafEst_[layer].sampleNum_ > 0) {
      testTaf = StatEstimatorTest (gri->tafEst_ + layer,
        gri->itafEst_[layer].mean_
        / AvgCalcTest (gri->cgItvlAvgCalc_ + layer,
                       now - gri->lastCongTime_[layer]));
      
    }
    else {
      testTaf = StatEstimatorTest (gri->tafEst_ + layer,
        gri->itafEst2_[layer].mean_
        / AvgCalcTest (gri->cgItvlAvgCalc_ + layer,
                       now - gri->lastCongTime_[layer]));
    }

    d = GMCC_RCV_JUMP_FACTOR2 * GMCC_RCV_LESS_CG2_CO * testTaf
        + GMCC_RCV_JUMP_FACTOR
          * sqrt (gri->crTafDev_[layer] * gri->crTafDev_[layer]
                  / GMCC_RCV_SAMPLE_SIZE);

    if (gri->crTafAvg_[layer] <= d) {
      gri->lessCgNum2_[layer] = 0;
      return;
    }

    if (++ gri->lessCgNum2_[layer] < GMCC_RCV_CONFIDENT_NUM2) return;
  }

  /* 
   * Check last join result
   */
  if (gri->joinResult_ == 0) {
    GmccRcvCheckGoodJoin (agent);  
    if (gri->joinResult_ == 0) return;
  }
  
  /* Can only join from the top joined actived layer */
  if (layer != gri->topJoinedLayer_) return;
  
  /*
   * Check whether all joined layers have enough samples
   */
  for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
    if (gri->sampleNum_[i] + gri->ovsdSampleNum_[i]
        >= GMCC_RCV_REQD_SAMPLE_SIZE) continue;
    /* If a layer does not have enough loss sample, it must have enough
     * samples for less congestion */
    if (gri->lessCgNum2_[i] < GMCC_RCV_CONFIDENT_NUM2) return;
  }
  
  /*
   * Now can try to join
   */    
  gri->lastJoinOsTolr_ = -1;
  gri->lastJoinFrom_ = layer;
  GmccRcvJump (agent, gce->timestamp_, 3);
}


void GmccRcvProcDataPkt (Agent * agent, Packet * p)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  GmccData * data = &(p->type.gd_);
  int layer = data->layer_;
  int i;
  short mature;
  double itafSample, itaf, cor, taf;
  double osr, orr;
  int drop;
  double d, d1, d2, mu, dev;

  gri->senderAddr_ = p->srcAddr_;
  gri->senderAgent_ = p->srcAgent_;

  if (gri->trackStat_) {
    gri->totalRcvd_ += p->size_;
    gri->itvlRcvd_ += p->size_;
    if (gri->lastRateRecTime_ < 0) {
      printf ("%lf at %d.%lu total rcv rate 0 Mbps , avg 0 Mbps\n",
        now, lp->id, GetGrpAddr4Print (gri->grpAddrs_[0]));
      gri->lastRateRecTime_ = now;
    }
    else if (now - gri->lastRateRecTime_ >= GMCC_RCV_TTL_RATE_SAMPLE_ITVL) {
      printf ("%lf at %d.%lu total rcv rate %lf Mbps , avg %lf Mbps\n",
        now, lp->id, GetGrpAddr4Print (gri->grpAddrs_[0]), 
        gri->itvlRcvd_ / (now - gri->lastRateRecTime_) / 125000,
        gri->totalRcvd_ / (now - gri->begTime_) / 125000);
      gri->itvlRcvd_ = 0;
      gri->lastRateRecTime_ = now;
    }  
  }
  
  if (! gri->joinedLayer_[layer]) {
    /* Should not receive this packet because the receiver has not joined
     * this layer */
    return;
  }

  /* Ctrl pkt may be missed */
  if (! gri->activeLayer_[layer]) gri->activeLayer_[layer] = 1;
  if (gri->topJoinedActLayer_ < layer) gri->topJoinedActLayer_ = layer;

  if (gri->lastRcvTime_[layer] < 0) {  /* First packet */    
    gri->lastContSqn_[layer] = data->sqn_ - 1;
    gri->lastCongTime_[layer] = now;
  }
  
  if ((gri->lastContSqn_[layer] + 1) == data->sqn_) { /* No loss */
    gri->lastRcvTime_[layer] = now;
    gri->lastContSqn_[layer] = data->sqn_;
    gri->lastSndRate_[layer] = data->rate_;
    ++ gri->contPktNum_[layer];
    RateAvgrAddSample (gri->rcvRateAvg_ + layer, now, p->size_);
    
    if (now - gri->lastCiTime_[layer] < gri->grttAvg_ / 2) {
      return;
    }      

    if (gri->cr_[layer] == _nodeTable[lp->id].addr_) {
      /* Send echo to keep CR valid */
      gri->lastCiTime_[layer] = now;
      GmccRcvSendCiPkt (agent, data->timestamp_, layer, 0, 1, 1);
      return;
    }

    /* Only the top active layer needs to be kept alive */
    if (layer < gri->topActLayer_ || gri->cr_[layer] != (uint32) -1) {
      return;
    }
        
    /* 
     * Top active layer needs to kept alive
     */
    for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
      if (i == gri->topActLayer_ || ! gri->activeLayer_[i]) continue;
      if (gri->moreCgNum_[i] > 0 || gri->cr_[i] == _nodeTable[lp->id].addr_) {
        /* This receiver is already or about to be a CR, 
         * so can't keep the top active layer alive */
        return;
      }
    }
    
    mature = 1;
    
    if (gri->sampleNum_[layer] + gri->ovsdSampleNum_[layer]
        < GMCC_RCV_REQD_SAMPLE_SIZE) {
      mature = 0;
    }
    
    gri->lastCiTime_[layer] = now;
    GmccRcvSendCiPkt (agent, data->timestamp_, layer, 0, mature, 1);
    return;
  }
  
  /* 
   * Packets lost. Caculate TAF and TRAC varience ratio
   */
     
  /* Old sending rate and receiving rate */
  osr = gri->lastSndRate_[layer];
  orr = RateAvgrGetMean (gri->rcvRateAvg_ + layer);

  itafSample = (data->rate_ <= 0) ? 1 : 1 - orr / osr;

  RateAvgrAddSample (gri->rcvRateAvg_ + layer, now, p->size_);
  gri->lastSndRate_[layer] = data->rate_;
  gri->contPktNum_[layer] = 1;

  if (itafSample < 0) itafSample = 0;
  
  if (! data->oversend_) {
    itaf = AvgCalcAddSample (gri->itafAvg_ + layer, itafSample);
    StatEstimatorAddSample (gri->itafEst_ + layer, itaf);
    ++ gri->sampleNum_[layer];
  }
  else {
    itaf = AvgCalcAddSample (gri->itafAvg2_ + layer, itafSample);
    StatEstimatorAddSample (gri->itafEst2_ + layer, itaf);
    ++ gri->ovsdSampleNum_[layer];
  }

  cor = 1 / AvgCalcAddSample (gri->cgItvlAvgCalc_ + layer,
    now - gri->lastCongTime_[layer]);
  gri->lastCongTime_[layer] = now;
 
  StatEstimatorAddSample (gri->tafEst_ + layer, itaf * cor);
  taf = gri->tafEst_[layer].mean_;
  gri->lastRcvTime_[layer] = now;
  GmccRcvUpdateCgCmp (agent, layer);

  gri->lastContSqn_[layer] = data->sqn_;

  /* 
   * Check whether the receiver should leave the top joined active layer
   */
  drop = 0;
  
  if (gri->moreCgNum_[layer] > gri->confidentNum3_ 
      || gri->cr_[layer] == _nodeTable[lp->id].addr_) {
    for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
      if (i == layer || ! gri->activeLayer_[i]) continue;
      if (gri->moreCgNum_[i] > GMCC_RCV_CONFIDENT_NUM3 
         || gri->cr_[i] == _nodeTable[lp->id].addr_) {
        drop = 1;
        break;
      }
    }
  }
  
  if (drop) { /* Should leave the top layer */
    if (gri->joinResult_ == 0) { /* Join result has not been updated */
      gri->joinResult_ = -1;
      
      if (gri->lastJoinOsTolr_ > 0) {
        gri->osTolr_[gri->lastJoinFrom_] 
          = GMCC_RCV_OVS_TOLR_DECR_FACTOR * gri->lastJoinOsTolr_;
      }
    }
      
    if (GmccRcvLeave (agent, layer, drop) && ! gri->joinedLayer_[layer]) {
      return;
    }
  }
  
  /* No drop. */
  
  else do {  
    /* 
     * Check join result first
     */
    if (gri->joinResult_ == 0) {
      GmccRcvCheckGoodJoin (agent);
      if (gri->joinResult_ == 0) break;
    }
    
    /* Can only join from the top joined layer */
    if (layer != gri->topJoinedActLayer_) break;

    /*
     * Check whether all joined layers have enough samples
     * NOTE: different from that in RecvCtrlPkt ()
     */
    for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
      if (gri->activeLayer_[i]
          && gri->sampleNum_[i] + gri->ovsdSampleNum_[i] 
             < GMCC_RCV_REQD_SAMPLE_SIZE) break;
    }

    if (i <= gri->topJoinedLayer_) break;  /* Layer i does not have enough
                                            * samples */
    
    /*
     * Check whether the receiver can try to join another layer
     */    
    if (gri->lessCgNum_[layer] >= GMCC_RCV_CONFIDENT_NUM) {
      gri->lastJoinOsTolr_ = -1;
      gri->lastJoinFrom_ = layer;
      GmccRcvJump (agent, data->timestamp_, 1);
      break;
    }
    
    if (gri->itafEst_[layer].sampleNum_ >= GMCC_RCV_REQD_SAMPLE_SIZE
        && gri->itafEst2_[layer].sampleNum_ >= GMCC_RCV_REQD_SAMPLE_SIZE2) {
      d1 = gri->itafEst_[layer].dev_;
      d2 = gri->itafEst2_[layer].dev_;
      d = gri->osTolr_[layer] * sqrt (
        (d1 * d1) / gri->itafEst_[layer].sampleNum_
         + (d2 * d2) / gri->itafEst2_[layer].sampleNum_);
      mu = gri->itafEst2_[layer].mean_ - gri->itafEst_[layer].mean_;
      
      if (mu - d <= 0 && mu + d >= 0) {
        gri->lastJoinOsTolr_ = gri->osTolr_[layer];
        gri->lastJoinFrom_ = layer;
        GmccRcvJump (agent, data->timestamp_, 2);
      }
    }
  } while (0);

  /*
   * Decide whether this receiver can be a CR
   * If reach here, this receiver is not a (possible) CR for more than one layer
   */
  if (gri->cr_[layer] == _nodeTable[lp->id].addr_) {
    /* If it is the CR of this layer, should keep being CR */
    GmccRcvSendCiPkt (agent, data->timestamp_, layer, orr, 1, 0);
    return;
  }
     
  /* If is already the CR of another layer. Cannot be CR for this layer */
  for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
    if (i == layer || ! gri->activeLayer_[layer]) continue;
    if (gri->cr_[i] == _nodeTable[lp->id].addr_ || gri->moreCgNum_[i] > 0) { 
      return;
    }
  }

  do {
    if (gri->cr_[layer] == (uint32) -1 && layer == gri->topActLayer_)
      break;  /* There is no CR in the top active layer yet */
        
    if (layer == gri->lastJoinedLayer_ && gri->joinResult_ == 0
        && gri->cr_[layer] != _nodeTable[lp->id].addr_) {  
      /* cr_[layer].addr_ >= 0 implied */
      return;
    }
  
    /* cr_[layer] != here_ implied */
    if (gri->sampleNum_[layer] + gri->ovsdSampleNum_[layer]
        < GMCC_RCV_REQD_SAMPLE_SIZE) {
      /* There is now a CR for this layer, so we need enough samples 
       * to get correct comparison */
      return;
    }


    dev = gri->tafEst_[layer].dev_;
    d = sqrt ((dev * dev
               + GMCC_RCV_CHANGE_CR_FACTOR2 * GMCC_RCV_CHANGE_CR_FACTOR2
                 * gri->crTafDev_[layer] * gri->crTafDev_[layer])
               / GMCC_RCV_SAMPLE_SIZE);
    d = GMCC_RCV_CHANGE_CR_FACTOR2 * gri->crTafAvg_[layer]
        + GMCC_RCV_CHANGE_CR_FACTOR * d;
    
    /* Can be CR only if TAF is high enough */
    if (gri->tafEst_[layer].mean_ <= d) {
      return;
    }
  } while (0);

  GmccRcvSendCiPkt (agent, data->timestamp_, layer, orr, 1, 0);
}


void GmccRcvSendCiPkt  (Agent * agent, double timestamp, int layer, 
                        double rcvRate, short mature, short keepalive)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  Packet p;
  GmccCi * ci = & (p.type.gci_);
  
  if (! gri->active_) return;

  p.type_ = GMCC_CI;
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = gri->senderAddr_;
  p.dstAgent_ = gri->senderAgent_;
  
  p.size_ = GMCC_CI_PKT_SIZE;
  p.inPort_ = (uint16) -1;

  /* Construct CI and send */

  ci->tsEcho_ = timestamp;
  if (gri->tafEst_[layer].sampleNum_ == 0) {
    ci->tafAvg_ = ci->tafDev_ = 0;
  }
  else {
    ci->tafAvg_ = gri->tafEst_[layer].mean_;
    ci->tafDev_ = gri->tafEst_[layer].dev_;
  }
  ci->rcvRate_ = rcvRate;
  ci->layer_ = layer;
  ci->mature_ = mature;
  ci->keepalive_ = keepalive;
  
  ++ gri->ciSent_;
  
  SendPacket (tw_getlp (agent->nodeId_), &p);
}


int GmccRcvJump (Agent * agent, double timestamp, int reason)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  int i, availLayer, moreCg;
  
  if (gri->joinResult_ == 0) {
    return 0; /* No result of last join yet */
  }

  /* Check whether all joined layers have CR and have got
   * enough samples. If not, don't join new layer */
  availLayer = gri->topJoinedLayer_ + 1;
  moreCg = 0;
  for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
    if (! gri->activeLayer_[i]) {
      availLayer = i;
      continue;
    }
    if (gri->cr_[i] == (uint32) -1 || gri->crTafAvg_[i] <= 0 
        || gri->crTafDev_[i] < 0) {
      return 0; /* This layer does not have CR yet */
    }
    if (gri->moreCgNum_[i] > 0 || gri->cr_[i] == _nodeTable[lp->id].addr_) {
      if (++ moreCg == 1) {  /* Limit the number of layers in which 
                              * the receiver can be the most congested
                              * for join */
        return 0;
      }
    }
  }
  
  if (availLayer >= gri->layerNum_) {
    return 0; /* No free layer found */
  }
 
  /* If the layer to join is already active but does not have a CR yet,
   * don't join */ 
  if (availLayer > gri->topJoinedLayer_) {
    if (gri->activeLayer_[availLayer] && gri->cr_[i] == (uint32) -1) {
      return 0;
    }
    gri->topJoinedLayer_ = gri->lastJoinedLayer_ = availLayer;
  }
  else {
    gri->lastJoinedLayer_ = availLayer;
  } 
    
  /* Layer i is the lowest inactive layer this receiver is in
   * or the lowest active layer this receiver has not joined
   * If not in this layer, join it */
  if (! gri->joinedLayer_[gri->lastJoinedLayer_]) {
    JoinGroup (lp->id, gri->grpAddrs_[gri->lastJoinedLayer_]);  
    RegisterAgentCopy (agent, gri->grpAddrs_[gri->lastJoinedLayer_]);
    gri->joinedLayer_[gri->lastJoinedLayer_] = 1;
  }
  gri->joinResult_ = 0;

  /* Reinitialize statistic for this layer */ 
  GmccRcvInitLayer2 (agent, gri->lastJoinedLayer_, gri->lastJoinedLayer_ - 1);
  
  /* "Less congested" statistics is not valid any more for any layer */
  for (i = 0; i < gri->layerNum_; ++ i) {
    gri->lessCgNum_[i] = gri->lessCgNum2_[i] = 0;
    gri->moreCgNum_[i] = 0; /* To prevent prompt leave after join */
    gri->lastResetTime_[i] = now;
    gri->sampleNum_[i] = gri->ovsdSampleNum_[i] = 0;
  }

  /* Send a CI after joining a new layer to let the source know */
  GmccRcvSendCiPkt (agent, timestamp, gri->lastJoinedLayer_, 0, 0, 0);
  return 1;
}


int GmccRcvLeave (Agent * agent, int layer, int reason)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  int i, j;

  /* Finr top joined active layer */
  for (i = gri->topJoinedLayer_; i > 0; -- i) {
    if (gri->activeLayer_[i]) break;
  }

  if (i == 0) {
    return 0;
  }
  
  j = i - 1;
  /* Leave the top joined active layer and all joined layer above */
  for (; i <= gri->topJoinedLayer_; ++ i) {
    LeaveGroup (lp->id, gri->grpAddrs_[i]);
    UnregisterAgentCopy (lp->id, gri->grpAddrs_[i]);
    gri->joinedLayer_[i] = 0;
    GmccRcvInitLayer (agent, i);
  }
  gri->topJoinedLayer_ = j;
  
  /* "more/equal congested" statistics of all layers is now invalid */
  for (i = 0; i < gri->layerNum_; ++ i) {
    gri->moreCgNum_[i] = 0;
    gri->lessCgNum_[i] = gri->lessCgNum2_[i] = 0; /* To prevent prompt join
                                                   * after leave */
    gri->lastResetTime_[i] = now;
    gri->sampleNum_[i] = gri->ovsdSampleNum_[i] = 0;
  }
  
  return 1;
}


void GmccRcvCheckGoodJoin (Agent * agent)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  int i;
  
  if (! gri->activeLayer_[gri->lastJoinedLayer_]) return;

  for (i = 0; i <= gri->topJoinedLayer_; ++ i) {
    if (gri->lessCgNum2_[i] >= GMCC_RCV_CONFIDENT_NUM2) {
      continue;  /* No increased cong in this layer */
    }
    if (gri->sampleNum_[i] + gri->ovsdSampleNum_[i]
        < GMCC_RCV_REQD_SAMPLE_SIZE) return;
    if (i == gri->lastJoinedLayer_) continue;
    if (gri->moreCgNum_[i] > 0 
       || gri->cr_[i] == _nodeTable[agent->nodeId_].addr_) {
      return;
    }
  }

  /* Join succeeded */
  gri->joinResult_ = 1;
  if (gri->lastJoinOsTolr_ < 0) return;
    
  gri->osTolr_[gri->lastJoinFrom_] = GMCC_RCV_OVS_TOLR_INCR_FACTOR
    *  gri->lastJoinOsTolr_;
  if (gri->osTolr_[gri->lastJoinFrom_] > 3.5) {
    gri->osTolr_[gri->lastJoinFrom_] = 3.5;
  }
}


void GmccRcvUpdateCgCmp (Agent * agent, int i)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  double mean, dev, d1, d2, d3;
  double alpha, beta;
  int n;

  /* 
   * Update less/more-congested-than-CR status for this layer 
   * Same as in RecvDataPkt ()
   */
  if (gri->cr_[i] == _nodeTable[agent->nodeId_].addr_) {
    gri->moreCgNum_[i] = gri->lessCgNum_[i] = gri->lessCgNum2_[i] = 0;
    return;
  }
  
  if (! gri->activeLayer_[i] || gri->cr_[i] == (uint32) -1 
      || gri->crTafAvg_[i] < 0 || gri->tafEst_[i].sampleNum_ == 0) {
    return;
  }
  
  alpha = GMCC_RCV_CHANGE_CR_FACTOR2;
  beta = GMCC_RCV_JUMP_FACTOR2;
  n = gri->tafEst_[i].sampleNum_;
  mean = gri->tafEst_[i].mean_;
  dev = gri->tafEst_[i].dev_;
  d1 = sqrt (dev * dev / n
             + gri->crTafDev_[i] * gri->crTafDev_[i] / GMCC_RCV_SAMPLE_SIZE);
  d1 = gri->crTafAvg_[i] + GMCC_RCV_MORE_CG_FACTOR * d1;
  d2 = sqrt (beta * beta * dev * dev / n
             + gri->crTafDev_[i] * gri->crTafDev_[i] / GMCC_RCV_SAMPLE_SIZE);
  d2 = beta * mean + GMCC_RCV_JUMP_FACTOR * d2;
  d3 = sqrt (dev * dev / n
             + alpha * alpha * gri->crTafDev_[i] * gri->crTafDev_[i]
               / GMCC_RCV_SAMPLE_SIZE);
  d3 = alpha * gri->crTafAvg_[i] + GMCC_RCV_CHANGE_CR_FACTOR * d3;

  if (gri->crTafAvg_[i] > d2) {
    ++ gri->lessCgNum_[i];
    gri->moreCgNum_[i] = 0;
    return;
  }
  else {
    gri->lessCgNum_[i] = 0;
  }

  if (gri->sampleNum_[i] + gri->ovsdSampleNum_[i] 
      < GMCC_RCV_REQD_SAMPLE_SIZE) {
    return;
  }

  if (mean > d1) {
    ++ gri->moreCgNum_[i];
    gri->lessCgNum_[i] = 0;
    return;
  }
  
  gri->moreCgNum_[i] = gri->lessCgNum_[i] = 0;
}


void GmccRcvTimer (Agent * agent, int timerId)
{
  GmccRcvInfo * gri = (GmccRcvInfo *) agent->info_;
  
  if (! gri->active_) return;
}


void GmccRcvPrintStat (Agent * agent)
{}


