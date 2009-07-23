#include "Routing.h"

MRouteHash _mrouteTable[MROUTE_TABLE_SIZE];
GroupInfoHash _groupTable[GROUP_TABLE_SIZE];

void InitMRoute ()
{
  memset ((void *) _mrouteTable, 0, sizeof (MRouteHash) * MROUTE_TABLE_SIZE);
  memset ((void *) _groupTable, 0, sizeof (GroupInfoHash) * GROUP_TABLE_SIZE);
}


/*
 * Assume a join message arrives at the router from port 'inPort',
 * update the multicast routing table.
 * @return 0 if this router did not have routing info for this group.
 *         1 if it did.
 */
int AddMRoute (int lpId, uint32 groupId, uint16 inPort)
{
  MRouteHash * mrh = _mrouteTable 
                     + ((lpId + 1) * (groupId + 1) * HASH_DIST_FACTOR + groupId)
                       % MROUTE_TABLE_SIZE;
  MRoute * mr;
  int16 i, unused = -1;
  
  #ifdef ROUTING_DEBUG
  printf ("Add mroute: lpid = %d, group = %lu, port = %d\n", 
    lpId, groupId, inPort);
  #endif
  
  if (mrh->head_ == mrh->tail_) {  /* Nothing in this hash bucket yet */
    mrh->tail_ = (mrh->tail_ + 1) % HASH_BUCKET_DEPTH;
    mr = mrh->route_ + mrh->head_;
    mr->lpId_ = lpId;
    mr->groupId_ = groupId;
    mr->used_ = 1;    
    SetBit (mr->portMap_, inPort);
    return 0;
  }
  
  for (i = mrh->head_; i != mrh->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    mr = mrh->route_ + i;
    if (! mr->used_) {
      if (unused < 0) unused = i;
      continue;
    }    
    if (mr->lpId_ != lpId || mr->groupId_ != groupId) continue;
    SetBit (mr->portMap_, inPort);

    return 1;
  }    

  /* Cannot find the entry */
  if ((mrh->tail_ + 1) % HASH_BUCKET_DEPTH == mrh->head_ && unused < 0) {
    printf ("Multicast routing table overflowed!\n");
    exit (-1);
  }
  
  if (unused < 0) {
    mr = mrh->route_ + mrh->tail_;
    mrh->tail_ = (mrh->tail_ + 1) % HASH_BUCKET_DEPTH;
  }
  else {
    mr = mrh->route_ + unused;
  }
  mr->used_ = 1;
  mr->lpId_ = lpId;
  mr->groupId_ = groupId;
  SetBit (mr->portMap_, inPort);
  
  return 0;
}


/*
 * @return 0 if this router does not have routing info for this group any more.
 *         1 if it does.
 */
int DelMRoute (int lpId, uint32 groupId, uint16 inPort)
{
  MRouteHash * mrh = _mrouteTable 
                     + ((lpId + 1) * (groupId + 1) * HASH_DIST_FACTOR + groupId)
                       % MROUTE_TABLE_SIZE;
  MRoute * mr;
  int16 i;
  
  #ifdef ROUTING_DEBUG
  printf ("Del mroute: lpid = %d, group = %lu, port = %d\n", 
    lpId, groupId, inPort);
  #endif    
  
  for (i = mrh->head_; i != mrh->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    mr = mrh->route_ + i;    
    if (mr->lpId_ != lpId || mr->groupId_ != groupId || ! mr->used_) continue;
    ResetBit (mr->portMap_, inPort);
    if (IsEmptyMap (mr->portMap_)) {
      if (i == mrh->head_) {
        mrh->head_ = (i + 1) % HASH_BUCKET_DEPTH;
      }
      else if (i == mrh->tail_) {
        mrh->tail_ = (mrh->tail_ + HASH_BUCKET_DEPTH - 1) % HASH_BUCKET_DEPTH;
      }
      else mr->used_ = 0;
      
      #ifdef ROUTING_DEBUG
      printf ("\tused_ = %d, %d, %d\n", (int) mr->used_, mrh->head_,
              mrh->tail_);
      #endif
      return 0;
    }
    return 1;
  }
  
  return 0;
}


/*
 * For multicast.
 * @return NULL if not found.
 */
MapUnitType * GetFwdPortMap (int lpId, uint32 groupId)
{
  MRouteHash * mrh = _mrouteTable 
                     + ((lpId + 1) * (groupId + 1) * HASH_DIST_FACTOR + groupId)
                       % MROUTE_TABLE_SIZE;
  MRoute * mr;
  int16 i;

  for (i = mrh->head_; i != mrh->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    mr = mrh->route_ + i;
    if (! mr->used_) continue;    
    if (mr->lpId_ != lpId || mr->groupId_ != groupId) continue;
    return mr->portMap_;
  }
  
  return NULL;
}


/*
 * Get the forward link of the router for the destination address 'destAddr'
 * for unicast.
 * @return the link.
 */
Link * GetFwdLink (int routerIdx, uint32 destAddr)
{
  Node * node = _nodeTable + routerIdx, * nbNode;
  Link * lnk = node->firstLink_, * upLink = node->link.upLink_;
  
  if (lnk == NULL) {
    printf ("Isolated node <%d> found.\n", routerIdx);
    exit (-1);
  }
  
  /* This node is the destination */
  if (destAddr == node->addr_) return NULL;
  
  /* Destination not in downstream tree, use default link */
  if (destAddr < node->lowAddr_ || destAddr > node->highAddr_) {
    return upLink;
  }

  for (; lnk != NULL; lnk = lnk->next_) {
    if (! lnk->active_ || ! lnk->route_ || lnk == upLink) continue;
    nbNode = _nodeTable + lnk->neighbour_;
    if (nbNode->lowAddr_ <= destAddr && destAddr <= nbNode->highAddr_) {
      return lnk;
    }
  };
  
  return NULL;
}


void AddGroup (uint32 groupId, int srcId)
{
  GroupInfoHash * gih = _groupTable
                        + (groupId + 1) * HASH_DIST_FACTOR % GROUP_TABLE_SIZE;
  GroupInfo * gi;
  int16 i, unused = -1;

  if (gih->head_ == gih->tail_) {  /* Nothing in this hash bucket yet */
    gih->tail_ = (gih->tail_ + 1) % HASH_BUCKET_DEPTH;
    gi = gih->gi_ + gih->head_;
    gi->srcId_ = srcId;
    gi->groupId_ = groupId;
    gi->used_ = 1;    

    return;
  }
  
  for (i = gih->head_; i != gih->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    gi = gih->gi_ + i;
    if (! gi->used_) {
      if (unused < 0) unused = i;
      continue;
    }    
    if (gi->groupId_ != groupId) continue;

    if (gi->srcId_ != srcId) {
      printf ("Different sources have been assigned to the same group.\n");
      exit (-1);
    }
    return;
  }    

  /* Cannot find the entry */
  if ((gih->tail_ + 1) % HASH_BUCKET_DEPTH == gih->head_ && unused < 0) {
    printf ("Multicast group table overflowed!\n");
    exit (-1);
  }
  
  if (unused < 0) {
    gi = gih->gi_ + gih->tail_;
    gih->tail_ = (gih->tail_ + 1) % HASH_BUCKET_DEPTH;
  }
  else {
    gi = gih->gi_ + unused;
  }
  gi->used_ = 1;
  gi->srcId_ = srcId;
  gi->groupId_ = groupId;
}


/*
 * @return -1 if not found.
 */
int GetGroupSrcId (uint32 groupId)
{
  GroupInfoHash * gih = _groupTable
                        + (groupId + 1) * HASH_DIST_FACTOR % GROUP_TABLE_SIZE;
  GroupInfo * gi;
  int16 i;
  
  if (gih->head_ == gih->tail_) {  /* Nothing in this hash bucket yet */
    return -1;
  }
  
  for (i = gih->head_; i != gih->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    if (! (gi = gih->gi_ + i)->used_) continue;
    if (gi->groupId_ == groupId) return gi->srcId_;
  }

  return -1;
}


void DelGroup (uint32 groupId)
{
  GroupInfoHash * gih = _groupTable
                        + (groupId + 1) * HASH_DIST_FACTOR % GROUP_TABLE_SIZE;
  GroupInfo * gi;
  int16 i;
    
  for (i = gih->head_; i != gih->tail_; i = (i + 1) % HASH_BUCKET_DEPTH) {
    gi = gih->gi_ + i;    
    if (gi->groupId_ != groupId || ! gi->used_) continue;
    if (i == gih->head_) {
      gih->head_ = (i + 1) % HASH_BUCKET_DEPTH;
    }
    else if (i == gih->tail_) {
      gih->tail_ = (gih->tail_ + HASH_BUCKET_DEPTH - 1) % HASH_BUCKET_DEPTH;
    }
    else gi->used_ = 0;
    return;
  }
}


uint32 AllocGroupAddr ()
{
  uint32 addr;
  
  for (;;) {
    if ((addr = rand ()) == (uint32) -1) continue;
    addr |= 0x80000000L;
    if (GetGroupSrcId (addr) != (uint32) -1) continue;
    break;
  }
  
  return addr;
}


/*
 * @param thisAddr: the address of the node which wants to join the group
 * @param srcAddr: the address of the group source
 */
void JoinGroup (int routerIdx, uint32 groupId)
{
  int srcAddr;
  int srcIdx;
  Link * lnk;
  
  if ((srcIdx = GetGroupSrcId (groupId)) < 0) {
    printf ("No source specified for group %d.\n", 
            (int) GetGrpAddr4Print (groupId));
    exit (-1);
  }  
  
  srcAddr = _nodeTable[srcIdx].addr_;
  
  /* This router had routing info for this group before. So don't need to 
   * follow up */
  if (AddMRoute (routerIdx, groupId, HIGHEST_MAP_BIT_POS)) {
    return;
  }

  lnk = GetFwdLink (routerIdx, srcAddr);
  while (lnk != NULL) {
    routerIdx = lnk->neighbour_;
    if (AddMRoute (routerIdx, groupId, lnk->nbPort_)) return;
    lnk = GetFwdLink (routerIdx, srcAddr);
  }
}


/*
 * @param thisAddr: the address of the node which wants to join the group
 * @param srcAddr: the address of the group source
 */
void LeaveGroup (int routerIdx, uint32 groupId)
{
  int srcAddr;
  int srcIdx;
  Link * lnk;;
  
  if ((srcIdx = GetGroupSrcId (groupId)) < 0) {
    printf ("No source specified for group %d.\n", 
            (int) GetGrpAddr4Print (groupId));
    exit (-1);
  }  

  srcAddr = _nodeTable[srcIdx].addr_;

  if (DelMRoute (routerIdx, groupId, HIGHEST_MAP_BIT_POS)) {
    return;
  }

  lnk = GetFwdLink (routerIdx, srcAddr);
  while (lnk != NULL) {
    routerIdx = lnk->neighbour_;
    if (DelMRoute (routerIdx, groupId, lnk->nbPort_)) {
      return;
    }
    lnk = GetFwdLink (routerIdx, srcAddr);
  }
}


/*
 * Check whether a bit in the bit map is set
 * @return 1 if set. -1 on error. 0 if unset.
 */
int IsBitSet (MapUnitType * map, uint16 pos)
{
  uint16 i;
  
  if (pos > MAP_UNIT_SIZE * PORT_MAP_SIZE) return -1;
  i = pos / MAP_UNIT_SIZE;
  return ((map[i] >> (pos - i * MAP_UNIT_SIZE)) & 0x0001) ? 1 : 0;  
}


void SetBit (MapUnitType * map, uint16 pos)
{
  MapUnitType u = 0x1;
  uint16 i;
  
  if (pos > MAP_UNIT_SIZE * PORT_MAP_SIZE) {
    printf ("Bit position out of range!\n");
    exit (-1);
  }
  i = pos / MAP_UNIT_SIZE;
  map[i] |= (u << (pos - i * MAP_UNIT_SIZE));

  #ifdef ROUTING_DEBUG
  printf ("\t%08X (set)\n", * ((int *) map));  
  #endif
}


void ResetBit (MapUnitType * map, uint16 pos)
{
  MapUnitType u = 0x01;
  uint16 i;
  
  if (pos > MAP_UNIT_SIZE * PORT_MAP_SIZE) {
    printf ("Bit position out of range!\n");
    exit (-1);
  }
  i = pos / MAP_UNIT_SIZE;
  map[i] &= ~(u << (pos - i * MAP_UNIT_SIZE));

  #ifdef ROUTING_DEBUG
  printf ("\t%08X (reset)\n", * ((int *) map));
  #endif
}


int IsEmptyMap (MapUnitType * map)
{
  int i;
  int n = PORT_MAP_SIZE / (32 / MAP_UNIT_SIZE);
  for (i = 0; i < n; ++ i) {
    if (((uint32 *) map)[i]) return 0;
  }
  for (i = n * (32 / MAP_UNIT_SIZE) + 1; i < PORT_MAP_SIZE; ++ i) {
    if (map[i]) return 0;
  }
  return 1;
}


/* 
 * For debug only
 */
#ifdef ROUTING_DEBUG

int main (int argc, char ** argv)
{
  int i, m;
  MapUnitType * map;
  uint32 grp;
  
  if (argc != 2) {
    printf ("Topology file is not specified!\n");
    exit (1);
  }
  
  if (! ReadTopo (argv[1], &m)) {
    printf ("Reading topology from <%s> failed.\n", argv[1]);
    return 1;
  }

  InitMRoute ();
  AssignAddr (0, 0);
  BuildAddr2IdxMap ();

  PrintLinkTable ();  
  
  grp = 0x80000000L + 1;
  
  AddGroup (grp, 0);
  JoinGroup (7, grp);
  JoinGroup (4, grp);

  for (i = 0; i <= 7; ++ i) {
    map = GetFwdPortMap (i, grp);
    if (map == NULL) {
      printf ("Node %d: Not in group.\n", i);
    }
    else {
      printf ("Node %d: %08X\n", i, * ((int *) map));
    }
  }

  /*

  printf ("\n");

  LeaveGroup (4, 1);
    
  for (i = 0; i < _nodeNum; ++ i) {
    map = GetFwdPortMap (i, 1);
    if (map == NULL) {
      printf ("Node %d: Not in group.\n", i);
    }
    else {
      printf ("Node %d: %08X\n", i, * ((int *) map));
    }
  }
   
  */

  return 0;
}

#endif 
