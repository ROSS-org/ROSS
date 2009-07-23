#ifndef ROSS_MCAST_TFMCC_SINK_H
#define ROSS_MCAST_TFMCC_SINK_H

#include "Transport.h"


#define LARGE_DOUBLE 9999999999.99 

/* packet status */
enum {
  TFMCC_RCV_UNKNOWN = 0,
  TFMCC_RCV_RCVD = 1,
  TFMCC_RCV_LOST = 2,
  TFMCC_RCV_NOLOSS = 3
};

#define TFMCC_RCV_DEFAULT_NUMSAMPLES        8 
#define TFMCC_RCV_MAX_NUM_RECVS             10000


#define TFMCC_RCV_NAK_PKT_SIZE              40

typedef struct TfmccRcvInfoStruct {
  int8 active_;
  
  int psize_;               // size of received packet
  int fairsize_;            // packet size of competing flow
  int avgsize_;             // avg size of sent packets (EWMA)
  int sum_psize_;
  int sum_lsize_;
  int aggregate_packets_;   // bool
  int aggregate_loss_;      // bool
  double rtt_;              // smoothed measured rtt
  double rtt_est;   // use adjusted one-way delay to continue to modify the rtt
  double rtt_lossinit;      // RTT when history was initialized
  double max_rtt;           // max RTT (from sender)
  double conservative_rtt;
  double one_way_;          // one-way delay from receiver to sender
  int smooth_;              // for the smoother method for incorporating
                            //  incorporating new loss intervals

  // these assist in keep track of incming packets and calculate flost_
  double last_timestamp_, last_arrival_;
  int hsz;
  char *lossvec_;
  int  *sizevec_;
  double *rtvec_;
  double *tsvec_;
  int round_id ;
  int numsamples ;
  int *sample;
  double *weights ;
  double *mult ;
  double mult_factor_;      // most recent multiple of mult array
  int sample_count ;
  int last_sample ;  
  int maxseq;               // max seq number seen
  int total_received_;      // total # of pkts rcvd by rcvr
  int bval_;                // value of B used in the formula
  double last_report_sent;  // when was last feedback sent
  int rcvd_since_last_report;   // # of packets rcvd since last report
  double lastloss;          // when last loss occured
  int tight_loop_;          // flag for tight feedback loop to rep recv
  double fbtime_mult_;      // T = mult * RTT
  double fb_time;           // delay for feedback suppression
  double constant_rate_;    // set to positive value to use instead of
                            // calculated rate (Bit/s)
  double comparison_rate;   // rate at beginning of feedback round (for
                            // suppression)
  double suppression_rate;  // sender's suppression rate
  int receiver_leave;       // flag to indicate that CLR will leave group

  // these are for "faking" history after slow start
  int loss_seen_yet;            // have we seen the first loss yet?
  int adjust_history_after_ss;  // fake history after slow start? (0/1)
  int false_sample;             // by how much?

  int discount ;            // emphasize most recent loss interval
                            //  when it is very large
  int representative;       // flag: 1 = feedback receiver
  double df_;               // decay factor for accurate RTT estimate
  double rate_;
  
  uint32 nakSent_;
} TfmccRcvInfo;


Agent * TfmccRcvInit (int nodeId, uint16 agentId, uint32 listenAddr);
void TfmccRcvStart (Agent * agent);
void TfmccRcvStop (Agent * agent);
void TfmccRcvPktHandler (Agent * agent, Packet * p);
void TfmccRcvSendPkt (Agent * agent, Packet * srcPkt);
void TfmccRcvTimer (Agent * agent, int timerId, int serial);
void TfmccRcvPrintStat (Agent * agent);

void TfmccRcvAddPktToHistory (Agent * agent, Packet * p);
double TfmccRcvEstLoss (Agent * agent);
double TfmccRcvWeightedAvg 
  (Agent * agent, int start, int end, double factor, double *m, 
   double *w, int *sample);
inline void TfmccRcvShiftArrayInt (int *a, int sz, int defval);
inline void TfmccRcvShiftArrayDbl (double *a, int sz, double defval);
inline void TfmccRcvMultArray (double *a, int sz, double multiplier);
inline double TfmccRcvEstThput (Agent * agent);
double TfmccRcvAdjustHistory (Agent * agent, double ts); 
  
inline double TfmccRcvFdbkTime (Agent * agent, double rtt, int num_recv);
#endif
