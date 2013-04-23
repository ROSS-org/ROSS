 
 #include <ross.h>
 
 #define MEAN_DEPARTURE 30.0
 #define MEAN_LAND 10.0
 typedef enum events events;
 typedef struct Dissim_State Dissim_State;
 typedef struct Msg_Data Msg_Data;
 typedef struct Civilian_State Civilian_State;
 typedef struct Powerline_State Powerline_State;
 typedef struct Building_State Building_State;
 typedef struct Substation_State Substation_State;
 typedef struct Generator_State Generator_State;
 
//By Curtis Antolik

 enum events{ CIVILIAN = 1, BUILDING, POWERLINE, SUBSTATION, GENERATOR, UPDATEDRAW, UPDATEAVAILABLE, DAMAGE, REPAIR, UPDATEOCCUPANCY, POWERON, POWEROFF, HOME, WORK, TRAVELHOME, TRAVELWORK, TRAVELLEISURE, LEISURE, CRISISLEVELRISES, CRISISLEVELFALLS, FINDFAMILY, EVACUATE, SHELTER, FLIGHT, GROUP, SHOCK, CURIOUS, TRAVELEVACUATE, RELEASEHOME, RELEASEWORK, RELEASEHOSPITAL, HOSPITAL, TRAVELHOSPITAL, RELEASELEISURE};
  


struct Msg_Data{
    enum events classToUse;
    int event_type;
    int InfoBlock1;
    int InfoBlock2;
};
 struct Dissim_State{
    int dissimType;
    Civilian_State c;
    Powerline_State p;
    Building_State b;
    Substation_State s;
    Generator_State g;
    
};

 /*struct airport_message
 {
         airport_event_t  type;
 
         tw_stime         waiting_time;
         tw_stime         saved_furthest_flight_landing;
 };*/
 
 static tw_lpid   nlp_per_pe = 14;
 static tw_stime  mean_flight_time = 1;
 static int       opt_mem = 20000000;
 static int       planes_per_airport = 1;
 static int A = 60; 
 static int R = 10;
 static int G = 45;
 
 static tw_stime  wait_time_avg = 0.0;
