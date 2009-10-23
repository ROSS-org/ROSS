#ifndef ROSS_MCAST_TRANSPORT_H
#define ROSS_MCAST_TRANSPORT_H

#include "Multicast.h"

#define SIM_RELAX_TIME                  0.00000000001
#define DATA_PACKET_SIZE                1000
#define CTRL_PACKET_SIZE                40

inline void SendPacket (tw_lp * lp, Packet * p);
inline void SendExtPkt (tw_lp * lp, Packet * p, void * ext, int extSize);
void ReadAgentAct (char * fileName);
inline void CancelTimer (Agent * a, tw_event ** e);
inline void ReschedTimer (Agent * a, int timerId, double offset, 
                          tw_event ** e);
inline tw_event * ScheduleTimer (Agent * a, int timerId, double offset);

inline void ReschedTimerWithSn (Agent * a, int timerId, int serial,
                                double offset, tw_event ** e);
inline tw_event * ScheduleTimerWithSn (Agent * a, int timerId, int serial,
                                       double offset);

extern char * _AgentActFileName;

#endif
