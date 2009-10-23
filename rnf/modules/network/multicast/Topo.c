#include "Topo.h"
#include "Util.h"

Node * _nodeTable; /* Stores all the link and node info */
int _nodeNum;      /* Total node number */ 
int * _addrToIdx;  /* Stores the address to index mapping of all nodes */

/*
 * @param maxBufSize in bytes
 * @Return: 0 if failed, otherwise return the number of nodes actually read.
 */
int ReadTopo (char * topoFileName, int * maxBufSize) 
{
  int fd, n, i, readNodeNum = 0;
  char buf[MAX_LINE_LEN];
  unsigned int node[2];
  unsigned long bw[2], bs[2];
  double delay[2];  
  Link * lnk;    
  
  _nodeNum = -1;
  
  if (topoFileName == NULL) return 0;
  
  if ((fd = open (topoFileName, O_RDONLY)) < 0) {
    #ifdef TOPO_DEBUG_MSG
    printf ("Failed to open the topology file <%s>\n", topoFileName);
    #endif
    return 0;
  } 
  
  /* 
   * Get the total number of nodes
   */
  do {
    if (ReadLine (fd, buf) <= 0) {
      #ifdef TOPO_DEBUG_MSG
      printf ("Topology error: No total node number.\n");
      #endif
      close (fd);
      return 0;
    }
    
    if (sscanf (buf, "%u", &_nodeNum) != 1) {
      #ifdef TOPO_DEBUG_MSG
      printf ("Topology error: Wrong format of total node number.\n");
      #endif
      close (fd);
      return 0;
    }  
  } while (_nodeNum < 0);
  
  if ((_nodeTable = (Node *) calloc (_nodeNum, sizeof (Node))) == NULL) {
    printf ("Unable to allocate memory for node table.\n");
    exit (1);
  }
  
  memset ((void *) _nodeTable, 0, sizeof (Node) * _nodeNum);
  for (i = 0; i < _nodeNum; ++ i) {
    _nodeTable[i].addr_ = _nodeTable[i].lowAddr_ = _nodeTable[i].highAddr_ = -1;
  }
  
  /*
   * Read each line for link info
   */  
  while (ReadLine (fd, buf) > 0) {
    if ((n = sscanf (buf, "%u %u %lu %lf %lu %lu %lf %lu",
                           node, node + 1, 
                           bw, delay, bs, bw + 1, delay + 1, bs + 1)) <= 0) {
      continue;                           
    }
    
    if (node[0] >= _nodeNum || node[1] >= _nodeNum) {
      printf ("Topology error: Node index beyond scope. <%s>\n", buf);
      close (fd);
      return 0;
    }
                           
    if (n < 5) {
      #ifdef TOPO_DEBUG_MSG
      printf ("Topology error: Not enough link info. <%s>\n", buf);
      #endif
      close (fd);
      return 0;
    }
    if (n > 8) {
      #ifdef TOPO_DEBUG_MSG
      printf ("Topology error: Too much link info. <%s>\n", buf);
      #endif
      close (fd);
      return 0;
    }
    switch (n) {
    case 5:
      bw[1] = bw[0]; delay[1] = delay[0]; bs[1] = bs[0];
      break;
    case 6:
      delay[1] = delay[0]; bs[1] = bs[0];
      break;
    case 7:
      bs[1] = bs[0];
      break;
    }
    
    /*
     * Construct link table
     */    
    for (i = 0; i < 2; ++ i) {    
      if ((lnk = malloc (sizeof (Link))) == NULL) {
        printf ("Unable to allocate memory for link table.\n");
        exit (1);
      }
      memset ((void *) lnk, 0, sizeof (Link));
      lnk->neighbour_ = node[(i + 1) % 2];
      lnk->bw_ = bw[i] * 1000;
      lnk->latency_ = delay[i];
      lnk->bufSize_ = bs[i] * 1000;
      if (lnk->bufSize_ > *maxBufSize) *maxBufSize = lnk->bufSize_;
      lnk->active_ = 1;
      lnk->next_ = NULL;
      
      if (_nodeTable[node[i]].firstLink_ == NULL) {
        ++ readNodeNum;
        _nodeTable[node[i]].firstLink_ = _nodeTable[node[i]].link.lastLink_ = lnk;
      }
      else {
        _nodeTable[node[i]].link.lastLink_->next_ = lnk;
        _nodeTable[node[i]].link.lastLink_ = lnk;
      }
      ++ _nodeTable[node[i]].linkNum_;
    }
  }
  
  close (fd);
  
  for (i = 0; i < _nodeNum; ++ i) {
    _nodeTable[i].link.upLink_ = NULL;
  }  
  BuildNodeLinkArray ();

  return readNodeNum;
}


/*
 * Assign address to all nodes for the ease of routing.
 * @return The number of nodes getting address.
 */
int AssignAddr (int startNodeId, uint32 startAddr)
{
  Link * lnk = _nodeTable[startNodeId].firstLink_, * lnk2;
  Node * nbNode;
  int numAddrAssigned = 1;
  uint16 port, nbPort;
  
  _nodeTable[startNodeId].addr_ = _nodeTable[startNodeId].lowAddr_ = 
    _nodeTable[startNodeId].highAddr_ = startAddr;
  
  for (port = 0; lnk != NULL; lnk = lnk->next_, ++ port) {
    if ((nbNode = _nodeTable + lnk->neighbour_)->addr_ != (uint32) -1) {
       continue; /* Have been assigned */
    }
    /* Mark this link for routing */
    lnk->route_ = 1;
    
    /* Set uplink for the neighbour node */
    for (nbPort = 0, lnk2 = nbNode->firstLink_; 
         lnk2 != NULL; lnk2 = lnk2->next_, ++ nbPort) {
      if (lnk2->neighbour_ == startNodeId) {
        nbNode->link.upLink_ = lnk2;
        nbNode->link.upLink_->route_ = 1;
        nbNode->link.upLink_->nbPort_ = port;
        lnk->nbPort_ = nbPort;
        break;
      }
    }
    if (lnk2 == NULL) {
      printf ("Failed to set uplink for node %d.\n", lnk->neighbour_);
      exit (1);
    }
    /* Assign address recursively */
    numAddrAssigned += 
      AssignAddr (lnk->neighbour_, startAddr + numAddrAssigned);
      
    _nodeTable[startNodeId].highAddr_ = nbNode->highAddr_;
  };
  
  return numAddrAssigned;
}


void BuildAddr2IdxMap ()
{
  int i;
  
  if ((_addrToIdx = (int *) calloc (_nodeNum, sizeof (int))) == NULL) {
    printf 
      ("Unable to allocate memory for address to index mapping for nodes.\n");
    exit (1);
  }
  
  memset ((void *) _addrToIdx, -1, sizeof (int) * _nodeNum);
  
  for (i = 0; i < _nodeNum; ++ i) {
    if (_nodeTable[i].addr_ < 0) continue;
    if (_addrToIdx[_nodeTable[i].addr_] >= 0) {
      printf ("Duplicate address.\n");
      exit (1);
    }
    _addrToIdx[_nodeTable[i].addr_] = i;
  }
}


void BuildNodeLinkArray ()
{
  int i, j;
  Link * lnk;
  Link ** lnkPtr;
  
  for (i = 0; i < _nodeNum; ++ i) {
    lnkPtr = (Link **) calloc (_nodeTable[i].linkNum_, sizeof (Link *));
    if (lnkPtr == NULL) {
      printf ("Failed to allocate memory for link array.\n");
      exit (-1);
    }
    for (j = 0, lnk = _nodeTable[i].firstLink_; lnk != NULL;
         ++ j, lnk = lnk->next_) {
      lnkPtr[j] = lnk;
    }
    _nodeTable[i].links_ = lnkPtr;
  }
}


void PrintLinkTable ()
{
   int i, j;
   Link * lnk;
   
   for (i = 0; i < _nodeNum; ++ i) {
     if ((lnk = _nodeTable[i].firstLink_) == NULL) {
       printf ("Node %d is not linked !!!\n", i);
       exit (1);
     }
     
     printf ("Node %d: %d ~ %d, ", i, (int) _nodeTable[i].lowAddr_,
       (int) _nodeTable[i].highAddr_); 
     if (_nodeTable[i].link.upLink_ == NULL) {
       printf ("upstream N/A\n");
     }
     else {
       printf ("upstream %d\n", _nodeTable[i].link.upLink_->neighbour_);
     }
     j = 0;
     do {
       printf ("\t[%d:%d -> %d:%d]<%d>\t bw = %lu, delay = %lf, buf = %lu\n",
         i, j,
         lnk->neighbour_, (int) lnk->nbPort_, lnk->route_,
         lnk->bw_, lnk->latency_, lnk->bufSize_
       );
       ++ j;
     } while ((lnk = lnk->next_) != NULL);
   }  
   
   
   /* Addr to index mapping */
   for (i = 0; i < _nodeNum; ++ i) {
     printf ("Addr: %d -> Index: %d\n", i, _addrToIdx[i]);
   }
}


/* 
 * For debug only
 */
#ifdef TOPO_DEBUG

int main (int argc, char ** argv)
{
  int maxBufSize;
  
  if (argc != 2) {
    printf ("Topology file is not specified!\n");
    exit (1);
  }
  
  if (! ReadTopo (argv[1], &maxBufSize)) {
    printf ("Reading topology from <%s> failed.\n", argv[1]);
    return 1;
  }

  AssignAddr (0, 0); 
  BuildAddr2IdxMap ();  
  BuildNodeLinkArray ();

  PrintLinkTable ();  
  
  return 0;
}

#endif 
