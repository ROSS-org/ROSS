#include "Packet.h"
#include "ross.h"


ExtPacketHash _extPktTable[EXT_PACKET_TABLE_SIZE];


void InitExtPktTable ()
{
  memset ((void *) _extPktTable, 0, 
          sizeof (ExtPacketHash) * EXT_PACKET_TABLE_SIZE);
}


void AddExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId, void * data,
                int dataSize, uint32 ref)
{
  ExtPacketHash * eph = _extPktTable 
                        + (srcAddr + 1) * (srcAgent + 1) * (pktId + 1)
                          * HASH_DIST_FACTOR % EXT_PACKET_TABLE_SIZE;
  ExtPacket * ep;
  int16 i, unused = -1;
  
  if (eph->head_ == eph->tail_) {  /* Nothing in this hash bucket yet */
    eph->tail_ = (eph->tail_ + 1) % HASH_BUCKET_DEPTH;
    ep = eph->ep_ + eph->head_;
    ep->srcAddr_ = srcAddr;
    ep->srcAgent_ = srcAgent;
    ep->id_ = pktId;
    memcpy ((void *) & (ep->data_), data, dataSize);
    ep->used_ = ref;    
    return;
  }
  
  for (i = eph->head_; i != eph->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    ep = eph->ep_ + i;
    if (! ep->used_) {
      if (unused < 0) unused = i;
      continue;
    }    
    if (ep->srcAddr_ != srcAddr || ep->srcAgent_ != srcAgent
        || ep->id_ != pktId) continue;
    printf ("Duplicate extended packet. %lu, %lu, %lu\n",
      srcAddr, srcAgent, pktId);    
    tw_exit (-1);
  }    

  /* Cannot find the entry */
  if ((eph->tail_ + 1) % HASH_BUCKET_DEPTH == eph->head_ && unused < 0) {
    printf ("Extened packet table overflowed!\n");
    tw_exit (-1);
  }
  
  if (unused < 0) {
    ep = eph->ep_ + eph->tail_;
    eph->tail_ = (eph->tail_ + 1) % HASH_BUCKET_DEPTH;
  }
  else {
    ep = eph->ep_ + unused;
  }
  ep->used_ = ref;
  ep->srcAddr_ = srcAddr;
  ep->srcAgent_ = srcAgent;
  ep->id_ = pktId;
  memcpy ((void *) & (ep->data_), data, dataSize);
}


uint32 GetExtPktRef (uint32 srcAddr, uint32 srcAgent, uint32 pktId)
{
  ExtPacketHash * eph = _extPktTable 
                        + (srcAddr + 1) * (srcAgent + 1) * (pktId + 1)
                          * HASH_DIST_FACTOR % EXT_PACKET_TABLE_SIZE;
  ExtPacket * ep;
  int16 i;

  for (i = eph->head_; i != eph->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    ep = eph->ep_ + i;    
    if (ep->srcAddr_ != srcAddr || ep->srcAgent_ != srcAgent
        || ep->id_ != pktId) continue;
    return ep->used_;
  }
  
  printf ("Failed to get reference for extended pkt. %lu, %lu, %lu\n",
    srcAddr, srcAgent, pktId);
  tw_exit (-1);
  
  return 0;
}


void SetExtPktRef (uint32 srcAddr, uint32 srcAgent, uint32 pktId, uint32 ref)
{
  ExtPacketHash * eph = _extPktTable 
                        + (srcAddr + 1) * (srcAgent + 1) * (pktId + 1)
                          * HASH_DIST_FACTOR % EXT_PACKET_TABLE_SIZE;
  ExtPacket * ep;
  int16 i;

  for (i = eph->head_; i != eph->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    ep = eph->ep_ + i;    
    if (ep->srcAddr_ != srcAddr || ep->srcAgent_ != srcAgent
        || ep->id_ != pktId) continue;
    ep->used_ = ref;
    return;
  }
  
  printf ("Failed to add reference for extended pkt. %lu, %lu, %lu\n",
    srcAddr, srcAgent, pktId);
  tw_exit (-1);
} 
 

void DelExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId)
{
  ExtPacketHash * eph = _extPktTable 
                        + (srcAddr + 1) * (srcAgent + 1) * (pktId + 1)
                          * HASH_DIST_FACTOR % EXT_PACKET_TABLE_SIZE;
  ExtPacket * ep;
  int16 i;

  for (i = eph->head_; i != eph->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    ep = eph->ep_ + i;    
    if (ep->srcAddr_ != srcAddr || ep->srcAgent_ != srcAgent
        || ep->id_ != pktId || ! ep->used_) continue;
    if (i == eph->head_) {
      eph->head_ = (i + 1) % HASH_BUCKET_DEPTH;
    }
    else if (i == eph->tail_) {
      eph->tail_ = (eph->tail_ + HASH_BUCKET_DEPTH - 1) % HASH_BUCKET_DEPTH;
    }
    else ep->used_ = 0;    
    return;
  }
}


void * GetExtPkt (uint32 srcAddr, uint32 srcAgent, uint32 pktId)
{
  ExtPacketHash * eph = _extPktTable 
                        + (srcAddr + 1) * (srcAgent + 1) * (pktId + 1)
                          * HASH_DIST_FACTOR % EXT_PACKET_TABLE_SIZE;
  ExtPacket * ep;
  int16 i;

  for (i = eph->head_; i != eph->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    ep = eph->ep_ + i;    
    if (ep->srcAddr_ != srcAddr || ep->srcAgent_ != srcAgent
        || ep->id_ != pktId || ! ep->used_) continue;
    return (void *) & (ep->data_);        
  }
  return NULL;
}
