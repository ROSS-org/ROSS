#include <ross.h>


/* This is a modify verison of airport.c which was done by Justin M. LaPre */

int A = 60; 
int R = 10;
int G = 45;

int NumLanded = 0;

enum events { ARRIVAL, LAND, DEPARTURE};

typedef struct {
  enum events event_type;
} Msg_Data;


typedef struct {
  int OnTheGround;
  int InTheAir;
  int RunwayFree;

  int NumLanded;
} Airport_State;


void Airport_StartUp(Airport_State *, tw_lp *);
void Airport_EventHandler(Airport_State *, tw_bf *, Msg_Data *, tw_lp *);
void Airport_RC_EventHandler(Airport_State *, tw_bf *, Msg_Data *, tw_lp *);
void Airport_Statistics_CollectStats(Airport_State *, tw_lp *);

#define AIR_LP 1

tw_lptype airport_lps[] = {
  {
    AIR_LP, sizeof(Airport_State),
    (init_f) Airport_StartUp,
    (event_f) Airport_EventHandler,
    (revent_f) Airport_RC_EventHandler,
    (final_f) Airport_Statistics_CollectStats,
    (statecp_f) NULL
  },
  { 0 },
};

int main(int argc, char * argv[]) {
  tw_lp *lp;
  tw_kp *kp;
  g_tw_ts_end = 300;
  g_tw_gvt_interval = 16;

  /* tw_lptype NumPE NumKP NumLP Message_Size*/

  tw_init(airport_lps,1,1,1,sizeof(Msg_Data));

  lp = tw_getlp(0);
  kp = tw_getkp(0);
  tw_lp_settype(lp, AIR_LP);
  tw_lp_onkp(lp, kp);
  tw_lp_onpe(lp, tw_getpe(0));
  tw_kp_onpe(kp, tw_getpe(0));
 
  tw_run();

  printf("Number of Landings: %d\n", NumLanded);

  return 0;
}

void  Airport_StartUp(Airport_State *SV, tw_lp * lp) {
  int i;
  tw_event *CurEvent;
  tw_stime ts;
  Msg_Data *NewM;

  SV->OnTheGround = 0;
  SV->InTheAir = 0 ;
  SV->RunwayFree = 1;
  SV->NumLanded = 0;

  for(i = 0; i < 5; i++) {
    ts = tw_rand_exponential(lp->id, A);
    CurEvent = tw_event_new(lp, ts, lp);
    NewM = (Msg_Data *)tw_event_data(CurEvent);
    NewM->event_type = ARRIVAL;
    tw_event_send(CurEvent);
  }
}

void Airport_EventHandler(Airport_State *SV, tw_bf *CV, Msg_Data *M,
		      tw_lp *lp) {
  tw_stime ts;
  tw_event *CurEvent;
  Msg_Data *NewM;

  *(int *)CV = (int)0;

  switch(M->event_type) {
    
  case ARRIVAL: 
    // Schedule a landing in the future
    SV->InTheAir++;
    
    if((CV->c1 = (SV->RunwayFree == 1))){
      SV->RunwayFree = 0;
   
      ts = tw_rand_exponential(lp->id, R);
      CurEvent = tw_event_new(lp, ts, lp);
      NewM = (Msg_Data *)tw_event_data(CurEvent);
      NewM->event_type = LAND;
      tw_event_send(CurEvent);
    }
    break;
      
  case LAND: 
    SV->InTheAir--;
    SV->OnTheGround++;
    SV->NumLanded++;
    
    ts = tw_rand_exponential(lp->id, G);
    CurEvent = tw_event_new(lp, ts, lp);
    NewM = (Msg_Data *)tw_event_data(CurEvent);
    NewM->event_type = DEPARTURE;
    tw_event_send(CurEvent);
    
    if ((CV->c1 = (SV->InTheAir > 0))){
      ts = tw_rand_exponential(lp->id, R);
      CurEvent = tw_event_new(lp, ts, lp);
      NewM = (Msg_Data *)tw_event_data(CurEvent);
      NewM->event_type = LAND;
      tw_event_send(CurEvent);
    }
    else
      SV->RunwayFree = 1;
    break;
    
  case DEPARTURE: 
    SV->OnTheGround--;
    
    ts = tw_rand_exponential(lp->id, A);
    CurEvent = tw_event_new(lp, ts, lp);
    NewM = (Msg_Data *) tw_event_data(CurEvent);
    NewM->event_type = ARRIVAL;
    tw_event_send(CurEvent);
    break;
  }
}

void Airport_RC_EventHandler(Airport_State *SV, tw_bf *CV, Msg_Data *M,
			tw_lp *lp) {
  
  switch(M->event_type) {

  case ARRIVAL: 
    SV->InTheAir--;
    
    if(CV->c1){
      tw_rand_reverse_unif(lp->id);
      SV->RunwayFree = 1;
    }
    break;
    
  case LAND: 
    SV->InTheAir++;
    SV->OnTheGround--;
    SV->NumLanded--;
    tw_rand_reverse_unif(lp->id);
    
    if(CV->c1)
      tw_rand_reverse_unif(lp->id);
    else
      SV->RunwayFree = 0;
    break;


  case DEPARTURE:  
    SV->OnTheGround++;
    tw_rand_reverse_unif(lp->id);
    break;
  }
}
    

void Airport_Statistics_CollectStats(Airport_State *SV, tw_lp * lp) {

  NumLanded += SV->NumLanded;
}








