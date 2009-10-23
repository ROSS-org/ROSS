#ifndef ROSS_MCAST_ROUTING_H
#define ROSS_MCAST_ROUTING_H

#include "Topo.h"
#include <string.h>

#define MROUTE_TABLE_SIZE       36677
//#define MROUTE_TABLE_SIZE       1009 
#define GROUP_TABLE_SIZE        1009

#define HASH_DIST_FACTOR        3917

#define HASH_BUCKET_DEPTH       5   /* Only 4 will be used */
#define PORT_MAP_SIZE           4   /* In MAP_UNIT_SIZE bits */
typedef uint8 MapUnitType;
#define MAP_UNIT_SIZE           8
#define HIGHEST_BIT_TEST        0x80
#define HIGHEST_MAP_BIT_POS     (PORT_MAP_SIZE * MAP_UNIT_SIZE - 1)
#define MAX_PORT_NUM            (MAP_UNIT_SIZE * PORT_MAP_SIZE)


#define GetGrpAddr4Print(addr)          ((addr) & 0x7FFFFFFFL)
#define IsMcastAddr(addr)               ((addr) & 0x80000000)
#define SetGrpAddr4Prog(addr)           ((addr) |= 0x80000000)

typedef struct MRouteStruct {
  uint8 used_;
  int lpId_;
  uint32 groupId_;      /* Multicast group address */
  MapUnitType portMap_[PORT_MAP_SIZE]; /* The highest bit of the highest byte
                                        * is used to indicate whether there is
                                        * a local agent in the group */
} MRoute; /* Multicast routing */


typedef struct MRouteHashStruct {
  int16 head_, tail_;
  MRoute route_[HASH_BUCKET_DEPTH];
} MRouteHash;


typedef struct GroupInfoStruct {
  uint8 used_;
  uint32 groupId_;
  int   srcId_;        /* index, not address */
} GroupInfo;


typedef struct GroupInfoHashStruct {
  int16 head_, tail_;
  GroupInfo gi_[HASH_BUCKET_DEPTH];
} GroupInfoHash;


extern MRouteHash _mrouteTable[];
extern GroupInfoHash _groupTable[];

int AddMRoute (int lpId, uint32 groupId, uint16 inPort);
int DelMRoute (int lpId, uint32 groupId, uint16 inPort);
MapUnitType * GetFwdPortMap (int lpId, uint32 groupId);
inline void InitMRoute ();

void AddGroup (uint32 groupId, int srcId);
int GetGroupSrcId (uint32 groupId);
void DelGroup (uint32 groupId);
inline uint32 AllocGroupAddr ();

Link * GetFwdLink (int routerIdx, uint32 destAddr);

void JoinGroup (int routerIdx, uint32 groupId);
void LeaveGroup (int routerIdx, uint32 groupId);

inline int IsBitSet (MapUnitType * map, uint16 pos);
inline void SetBit (MapUnitType * map, uint16 pos);
inline void ResetBit (MapUnitType * map, uint16 pos);
inline int IsEmptyMap (MapUnitType * map);

#endif
