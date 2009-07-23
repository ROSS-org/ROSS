#include "tfmcc-sink.h"
#include "tfmcc.h"
#include "formula.h"

uint32 _totalTfmccNak = 0;

/***********************************
 T F M C C  R E C E I V E R
 ***********************************/


Agent * TfmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr)
{
  Agent * a;
  TfmccRcvInfo * tri;
  
  if (IsMcastAddr (listenAddr)) {
    a = GetAgent (nodeId, listenAddr);
  }
  else {
    a = GetAgent (nodeId, (uint32) agentId);
  }
  
  if (a != NULL) return a;
  
  a = RegisterAgent (nodeId, agentId, listenAddr, 
                     (PktHandler) TfmccRcvPktHandler, 
                     (StartFunc) TfmccRcvStart,
                     (StopFunc) TfmccRcvStop,
                     (TimerFunc) TfmccRcvTimer,
                     (AgentFunc) TfmccRcvPrintStat,
                     NULL);

  tri = malloc (sizeof (TfmccRcvInfo));
  
  if (tri == NULL) {
    printf ("Failed to allocate memory for TFMCC receiver info.\n");
    tw_exit (-1);
  }
  
  memset ((void *) tri, 0, sizeof (TfmccRcvInfo));
  a->info_ = tri;
             
  return a;                             
}


void TfmccRcvStart (Agent * agent)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  
  tri->active_ = 1;
  
  tri->fairsize_ = TFMCC_SRC_DATA_PKT_SIZE;
  tri->aggregate_packets_ = 1;
  tri->aggregate_loss_ = 0;
  tri->hsz = 100000;
  tri->adjust_history_after_ss = 1;
  tri->numsamples = -1;
  tri->discount = 1;
  tri->smooth_ = 1;
  tri->df_ = 0.9;
  tri->rate_ = 0;
  tri->bval_ = 1;
  tri->tight_loop_ = 1;
  tri->fbtime_mult_ = 6.0;
  tri->constant_rate_ = 0;  
    
  tri->rtt_ = 0;
  tri->rtt_est = 0;
  tri->one_way_ = 0;
  tri->last_timestamp_ = 0;
  tri->last_arrival_ = 0;
  tri->last_report_sent=0;
  tri->avgsize_ = 0;
  tri->sum_psize_ = 0;
  tri->sum_lsize_ = 0;

  tri->maxseq = -1;
  tri->rcvd_since_last_report  = 0;
  tri->loss_seen_yet = 0;
  tri->lastloss = -1000; // also count loss event directly after startup
  tri->false_sample = 0;
  tri->round_id = -1;    // so that first data packet starts a new round
  tri->sample_count = 1 ;
  tri->last_sample = 0;
  tri->mult_factor_ = 1.0;
  tri->representative = 0;
  tri->comparison_rate = 0;
  tri->suppression_rate = 0;
  tri->fb_time = 0;
  tri->receiver_leave = 0;

  tri->rtvec_ = NULL;
  tri->tsvec_ = NULL;
  tri->lossvec_ = NULL;
  tri->sizevec_ = NULL;
  tri->sample = NULL ; 
  tri->weights = NULL ;
  tri->mult = NULL ;
}


void TfmccRcvStop (Agent * agent)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  
  tri->active_ = 0;
  tri->rate_ = 0; // -> receiver leaves session
  tri->receiver_leave = 1;

  if (tri->rtvec_ != NULL) free ((void *) (tri->rtvec_));
  if (tri->tsvec_ != NULL) free ((void *) (tri->tsvec_));
  if (tri->lossvec_ != NULL) free ((void *) (tri->lossvec_));
  if (tri->sizevec_ != NULL) free ((void *) (tri->sizevec_));
  if (tri->sample != NULL) free ((void *) (tri->sample)); 
  if (tri->weights != NULL) free ((void *) (tri->weights));
  if (tri->mult != NULL) free ((void *) (tri->mult));

  tri->rtvec_ = NULL;
  tri->tsvec_ = NULL;
  tri->lossvec_ = NULL;
  tri->sizevec_ = NULL;
  tri->sample = NULL; 
  tri->weights = NULL;
  tri->mult = NULL;
}


void TfmccRcvPktHandler (Agent * agent, Packet * pkt)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  TfmccData * data = (TfmccData *) & (pkt->type.td_);
  int prevmaxseq = tri->maxseq;
  double p = -1;
  double rtt_cur;
  int new_round, i;
  double prev_last_arrival_, tmp_df_;
  
  if (! tri->active_) return;
  
  if (pkt->type_ != TFMCC_DATA) {
    printf ("TFMCC receiver (%d.%lu) received non-data <%d> packet.",
       lp->id, GetGrpAddr4Print (agent->id_), pkt->type_);
    printf (" src = %d.%u\n", _addrToIdx[pkt->srcAddr_], 
       pkt->srcAgent_); 
    tw_exit (-1);
  }

  tri->rcvd_since_last_report ++;

  if (tri->numsamples < 0) {
    // This is the first packet received.
    tri->numsamples = TFMCC_RCV_DEFAULT_NUMSAMPLES ;  
    // forget about losses before this
    prevmaxseq = tri->maxseq = data->seqno-1 ; 
    // initialize last_sample
    tri->last_sample = data->seqno;

    if (tri->smooth_ == 1) {
      tri->numsamples = tri->numsamples + 1;
    }
    tri->sample = (int *) malloc ((tri->numsamples + 1) * sizeof (int));
    tri->weights = (double *) malloc ((tri->numsamples + 1) * sizeof (double));
    tri->mult = (double *) malloc ((tri->numsamples + 1) * sizeof (double));
    for (i = 0 ; i <= tri->numsamples; ++ i) {
      tri->sample[i] = 0 ; 
      tri->mult[i] = 1.0 ; 
    }
    if (tri->smooth_ == 1) {
      tri->weights[0] = 1.0;
      tri->weights[1] = 1.0;
      tri->weights[2] = 1.0; 
      tri->weights[3] = 1.0; 
      tri->weights[4] = 1.0; 
      tri->weights[5] = 0.8;
      tri->weights[6] = 0.6;
      tri->weights[7] = 0.4;
      tri->weights[8] = 0.2;
      tri->weights[9] = 0;
    } else {
      tri->weights[0] = 1.0;
      tri->weights[1] = 1.0;
      tri->weights[2] = 1.0; 
      tri->weights[3] = 1.0; 
      tri->weights[4] = 0.8; 
      tri->weights[5] = 0.6;
      tri->weights[6] = 0.4;
      tri->weights[7] = 0.2;
      tri->weights[8] = 0;
    }
  } 

  tri->psize_ = pkt->size_;
  tri->avgsize_ = (int) ((tri->avgsize_ == 0) 
    ? tri->psize_ : (tri->avgsize_ * 0.9 + tri->psize_ * 0.1));
  prev_last_arrival_ = (tri->last_arrival_ == 0) 
    ? now : tri->last_arrival_; // no packet gap for first packet
  tri->last_arrival_ = now;
  tri->last_timestamp_ = data->timestamp;
  tri->max_rtt = data->max_rtt;

  if (data->is_clr) {
    if (data->rtt_recv_id == _nodeTable[lp->id].addr_)
      tri->representative = 1;
    else
      tri->representative = 0;
  }

  if (data->rtt_recv_id == _nodeTable[lp->id].addr_) {
    rtt_cur = now - data->timestamp_echo - data->timestamp_offset;
    tri->one_way_ = data->timestamp - data->timestamp_echo
      - data->timestamp_offset;
    if (tri->rtt_ > 0) {
      tmp_df_ = tri->representative ? tri->df_ : 0.5;
      tri->rtt_est = tri->rtt_ 
        = tmp_df_ * tri->rtt_ + (1 - tmp_df_) * (rtt_cur);
    } else {
      tri->rtt_est = tri->rtt_ = rtt_cur;

      // adjust previously faked lost history!
      if (tri->false_sample == -1 && tri->sample_count <= tri->numsamples + 1) {
        if (tri->rtt_ < tri->rtt_lossinit) {
          tri->sample[tri->sample_count - 1] = (int)
           (tri->sample[tri->sample_count - 1] 
            * (tri->rtt_ / tri->rtt_lossinit)
            * (tri->rtt_ / tri->rtt_lossinit));
          if (tri->sample[tri->sample_count-1] < 1) {
            tri->sample[tri->sample_count-1] = 1;
          }
        }
      }
    }
  }
  else {
    // do we have an initial measurement?
    if (tri->one_way_ > 0) {
      rtt_cur = tri->one_way_ + (now - data->timestamp);
      tri->rtt_est = rtt_cur;
    }
  }
  
  TfmccRcvAddPktToHistory (agent, pkt);

  if ((tri->loss_seen_yet == 0) && (data->seqno - prevmaxseq > 1)) {
    tri->loss_seen_yet = 1;
    if (tri->adjust_history_after_ss) {
      TfmccRcvAdjustHistory (agent, data->timestamp);
    }
  }

  if (tri->rtt_est > 0)
    tri->conservative_rtt = tri->rtt_est;
  else
    tri->conservative_rtt = tri->max_rtt;

  p = TfmccRcvEstLoss (agent);
  if (p == 0) {
    tri->rate_ = 2 * TfmccRcvEstThput (agent);
  }
  else {
    tri->rate_ = p_to_b 
      (p, tri->conservative_rtt, 4 * tri->conservative_rtt, 
       tri->fairsize_, tri->bval_);
  }

  // report a constant rate (for debugging etc)
  if (tri->constant_rate_ > 0) tri->rate_ = tri->constant_rate_ / 8.0;

  if (tri->round_id < data->round_id 
     || tri->round_id > data->round_id + TFMCC_MAX_ROUND_ID / 2) {
    tri->round_id = data->round_id;
    new_round = 1;
  } else {
    new_round = 0;
  }

  tri->suppression_rate = data->supp_rate;
  // use rate at beginning of round for suppression to avoid
  // possible feedback implosion in case of rate reduction during the round
  if (new_round) tri->comparison_rate = tri->rate_;

  if (tri->representative && tri->tight_loop_) {
    // feedback report once per RTT
    if (now - tri->last_report_sent > tri->conservative_rtt) {
      tri->fb_time = 0;
      TfmccRcvSendPkt (agent, pkt);
    }
  } else {
    if (new_round) {
      tri->fb_time = now 
        + TfmccRcvFdbkTime (agent, tri->max_rtt, TFMCC_RCV_MAX_NUM_RECVS);
    }

    if (tri->fb_time > 0) {
      // reschedule packet in case last data packet was received too long ago
      // to prevent implosion (max. RTT should not be less than inter-packet
      // gap) (but don't reschedule if it's the first data packet)
      if (now - prev_last_arrival_ > tri->max_rtt) {
        tri->fb_time += now - prev_last_arrival_ - tri->max_rtt;
      }

      if (tri->comparison_rate >= tri->suppression_rate
          || tri->rate_ >= tri->suppression_rate) {
        // suppress higher rate feedback
        tri->fb_time = 0;
      } else {
        // send non-CLR feedback if it's not the first data packet and fb_time
        // passed (old feedback needs to be sent before checking for a new
        // round!)
        if (tri->rate_ > 0 && tri->fb_time <= now) {
          TfmccRcvSendPkt (agent, pkt);
          tri->fb_time = 0;
        }
      }
    }
  }  
}


void TfmccRcvAddPktToHistory (Agent * agent, Packet * p)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  TfmccData * data = (TfmccData *) & (p->type.td_);
  register int i; 
  register int seqno = data->seqno;
  double delta, tstamp;
  
  if (tri->lossvec_ == NULL) {
    // Initializing history.
    tri->rtvec_= (double *) malloc (sizeof (double) * tri->hsz);
    tri->tsvec_ = (double *) malloc (sizeof (double) * tri->hsz);
    tri->lossvec_= (char *) malloc (sizeof (char) * tri->hsz);
    tri->sizevec_ = (int *) malloc (sizeof (int) * tri->hsz);
    if (tri->rtvec_ && tri->lossvec_) {
      for (i = 0; i <= tri->maxseq; ++ i) {
        tri->lossvec_[i] = TFMCC_RCV_NOLOSS; 
        tri->rtvec_[i] = now;
        tri->tsvec_[i] = tri->last_timestamp_;
        tri->sizevec_[i] = tri->fairsize_;
      }
      for (i = tri->maxseq + 1; i < tri->hsz ; ++ i) {
        tri->lossvec_[i] = TFMCC_RCV_UNKNOWN;
        tri->rtvec_[i] = -1; 
        tri->tsvec_[i] = -1; 
        tri->sizevec_[i] = 0;
      }
    }
    else {
      printf ("TFMCC: error allocating memory for packet buffers\n");
      tw_exit (-1);
    }
  }

  if (data->seqno - tri->last_sample > tri->hsz) {
    printf ("TFMCC: time=%f, pkt=%d, last=%d history too small\n",
             now, data->seqno, tri->last_sample);
    tw_exit (-1);
  }

  /* for the time being, we will ignore out of order and duplicate 
     packets etc. */
  if (seqno > tri->maxseq) {
    tri->rtvec_[seqno % tri->hsz] = now;  
    tri->tsvec_[seqno % tri->hsz] = tri->last_timestamp_;  
    tri->lossvec_[seqno % tri->hsz] = TFMCC_RCV_RCVD;
    tri->sizevec_[seqno % tri->hsz] = p->size_;
    i = tri->maxseq + 1;
    if (i < seqno) {
      delta = (tri->tsvec_[seqno % tri->hsz] 
               - tri->tsvec_[tri->maxseq % tri->hsz])
               /(seqno - tri->maxseq) ; 
      tstamp = tri->tsvec_[tri->maxseq % tri->hsz] + delta ;
      while(i < seqno) {
        tri->rtvec_[i % tri->hsz] = now;
        tri->tsvec_[i % tri->hsz] = tstamp;
        tri->sizevec_[i % tri->hsz] = tri->avgsize_;

        if (tri->tsvec_[i % tri->hsz] - tri->lastloss > tri->conservative_rtt) {
          // Lost packets are marked as "LOST"
          // at most once per RTT.
          tri->lossvec_[i % tri->hsz] = TFMCC_RCV_LOST;
          tri->lastloss = tstamp;
        }
        else {
          // This lost packet is marked "NOLOSS"
          // because it does not begin a loss event.
          tri->lossvec_[i % tri->hsz] = TFMCC_RCV_NOLOSS; 
        }
        ++ i;
        tstamp = tstamp + delta;
      }
    }
    tri->maxseq = seqno;
  }
}


double TfmccRcvEstLoss (Agent * agent)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  int i;
  double ave_interval1, ave_interval2; 
  int ds ; 
  double ave, avg;
  int factor = 2;
  double ratio;
  double min_ratio;

  // sample[i] counts the number of packets since the i-th loss event
  // sample[0] contains the most recent sample.

  //packets (and possibly loss events) are aggregated to form fairsize_'ed
  // packets

  for (i = tri->last_sample; i <= tri->maxseq ; i ++) {
    if (tri->aggregate_packets_) {
      if (! tri->aggregate_loss_ 
          && tri->lossvec_[i % tri->hsz] == TFMCC_RCV_LOST) {
        // adjust sample if each loss event counts as full packet
        tri->sum_psize_ += tri->fairsize_;
      } else {
        tri->sum_psize_ += tri->sizevec_[i % tri->hsz];
      }

      if (tri->sum_psize_ >= 0) {
        // after received packet ignore the next packets until fairsize_ bytes
        // were received
        tri->sum_psize_ -= tri->fairsize_;
        ++ tri->sample[0]; 
      }
    } else {
      ++ tri->sample[0]; 
    }

    if (tri->lossvec_[i % tri->hsz] == TFMCC_RCV_NOLOSS
        || tri->lossvec_[i % tri->hsz] == TFMCC_RCV_LOST) {
      tri->sum_lsize_ += tri->sizevec_[i % tri->hsz];
    }

    if ((tri->aggregate_loss_ && tri->sum_lsize_ >= 0) ||
        (! tri->aggregate_loss_ 
         && tri->lossvec_[i % tri->hsz] == TFMCC_RCV_LOST)) {
      tri->sum_lsize_ -= tri->fairsize_;
      //  new loss event
      ++ tri->sample_count;
      TfmccRcvShiftArrayInt (tri->sample, tri->numsamples + 1, 0); 
      TfmccRcvMultArray (tri->mult, tri->numsamples + 1, tri->mult_factor_);
      TfmccRcvShiftArrayDbl (tri->mult, tri->numsamples + 1, 1.0); 
      tri->mult_factor_ = 1.0;
    }
  }
  tri->last_sample = tri->maxseq + 1 ; 

  if (tri->sample_count > tri->numsamples + 1)
    // The array of loss intervals is full.
    ds = tri->numsamples + 1;
  else
    ds = tri->sample_count;

  if (tri->sample_count == 1 && tri->false_sample == 0) {
    // no losses yet
    return 0;
  }

  /* do we need to discount weights? */
  if (tri->sample_count > 1 && tri->discount && tri->sample[0] > 0) {
    ave = TfmccRcvWeightedAvg
      (agent, 1, ds, 1.0, tri->mult, tri->weights, tri->sample);
    factor = 2;
    ratio = (factor * ave) / tri->sample[0];
    min_ratio = 0.5;
    if ( ratio < 1.0) {
      // the most recent loss interval is very large
      tri->mult_factor_ = ratio;
      if (tri->mult_factor_ < min_ratio) 
        tri->mult_factor_ = min_ratio;
    }
  }
  // Calculations including the most recent loss interval.
  ave_interval1 = TfmccRcvWeightedAvg
    (agent, 0, ds, tri->mult_factor_, tri->mult, tri->weights, tri->sample);
  // The most recent loss interval does not end in a loss
  // event.  Include the most recent interval in the 
  // calculations only if this increases the estimated loss
  // interval.
  ave_interval2 = TfmccRcvWeightedAvg
    (agent, 1, ds, tri->mult_factor_, tri->mult, tri->weights, tri->sample);
  if (ave_interval2 > ave_interval1)
    ave_interval1 = ave_interval2;
  if (ave_interval1 < 0.99) { 
    avg = 0;
    for (i = 0 ; i <= tri->numsamples; i++) {
      avg += tri->sample[i];
    }
    tw_exit (1);
  }
  
  return 1 / ave_interval1; 
}


double TfmccRcvWeightedAvg (Agent * agent, int start, int end, double factor,
  double *m, double *w, int *sample)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  int i; 
  double wsum = 0;
  double answer = 0;
  
  if (tri->smooth_ == 1 && start == 0) {
    if (end == tri->numsamples + 1) {
      // the array is full, but we don't want to uses
      //  the last loss interval in the array
      end = end - 1;
    } 
    // effectively shift the weight arrays 
    for (i = start; i < end; i++) 
      if (i == 0)
        wsum += m[i] * w[i+1];
      else 
        wsum += factor * m[i] * w[i+1];
    for (i = start ; i < end; i++)
      if (i==0)
         answer += m[i]*w[i+1]*sample[i]/wsum;
      else 
        answer += factor*m[i]*w[i+1]*sample[i]/wsum;
    return answer;
  } 
  
  for (i = start ; i < end; i++) 
    if (i==0)
      wsum += m[i]*w[i];
    else 
      wsum += factor*m[i]*w[i];
  for (i = start ; i < end; i++)  
    if (i==0)
       answer += m[i]*w[i]*sample[i]/wsum;
    else 
      answer += factor*m[i]*w[i]*sample[i]/wsum;
  return answer;
}


void TfmccRcvShiftArrayInt (int *a, int sz, int defval)
{
  int i ;
  for (i = sz-2 ; i >= 0 ; i--) {
    a[i+1] = a[i] ;
  }
  a[0] = defval;
}


void TfmccRcvShiftArrayDbl (double *a, int sz, double defval)
{
  int i ;
  for (i = sz-2 ; i >= 0 ; i--) {
    a[i+1] = a[i] ;
  }
  a[0] = defval;
}


void TfmccRcvMultArray (double *a, int sz, double multiplier)
{
  int i ;
  for (i = 1; i <= sz-1; i++) {
    double old = a[i];
    a[i] = old * multiplier ;
  }
}


double TfmccRcvEstThput (Agent * agent)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  double last = tri->rtvec_[tri->maxseq % tri->hsz]; 
  int rcvd = 0;
  int i = tri->maxseq;
  
  // count number of packets received in the last 2 RTTs (min. of 10 packets)
  while (i > 0 
         && (rcvd < 10 
             || (last - tri->rtvec_[i % tri->hsz] 
                 < 2 * tri->conservative_rtt))) {
    if (tri->lossvec_[i % tri->hsz] == TFMCC_RCV_RCVD) ++ rcvd; 
    -- i;
  }
  if ((last - tri->rtvec_[i % tri->hsz]) > 0) {
    return rcvd / (last - tri->rtvec_[i % tri->hsz]) * tri->avgsize_;
  }
  
  return 0;
}


double TfmccRcvAdjustHistory (Agent * agent, double ts)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  int i;
  double p;

  for (i = tri->maxseq; i >= 0 ; i--) {
    if (tri->lossvec_[i % tri->hsz] == TFMCC_RCV_LOST) {
      tri->lossvec_[i % tri->hsz] = TFMCC_RCV_NOLOSS; 
    }
  }
  tri->lastloss = ts; 
  // ignore previous loss history
  tri->last_sample = tri->maxseq + 1;

  // no need to divide throughput by 2 since its actual receive rate, 
  // not sending rate
  p = b_to_p (TfmccRcvEstThput (agent), 
    tri->conservative_rtt, 4 * tri->conservative_rtt, 
    tri->fairsize_, tri->bval_);
  tri->false_sample = (int) (1.0 / p + 0.5);
  tri->sample[1] = tri->false_sample;
  tri->sample[0] = 0;
  tri->sample_count = 2;
  tri->false_sample = -1 ; 
  tri->rtt_lossinit = tri->conservative_rtt;
  return p;
}


double TfmccRcvFdbkTime (Agent * agent, double rtt, int num_recv)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  double T = tri->fbtime_mult_ * rtt;
  double x = rand () / ((double) RAND_MAX);
  double t;

  t = T * (1.0 + log(x) / log((double) num_recv));
  if (t < 0) t = 0;
  return t;
}


void TfmccRcvSendPkt (Agent * agent, Packet * srcPkt)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  tw_lp * lp = tw_getlp (agent->nodeId_);
  double now = tw_now (lp);
  Packet p;
  TfmccNak * nak;
  
  if (! tri->active_) return;

  p.type_ = TFMCC_NAK;
  p.srcAddr_ = _nodeTable[agent->nodeId_].addr_;
  p.srcAgent_ = agent->id_;
  p.dstAddr_ = srcPkt->srcAddr_;
  p.dstAgent_ = srcPkt->srcAgent_;  
  p.size_ = TFMCC_RCV_NAK_PKT_SIZE;
  p.inPort_ = (uint16) -1;

  nak = (TfmccNak *) & (p.type.tn_);
  nak->timestamp = now;
  nak->timestamp_echo = tri->last_timestamp_;
  nak->timestamp_offset = now - tri->last_arrival_;
  nak->rate = tri->rate_;
  nak->round_id = tri->round_id;
  nak->have_rtt = (tri->rtt_ != 0);
  nak->have_loss = tri->loss_seen_yet;
  nak->receiver_leave = tri->receiver_leave;
  tri->last_report_sent = now; 
  tri->rcvd_since_last_report = 0;
  
  SendPacket (lp, &p);
  
  ++ tri->nakSent_;
  
  if (IsMcastAddr (agent->listenAddr_)) ++ _totalTfmccNak;
}


void TfmccRcvTimer (Agent * agent, int timerId, int serial)
{
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;
  
  if (! tri->active_) return;
}


void TfmccRcvPrintStat (Agent * agent)
{  
  TfmccRcvInfo * tri = (TfmccRcvInfo *) agent->info_;

  printf 
    ("TFMCC receiver at %d.%lu statistics: NAK sent = %lu, ",
    agent->nodeId_, GetGrpAddr4Print (agent->id_), tri->nakSent_);
    
  printf ("total NAK sent = %lu\n", _totalTfmccNak);
}
