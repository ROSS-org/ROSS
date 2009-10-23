#ifndef ROSS_MCAST_PACKET_H
#define ROSS_MCAST_PACKET_H

#include "Routing.h"

#define EXT_PACKET_TABLE_SIZE               10111

/* Packet Type */
enum {
  ORMCC_DATA,
  ORMCC_CI,
  GMCC_DATA,
  GMCC_CTRL,
  GMCC_CI,
  PGMCC_DATA,
  PGMCC_ACK,
  PGMCC_NAK,
  TFMCC_DATA,
  TFMCC_NAK
};


typedef struct OrmccDataStruct {
  uint32 sqn_;
  double ts_;     /* Time stamp */
  uint32 crAddr_;
  double crTracAvg_, crTracDev_;
  double maxRtt_;
} OrmccData;


typedef struct OrmccCiStruct {
  uint32 sqn_;    /* Same as the data packet sqn */
  double tsEcho_; /* Time stamp echo */
  double trac_;
} OrmccCi;


typedef struct GmccDataStruct {
  uint32 sqn_;                      /* Sequence number */
  double timestamp_;                /* Timestamp */
  double rate_;                     /* Sending rate */
  int16  layer_;                    /* Layer */
  int8  oversend_;                  /* If 1, it is a packet sent at a non-cut
                                     * rate at congestion */
} GmccData;


#define GMCC_MAX_LAYER_NUM          16


typedef struct GmccCtrlStruct {
} GmccCtrl;


typedef struct GmccCtrlExtStruct {
  double timestamp_;                  /* Timestamp */
  double crTafAvg_[GMCC_MAX_LAYER_NUM], crTafDev_[GMCC_MAX_LAYER_NUM];
                                      /* Avg. & dev TAFs of CRs in each layer */
  double srtt_[GMCC_MAX_LAYER_NUM];
  int8  activeLayer_[GMCC_MAX_LAYER_NUM];  /* Indicate which layer is active,
                                            * i.e. has data in transmission */
  uint32 grpAddrs_[GMCC_MAX_LAYER_NUM];    /* Group addresses for layers */
  uint32 cr_[GMCC_MAX_LAYER_NUM];          /* CR of every layer */
  double grttAvg_, grttDev_;
  int16  layerNum_;
  int16  layer_;                      /* Indicate which layer statistics has
                                       * changed */
} GmccCtrlExt;


typedef struct GmccCiStruct {
  double    tsEcho_;   // timestamp echo
  double    tafAvg_, tafDev_;
  double    rcvRate_;  // receiving rate
  int       layer_;    // layer number 
  short     keepalive_;// claim the aliveness of the CR
  short     mature_;   // whether the receiver has join result
                       // used only if the receiver sends a keepalive packet 
                       // for the top active layer it knows
} GmccCi;


typedef struct PgmccDataStruct {
  uint32 sqn_;
  uint32 ackerAddr_;
} PgmccData;


typedef struct PgmccAckStruct {
  uint32 rxwLead_;
  uint32 bitmask_;
  double rxLoss_;
} PgmccAck;


typedef struct PgmccNakStruct {
  uint32 rxwLead_;
  double rxLoss_;
} PgmccNak;


typedef struct TfmccDataStruct {
  int seqno;                // data sequence number
  double supp_rate;         // suppression rate
  double timestamp;         // time this message was sent
  double timestamp_echo;    // echo timestamp from last receiver report
  double timestamp_offset;  // offset since sender received report
  double max_rtt;           // maximum RTT of all receivers
  int round_id;
  int is_clr;               // current representative receiver
  int rtt_recv_id;          // id of receiver whose timestamp is sent back
} TfmccData;


typedef struct TfmccNakStruct {
  double timestamp;         // time this report was sent
  double timestamp_echo;    // echo timestamp from last data packet
  double timestamp_offset;  // offset since receiver got last packet
  double rate;              // expected sending rate (eqn)
  int round_id;             // round id echo
  int have_rtt;             // receiver has at least one RTT measurement
  int have_loss;            // receiver exprienced at least one loss event
  int receiver_leave;       // receiver will leave session
} TfmccNak;


typedef struct PacketStruct {
  int16 type_;
  uint32 id_;                  /* If not 0, it has extended part */
  uint32 dstAddr_, srcAddr_;   /* Like IP */
  uint16 dstAgent_, srcAgent_;  /* Like TCP port */
  int size_;      /* in bytes */
  uint16 inPort_; /* The port of the next node this packet will come in
                   * through */
  union {
    OrmccData od_;
    OrmccCi oc_;
    GmccData gd_;
    GmccCtrl gct_;
    GmccCi   gci_;
    PgmccData pd_;
    PgmccAck pa_;
    PgmccNak pn_;
    TfmccData td_;
    TfmccNak tn_;
  }type;
} Packet; /* Used for network traffic */


typedef struct ExtPacketStruct {
  uint32 used_;     /* Also serves as reference number count */
  uint32 srcAddr_, srcAgent_, id_;
  union {
    GmccCtrlExt gce_;
  } data_;
} ExtPacket;


typedef struct ExtPacketHashStruct {
  int16 head_, tail_;
  ExtPacket ep_[HASH_BUCKET_DEPTH];
} ExtPacketHash;


extern ExtPacketHash _extPktTable[];

void InitExtPktTable ();
void AddExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId, void * data,
                int dataSize, uint32 ref);
inline void SetExtPktRef (uint32 srcAddr, uint32 srcAgent, uint32 pktId, 
                          uint32 ref);
inline uint32 GetExtPktRef (uint32 srcAddr, uint32 srcAgent, uint32 pktId);
inline void DelExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId);
inline void * GetExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId);

#endif
