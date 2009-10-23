#ifndef ROSS_MCAST_TOPO_H
#define ROSS_MCAST_TOPO_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef uint32
typedef unsigned long uint32;
#endif

#ifndef uint64
typedef unsigned long long uint64;
#endif

#ifndef int8
typedef char int8;
#endif
 
#ifndef int16
typedef short int16;
#endif

#ifndef int32
typedef long int32;
#endif

#ifndef int64
typedef long long int64;
#endif


#define TOPO_DEBUG_MSG          1


typedef struct LinkStruct {
  int neighbour_;           /* The implicit id (node table index) of the
                             * neighbour node */
  uint16 nbPort_;           /* The port of the neighbour */
  unsigned long bw_;        /* bandwidth in bit/sec. */
  double latency_;          /* in seconds */
  unsigned long bufSize_;   /* queue size in bytes */
  double lastPktOutTime_;   /* Latest time when all the packets in the buffer
                             * are cleared */
  uint8 active_;            /* Indicate whether this link is active */
  uint8 route_;             /* Indicate whehter this link is used for routing */
  struct LinkStruct * next_;
} Link;


/*
 * Each node has three IDs:
 * 1. lpId_, id of lp. The same as the implicit id below.
 * 2. Implicit id, the index in the node table. Equals to the id from the
 *    config file.
 * 3. Address id, the address in the tree to make routing simple. Begin from 0.
 */
typedef struct NodeStruct {
  uint32 addr_;                  /* Begins from 0 */
  uint32 lowAddr_, highAddr_;    /* Lowest and highest address of downstream
                                 * nodes */
  uint16 linkNum_;
  Link ** links_;
  Link * firstLink_;
  union {
    Link * upLink_;             /* The upstream link for default routing */
    Link * lastLink_;           /* Must not use after reading topology */
  }link;
} Node;


extern Node * _nodeTable;
extern int _nodeNum;
extern int * _addrToIdx;

void PrintLinkTable ();

int AssignAddr (int startPoint, uint32 startAddr);
inline void BuildAddr2IdxMap ();
int ReadTopo (char * topoFileName, int * maxBufSize);
void BuildNodeLinkArray ();

#endif
