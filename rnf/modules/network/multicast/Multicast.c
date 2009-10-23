#include "Multicast.h"
#include "Transport.h"

AgentHash _agentTable[AGENT_TABLE_SIZE];
char * _groupOpFileName = NULL;
StatAgentList * statAgentListHead_ = NULL, * statAgentListTail_ = NULL;

tw_lptype _mylps[] =
{
	{TW_MCAST, sizeof (NodeState),
	(init_f) McastStartUp,
	(event_f) McastEventHandler,
	(revent_f) McastRcEventHandler,
	(final_f) McastCollectStats,
	(statecp_f) NULL},
	{0},
};


void McastEventHandler(NodeState * sv, tw_bf * cv, EventMsg * em, tw_lp * lp)
{
  uint32 gid, ref = 0;
  int nid;
  MapUnitType * portMap;
  uint32 dstAddr;
  Packet * p;
  Agent * a;
  MapUnitType hiu;

  nid = lp->id;
  
  switch (em->type_) {
  case GROUP_OP:
    gid = em->content_.grpOp_.group_;
    switch (em->content_.grpOp_.op_) {
    case CREATE:
      AddGroup (gid, nid);
      break;
    case JOIN:
      JoinGroup (nid, gid);
      break;
    case LEAVE:
      LeaveGroup (nid, gid);
      break;
    default:
      printf ("Group operation error: Unknown group op type <%d> on lp %d.\n",
              (int) em->content_.grpOp_.op_, (int) nid);
      tw_exit (-1);
    }
    break;
  case DATA_PKT:
    dstAddr = em->content_.pkt_.dstAddr_;
    p = &(em->content_.pkt_);
    
    /* 
     * Check whether this node is the destination. If not, forward it only.
     * If yes, hand it to upper layer and forward it if necessary
     */  
    if (IsMcastAddr (dstAddr)) { /* Multicast address */
      if (p->id_) {
        ref = GetExtPktRef (p->srcAddr_, p->srcAgent_, p->id_);        
        -- ref;
      }
      
      if ((portMap = GetFwdPortMap (lp->id, dstAddr)) != NULL) {      
        if ((hiu = portMap[PORT_MAP_SIZE - 1]) & HIGHEST_BIT_TEST) {
          PassPktToUpperLayer (lp, p);
        }
              
        /* Forward it if necessary */
        portMap[PORT_MAP_SIZE - 1] &= ~(HIGHEST_BIT_TEST);
        
        if (! IsEmptyMap (portMap)) {
          ref += ForwardMulticastPkt (lp, p, portMap);
        }
        portMap[PORT_MAP_SIZE - 1] = hiu;
      }
      
      if (p->id_) {
        if (ref == 0) {
          DelExtPkt (p->srcAddr_, p->srcAgent_, p->id_);
        }
        else {
          SetExtPktRef (p->srcAddr_, p->srcAgent_, p->id_, ref);
        }
      }
      break;
    }
    
    /* Unicast address */
    if (dstAddr == _nodeTable[nid].addr_) {
      PassPktToUpperLayer (lp, p);
      if (p->id_) DelExtPkt (p->srcAddr_, p->srcAgent_, p->id_);
    }
    else {
      ForwardUnicastPkt (lp, p);
    }
    break;
  case START_AGENT:
    a = em->content_.agent_;
    a->startf_ (a);
    break;
  case STOP_AGENT:
    a = em->content_.agent_;
    a->stopf_ (a);
    break;
  case AGENT_TIMER:
    a = em->content_.timeOut_.agent_;
    a->timerf_ (a, em->content_.timeOut_.timerId_,
                em->content_.timeOut_.serial_);
    break;
  default:
    printf ("Unknown event message type %d.\n", em->type_);
    tw_exit (-1);
  }
}


void McastRcEventHandler(NodeState * sv, tw_bf * cv, EventMsg * em, tw_lp * lp)
{}


void McastStartUp(NodeState * sv, tw_lp *lp)
{
  if (lp->id != _nodeNum - 1) return;
  
  /* Initialization running for only once after all lp's have been init'ed*/
  ReadGroupOp (_groupOpFileName);
  free ((void *) _groupOpFileName);
  ReadAgentAct (_AgentActFileName);
  free ((void *) _AgentActFileName);
}


void McastCollectStats(NodeState * sv, tw_lp *lp)
{
  StatAgentList * sal, * sal2;

  if (lp->id != _nodeNum - 1) return;

  printf ("\n*** Agent statistics begins ***\n\n");

  for (sal = statAgentListHead_; sal != NULL;) {
    sal->agent_->statf_ (sal->agent_);
    sal2 = sal->next_;
    free ((void *) sal);
    sal = sal2;
  }

  printf ("\n*** Agent statistics ends ***\n\n");
}


/*
 * NOTE: group address is modified here by turning on the highest bit.
 */
void ReadGroupOp (char * fileName)
{  
  int fd, n;
  char buf[MAX_LINE_LEN];
  double t;
  int nid, act;
  uint32 gid;
  tw_lp * lp;
  tw_event * e;
  EventMsg * em;
  GroupOp * gop;
  
  /*
   * Read group operation of joining/leaving. Do it only once (at init of lp 0).
   */
  if ((fd = open (fileName, O_RDONLY)) < 0) {
    printf ("Failed to open the group operation file <%s>\n", fileName);
    exit (-1);
  }
  
  while (ReadLine (fd, buf) > 0) {
    if ((n = sscanf (buf, "%lf %d %d %d", 
                     &t, (int *) &nid, (int *) &gid, (int *) &act)) <= 0) {
      continue;
    }
    
    if (n != 4) {
      printf ("Group operation error: Wrong format. Line: <%s>\n", buf);
      exit (-1);
    }
    
    if (t < 0 || nid < 0 || gid < 0) {
      printf ("Group operation error: Invalid content. Line: <%s>\n", buf);
      exit (-1);
    }
    
    if (t >= g_tw_ts_end) continue;
    
    SetGrpAddr4Prog (gid);
    
    if (t <= 0) {  /* Process the operation now */
      switch (act) {
      case CREATE:
        AddGroup (gid, nid);
        break;
      case JOIN:
        JoinGroup (nid, gid);
        break;
      case LEAVE:
        LeaveGroup (nid, gid);
        break;
      default:
        printf ("Group operation error: Unknown group op type. Line: <%s>\n",
                buf);
        exit (-1);
      }
      continue;
    }
    
    /*
     * Future events
     */    
    if ((lp = tw_getlp (nid)) == NULL) {
      printf ("Group operation error: Unable to get lp for node. Line: <%s>\n",
               buf);
      exit (-1);
    }
            
    if ((e = tw_event_new (lp, t, lp)) == NULL) {
      printf ("Group operation error: Unable to create event. Line: <%s>\n",
              buf);
      exit (-1);
    }

    em = (EventMsg *) tw_event_data (e);
    em->type_ = GROUP_OP;
    gop = &(em->content_.grpOp_);
    gop->group_ = gid;
    gop->op_ = act;
    tw_event_send (e);
  }
  
  close (fd);
}


/*
 * An receiver agent in a multicast group use the multicast group address
 * (without the highest bit set) + 0x8000 as its agent ID. All other agents 
 * use the ID specified by 'agentId'.
 */ 
Agent * RegisterAgent (int nodeId, uint32 agentId, uint32 listenAddr, 
                       PktHandler h,
                       StartFunc startf, StopFunc stopf, TimerFunc timerf,
                       AgentFunc statf,
                       void * info)
{
  AgentHash * ah;
  Agent * a;
  int16 i, unused = -1;
  
  if (IsMcastAddr (listenAddr)) agentId = listenAddr;
  ah = _agentTable
       + ((nodeId + 1) * (agentId + 1) * HASH_DIST_FACTOR + agentId) 
         % AGENT_TABLE_SIZE;
  
  if (ah->head_ == ah->tail_) {  /* Nothing in this hash bucket yet */
    ah->tail_ = (ah->tail_ + 1) % HASH_BUCKET_DEPTH;
    a = ah->agent_ + ah->head_;
    memset ((void *) a, 0, sizeof (Agent));
    a->nodeId_ = nodeId;
    a->listenAddr_ = listenAddr;
    a->id_ = agentId ;
    a->used_ = 1;    
    a->handler_ = h;
    a->startf_ = startf;
    a->stopf_ = stopf;
    a->timerf_ = timerf;
    a->statf_ = statf;
    a->info_ = info;
    return a;
  }
  
  for (i = ah->head_; i != ah->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    a = ah->agent_ + i;
    if (! a->used_) {
      if (unused < 0) unused = i;
      continue;
    }    
    if (a->nodeId_ != nodeId || a->id_ != agentId) continue;

    printf ("Duplicate agents of %d:%u.\n", nodeId, (unsigned int) agentId);
    exit (-1);
  }    

  /* Cannot find the entry */
  if ((ah->tail_ + 1) % HASH_BUCKET_DEPTH == ah->head_ && unused < 0) {
    printf ("Agent table overflowed!\n");
    tw_exit (-1);
  }
  
  if (unused < 0) {
    a = ah->agent_ + ah->tail_;
    ah->tail_ = (ah->tail_ + 1) % HASH_BUCKET_DEPTH;
  }
  else {
    a = ah->agent_ + unused;
  }
  memset ((void *) a, 0, sizeof (Agent));
  a->nodeId_ = nodeId;
  a->listenAddr_ = listenAddr;
  a->id_ = agentId;
  a->used_ = 1;    
  a->handler_ = h;
  a->startf_ = startf;
  a->stopf_ = stopf;
  a->timerf_ = timerf;
  a->statf_ = statf;
  a->info_ = info;
  
  return a;
}


void RegisterAgentCopy (Agent * agent, uint32 listenAddr)
{
  AgentHash * ah;
  Agent * a;
  int16 i, unused = -1;
  uint32 agentId = listenAddr;
  
  ah = _agentTable
       + ((agent->nodeId_ + 1) * (agentId + 1) * HASH_DIST_FACTOR + agentId) 
         % AGENT_TABLE_SIZE;
  
  if (ah->head_ == ah->tail_) {  /* Nothing in this hash bucket yet */
    ah->tail_ = (ah->tail_ + 1) % HASH_BUCKET_DEPTH;
    a = ah->agent_ + ah->head_;
    memcpy ((void *) a, (void *) agent, sizeof (Agent));
    a->listenAddr_ = listenAddr;
    a->id_ = agentId ;
    return;
  }
  
  for (i = ah->head_; i != ah->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    a = ah->agent_ + i;
    if (! a->used_) {
      if (unused < 0) unused = i;
      continue;
    }    
    if (a->nodeId_ != agent->nodeId_ || a->id_ != agentId) continue;

    printf ("Duplicate agents of %d:%u.\n", agent->nodeId_, 
            (unsigned int) agentId);
    exit (-1);
  }    

  /* Cannot find the entry */
  if ((ah->tail_ + 1) % HASH_BUCKET_DEPTH == ah->head_ && unused < 0) {
    printf ("Agent table overflowed!\n");
    tw_exit (-1);
  }
  
  if (unused < 0) {
    a = ah->agent_ + ah->tail_;
    ah->tail_ = (ah->tail_ + 1) % HASH_BUCKET_DEPTH;
  }
  else {
    a = ah->agent_ + unused;
  }
  memcpy ((void *) a, (void *) agent, sizeof (Agent));
  a->listenAddr_ = listenAddr;
  a->id_ = agentId;
}


/*
 * If agentId < 0, it is not used for searching
 */
Agent * GetAgent (int nodeId, uint32 agentId)
{
  AgentHash * ah = _agentTable
                   + ((nodeId + 1) * (agentId + 1) * HASH_DIST_FACTOR + agentId)
                     % AGENT_TABLE_SIZE;
  Agent * a;
  int16 i;
  
  if (ah->head_ == ah->tail_) {  /* Nothing in this hash bucket yet */
    return NULL;
  }
  
  for (i = ah->head_; i != ah->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    a = ah->agent_ + i;
    if (! a->used_) continue;    
    if (a->nodeId_ == nodeId && a->id_ == agentId) {
      return a;
    }
  }
  
  return NULL;
}


void UnregisterAgent (int nodeId, uint32 agentId)
{
  AgentHash * ah = _agentTable
                   + ((nodeId + 1) * (agentId + 1) * HASH_DIST_FACTOR + agentId)
                     % AGENT_TABLE_SIZE;
  Agent * a;
  int16 i;
  
  for (i = ah->head_; i != ah->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    a = ah->agent_ + i;
    if (! a->used_) continue;
    if (a->nodeId_ != nodeId || a->id_ != agentId) {
      continue;
    }
    if (i == ah->head_) {
      ah->head_ = (i + 1) % HASH_BUCKET_DEPTH;
    }
    else if (i == ah->tail_) {
      ah->tail_ = (ah->tail_ + HASH_BUCKET_DEPTH - 1) % HASH_BUCKET_DEPTH;
    }
    else a->used_ = 0;
    if (a->info_) {
      free (a->info_);
      a->info_ = NULL;
    }
    return;
  }
}


void UnregisterAgentCopy (int nodeId, uint32 agentId)
{
  AgentHash * ah = _agentTable
                   + ((nodeId + 1) * (agentId + 1) * HASH_DIST_FACTOR + agentId)
                     % AGENT_TABLE_SIZE;
  Agent * a;
  int16 i;
  
  for (i = ah->head_; i != ah->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    a = ah->agent_ + i;
    if (! a->used_) continue;
    if (a->nodeId_ != nodeId || a->id_ != agentId) {
      continue;
    }
    if (i == ah->head_) {
      ah->head_ = (i + 1) % HASH_BUCKET_DEPTH;
    }
    else if (i == ah->tail_) {
      ah->tail_ = (ah->tail_ + HASH_BUCKET_DEPTH - 1) % HASH_BUCKET_DEPTH;
    }
    else a->used_ = 0;
    return;
  }
}


int ForwardPacket (tw_lp * srcLp, tw_lp * dstLp, Packet * p, tw_stime t)
{
  tw_event * e;
  EventMsg * em;
  double now = tw_now (dstLp);
  
  if (t + now >= g_tw_ts_end) return 0;  /* Otherwise the program will exit when
                                          * creating event */
  
#ifdef DEBUG_PKT_FWD
  printf ("%lf : pkt(%d:%lu) from %d to %d (in-port %d) at %lf\n", 
          tw_now (srcLp), _addrToIdx[p->srcAddr_],
          p->od_.sqn_, srcLp->id, dstLp->id, p->inPort_, now + t);
  fflush (stdout);
#endif  
  
  if ((e = tw_event_new (dstLp, t, dstLp)) == NULL) {
    printf ("Failed to create event for packet.\n");
    tw_exit (-1);
  }
  
  (em = (EventMsg *) tw_event_data (e))->type_ = DATA_PKT;  
  memcpy ((void *) &(em->content_.pkt_), (void *) p, sizeof (Packet));
  
  tw_event_send (e);
  
  return 1;
}


/*
 * @return 1 if forwarded, 0 if discarded
 */
int ForwardPktOnLink (tw_lp * lp, Link * lnk, Packet * p)
{  
  double t, now = tw_now (lp);

  if (lnk->lastPktOutTime_ <= now) {  
    /* No packet is being sent out. Therefore forward it right now */
    t = p->size_ * 8 / ((double) lnk->bw_); /* Transmission delay */
    lnk->lastPktOutTime_ = now + t;
    p->inPort_ = lnk->nbPort_; /* This will copied to the outgoing packet */
    return ForwardPacket (lp, tw_getlp (lnk->neighbour_), p, t + lnk->latency_);
  }
      
  if (p->size_ + (lnk->lastPktOutTime_ - now) / 8 * lnk->bw_ 
      > lnk->bufSize_ + SMALL_FLOAT) {
    /* Buffer is full. Discard the packet
     * The buffered packets include the one being sent except the portion
     * that has been sent. */

#ifdef DEBUG_PKT_DISCARD
    printf ("%lf : pkt(%d:%lu) at %d discarded\n",
            tw_now (lp), _addrToIdx[p->srcAddr_], p->od_.sqn_, lp->id);
    fflush (stdout);
#endif    
  
    return 0;
  }
  
  /* The buffer is not zero, some packet is being sent.
   * Schedule to forward this packet at a future time */
  p->inPort_ = lnk->nbPort_; /* This will copied to the outgoing packet */
  lnk->lastPktOutTime_ += p->size_ * 8 / ((double) lnk->bw_);    
  return ForwardPacket (lp, tw_getlp (lnk->neighbour_), p, 
                        lnk->lastPktOutTime_ + lnk->latency_ - now);
}


/*
 * @return # forwarded packets 
 */
int ForwardMulticastPkt (tw_lp * lp, Packet * p, MapUnitType * portMap)
{
  Node * node = _nodeTable + lp->id;
  Link ** lnks = node->links_, * lnk;
  uint16 i, linkNum = node->linkNum_, inPort = p->inPort_, fwd = 0;
  
  for (i = 0; i < linkNum; ++ i) {
    if (portMap[i / MAP_UNIT_SIZE] == 0) {
      i += MAP_UNIT_SIZE - 1;  /* -1 because there is ++ i */
      continue;
    }
    
    if (! IsBitSet (portMap, i)) continue;
    
    /* This is the port where the packet comes in */
    if (i == inPort) continue; 
    
    p->inPort_ = (lnk = lnks[i])->nbPort_; /* This will be copied to 
                                            * the outgoing packet */
    fwd += ForwardPktOnLink (lp, lnk, p);
  }
  
  return fwd;
}


int ForwardUnicastPkt (tw_lp * lp, Packet * p)
{
  Link * lnk = GetFwdLink (lp->id, p->dstAddr_);
  
  if (lnk == NULL) {
    printf ("Failed to forward packet to %u on %d.\n",
      (unsigned int) p->dstAddr_, lp->id);
    tw_exit (-1);
  }
  
  return ForwardPktOnLink (lp, lnk, p);
}  
  
  
void PassPktToUpperLayer (tw_lp * lp, Packet * p)
{
  Agent * a;
  
  if (IsMcastAddr (p->dstAddr_)) {
    a = GetAgent (lp->id, p->dstAddr_);
    if (a == NULL) {
      printf ("Node %d: no agent found for group %ld.\n", 
              lp->id, GetGrpAddr4Print (p->dstAddr_));
      tw_exit (-1);
    }
  }
  else {
    a = GetAgent (lp->id, (uint32) p->dstAgent_);
    if (a == NULL) {
      printf ("Node %d: no agent with id <%d> found.\n", 
              lp->id, p->dstAgent_);
      tw_exit (-1);
    }
  }  

  if (a->handler_) a->handler_ (a, p);
}
