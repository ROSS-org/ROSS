#include "Transport.h"
#include "ormcc.h"
#include "pgmcc.h"
#include "tfmcc.h"
#include "tfmcc-sink.h"
#include "gmcc-snd.h"
#include "gmcc-rcv.h"

char * _AgentActFileName = NULL;


void ReadAgentAct (char * fileName)
{
  int fd, n;
  char buf[MAX_LINE_LEN];
  char agentType[10];
  double t;
  int nid;
  char addrType, act;
  double randOff;
  int16 agentId;
  uint32 addr;
  int16 pktDstAgentId;
  tw_lp * lp;
  tw_event * e;
  EventMsg * em;
  Agent * a;
  StatAgentList * sal;

  /*
   * Read time table of agent action. Do it only once (at init of lp 0).
   */
  if ((fd = open (fileName, O_RDONLY)) < 0) {
    printf ("Failed to open the agent action file <%s>\n", fileName);
    exit (-1);
  }
  
  while (ReadLine (fd, buf) > 0) {
    if ((n = sscanf (buf, "%lf %lf %c %s %d %d %u %d %c", 
                     &t, &randOff, &act, agentType, &nid, (int *) &agentId, 
                     (unsigned int *) &addr, (int *) &pktDstAgentId,
                     &addrType)) <= 0) {
      continue;
    }
    
    if (n != 9) {
      printf ("Agent action error: Wrong format. Line: <%s>\n", buf);
      exit (-1);
    }
    
    if (randOff > 0) {
      t += (rand () / (double) RAND_MAX) * randOff;
    }
    
    if (t < 0 || nid < 0 || agentId < 0) {
      printf ("Agent action error: Invalid content. Line: <%s>\n", buf);
      exit (-1);
    }
    
    if (t > g_tw_ts_end) continue;
    
    if (t == g_tw_ts_end) {
      t = g_tw_ts_end - SIM_RELAX_TIME;
    }

    switch (addrType) {
    case 'm':
      SetGrpAddr4Prog (addr);
      break;
    case 'u':
      addr = _nodeTable[addr].addr_;
      break;
    default:
      printf ("Unknown address type: %s\n", buf);
      exit (-1);
    }
    
    if ((lp = tw_getlp (nid)) == NULL) {
      printf ("Agent action error: Unable to get lp for node. Line: <%s>\n",
              buf);
      exit (-1);
    }

    if ((e = tw_event_new (lp, t, lp)) == NULL) {
      printf ("Agent action error: Unable to create event. Line: <%s>\n", buf);
      exit (-1);
    }
    em = (EventMsg *) tw_event_data (e);
    
    /* Process the operation now */
    if (strcasecmp (agentType, "ormcc_src") == 0) {
      a = OrmccSrcInit (nid, agentId, addr, pktDstAgentId, act == 's');
    }
    else if (strcasecmp (agentType, "ormcc_rcv") == 0) {
      a = OrmccRcvInit (nid, agentId, addr);
    }
    else if (strcasecmp (agentType, "gmcc_src") == 0) {
      a = GmccSrcInit (nid, agentId, addr, pktDstAgentId);
    }
    else if (strcasecmp (agentType, "gmcc_rcv") == 0) {
      a = GmccRcvInit (nid, agentId, addr, act == 's');
    }
    else if (strcasecmp (agentType, "pgmcc_src") == 0) {
      a = PgmccSrcInit (nid, agentId, addr, pktDstAgentId, act == 's');
    }
    else if (strcasecmp (agentType, "pgmcc_rcv") == 0) {
      a = PgmccRcvInit (nid, agentId, addr);
    }
    else if (strcasecmp (agentType, "tfmcc_src") == 0) {
      a = TfmccSrcInit (nid, agentId, addr, pktDstAgentId, act == 's');
    }
    else if (strcasecmp (agentType, "tfmcc_rcv") == 0) {
      a = TfmccRcvInit (nid, agentId, addr);
    }
    else {            
      printf ("Agent action error: Unknown agent type.\n");
      exit (-1);
    }

    switch (act) {
      case 'b':
        em->type_ = START_AGENT;
        break;
      case 's':
        em->type_ = START_AGENT;
        if (statAgentListHead_ == NULL) {
          statAgentListTail_ = statAgentListHead_
            = (StatAgentList *) malloc (sizeof (StatAgentList));
          statAgentListHead_->agent_ = a;
          statAgentListHead_->next_ = NULL;
        }
        else {
          sal = (StatAgentList *) malloc (sizeof (StatAgentList));
          sal->agent_ = a;
          sal->next_ = NULL;
          statAgentListTail_->next_ = sal;
          statAgentListTail_ = sal;
        }
        break;
      case 'e':
        em->type_ = STOP_AGENT;
        break;
      default:
        printf ("Unknown agent action. %s\n", buf);
        exit (-1);
    }

    em->content_.agent_ = a;
    tw_event_send (e);
  }
  
  close (fd);
}


void SendPacket (tw_lp * lp, Packet * p)
{
  MapUnitType * portMap;
  
  p->id_ = 0; /* Make sure no extended part exists */
  if (IsMcastAddr (p->dstAddr_)) {
    if ((portMap = GetFwdPortMap (lp->id, p->dstAddr_)) != NULL) {
      ForwardMulticastPkt (lp, p, portMap);
    }
  }
  else {
    ForwardUnicastPkt (lp, p);
  }
}


void SendExtPkt (tw_lp * lp, Packet * p,  void * ext, int extSize)
{
  MapUnitType * portMap;
  uint32 ref = 0;


  if (IsMcastAddr (p->dstAddr_)) {
    if ((portMap = GetFwdPortMap (lp->id, p->dstAddr_)) != NULL) {
      ref = ForwardMulticastPkt (lp, p, portMap);
    }
  }
  else {
    ref = ForwardUnicastPkt (lp, p);
  }
  
  if (ref > 0) {
    AddExtPkt (p->srcAddr_, p->srcAgent_, p->id_, ext, extSize, ref);
  }
}


void CancelTimer (Agent * a, tw_event ** e)
{
  if (*e != NULL) tw_timer_cancel (tw_getlp (a->nodeId_), e);
}


void ReschedTimer (Agent * a, int timerId, double offset, tw_event ** e)
{
  if (*e != NULL) tw_timer_cancel (tw_getlp (a->nodeId_), e);

  *e = ScheduleTimer (a, timerId, offset);
}


tw_event * ScheduleTimer (Agent * a, int timerId, double offset)
{
  tw_lp * lp = tw_getlp (a->nodeId_);
  tw_event * e;
  double now;
  double t = (now = tw_now (lp)) + offset;
  EventMsg * em;
  
  /* Scheduling timers beyond the simulation causes the program to quit */
  if (t >= g_tw_ts_end) {
    if (g_tw_ts_end - now <= SIM_RELAX_TIME * 1.01) return NULL;
    t = g_tw_ts_end - SIM_RELAX_TIME;
  }

  if ((e = tw_timer_init (lp, t)) == NULL) {
    printf ("Failed to create timer event.\n");
    tw_exit (-1);
  }
  
  em = tw_event_data (e);
  
  em->type_ = AGENT_TIMER;
  em->content_.timeOut_.agent_ = a;
  em->content_.timeOut_.timerId_ = timerId;
  
  return e;
}


void ReschedTimerWithSn (Agent * a, int timerId, int serial, 
                               double offset, tw_event ** e)
{
  if (*e != NULL) tw_timer_cancel (tw_getlp (a->nodeId_), e);

  *e = ScheduleTimerWithSn (a, timerId, serial, offset);
}


tw_event * ScheduleTimerWithSn
   (Agent * a, int timerId, int serial, double offset)
{
  tw_lp * lp = tw_getlp (a->nodeId_);
  tw_event * e;
  double now;
  double t = (now = tw_now (lp)) + offset;
  EventMsg * em;
  
  /* Scheduling timers beyond the simulation causes the program to quit */
  if (t >= g_tw_ts_end) {
    if (g_tw_ts_end - now <= SIM_RELAX_TIME * 1.01) return NULL;
    t = g_tw_ts_end - SIM_RELAX_TIME;
  }

  if ((e = tw_timer_init (lp, t)) == NULL) {
    printf ("Failed to create timer event.\n");
    tw_exit (-1);
  }
  
  em = tw_event_data (e);
  
  em->type_ = AGENT_TIMER;
  em->content_.timeOut_.agent_ = a;
  em->content_.timeOut_.timerId_ = timerId;
  em->content_.timeOut_.serial_ = serial;
  
  return e;
}

#if ROSS_MODEL
/*
 * Sequential model only.
 * Command line arguments:
 *   simulation time, topology file name, time table file name
 */
int main (int argc, char ** argv)
{
  int n,
      maxBufSize = 0;  /* in bytes */
  tw_lp * lp;
  tw_kp * kp;
  tw_pe * pe;
  
  if (argc != 5) {
    printf ("Usage: %s <simulation time> <topology file name> ", argv[0]);
    printf ("<group info file name> <agent action file name>\n");
    return 0;
  }

  /*
   * Read and process topology 
   */

  if ((g_tw_ts_end = atof (argv[1])) <= 0) {
    printf ("Invalid simulation time.\n");
    return 1;
  }
  
  if ((n = ReadTopo (argv[2], &maxBufSize)) == 0) {
    printf ("Reading topology from <%s> failed.\n", argv[2]);
    return 1;
  }
  
  _groupOpFileName = strdup (argv[3]);
  _AgentActFileName = strdup (argv[4]);  
  
  if (n != _nodeNum) {
    printf ("Specified node number is larger than the real amount.\n");
    return 1;
  }
  
  srandom(time(NULL));
  AssignAddr (0, 0);
  BuildAddr2IdxMap ();
  //PrintLinkTable ();  
  InitMRoute ();
  memset ((void *) _agentTable, 0, sizeof (AgentHash) * AGENT_TABLE_SIZE);

  /*
   * Initialize ROSS
   */

  g_tw_events_per_pe = (maxBufSize / ((DATA_PACKET_SIZE + CTRL_PACKET_SIZE) / 2)
                        + EXTRA_EVENT_PER_NODE) * _nodeNum;

  if (! tw_init (_mylps, 1, KP_NUM_PER_PE, _nodeNum, sizeof (EventMsg))) {
    printf ("Failed to initialize ROSS.\n");
    return 1;
  }
  
  pe = tw_getpe (0);
  
  for (n = 0; n < _nodeNum; ++ n) {
    lp = tw_getlp (n);
    kp = tw_getkp (n % KP_NUM_PER_PE);
    tw_lp_settype (lp, TW_MCAST);
    tw_lp_onkp (lp, kp);
    tw_lp_onpe (lp, pe);
    tw_kp_onpe (kp, pe);
  }
  
  tw_run ();
  
  return 0;
}
#endif
