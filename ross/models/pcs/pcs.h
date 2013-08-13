#include "ross.h"

#define TW_MAX_NAME_LEN 31

#define NUM_CELLS_X 1024     //256
#define NUM_CELLS_Y 1024     //256

#define NUM_VP_X 512 
#define NUM_VP_Y 512

#define MAX_NORMAL_CHANNELS 10
#define MAX_RESERVE_CHANNELS 0

/*
 * This is in Mins 
 */
#define MOVE_CALL_MEAN 4500.0
#define NEXT_CALL_MEAN 360.0
#define CALL_TIME_MEAN 180.0

#define BIG_N (double)16.0

/*
 * When Normal_Channels == 0, then all have been used 
 */
#define NORM_CH_BUSY ( !( SV->Normal_Channels & 0xffffffff ) )

/*
 * When Reserve_Channels == 0, then all have been used 
 */
#define RESERVE_CH_BUSY ( !( SV->Reserve_Channels & 0xffffffff ) )

typedef int     Channel_t;
typedef int     Min_t;
typedef int     MethodName_t;

#define NONE 0
#define NORMAL_CH 1
#define RESERVE 2

#define COMPLETECALL 3
#define NEXTCALL 4
#define MOVECALL 5

#define NEXTCALL_METHOD 6
#define COMPLETIONCALL_METHOD 7
#define MOVECALLIN_METHOD 8
#define MOVECALLOUT_METHOD 9


/********* User Entity Classes ******************************************/

struct State
{
	double          Const_State_1;
	int             Const_State_2;
	int             Normal_Channels;
	int             Reserve_Channels;
	int             Portables_In;
	int             Portables_Out;
	int             Call_Attempts;
	int             Channel_Blocks;
	int             Busy_Lines;
	int             Handoff_Blocks;
	int             CellLocationX;
	int             CellLocationY;
};

struct RC
{
	int             wl1;	
};

struct Msg_Data
{
	MethodName_t    MethodName;
	double          CompletionCallTS;
	double          NextCallTS;
	double          MoveCallTS;
	Channel_t       ChannelType;
	struct RC       RC;
};

struct CellStatistics
{
	int             Call_Attempts;
	int             Channel_Blocks;
	int             Busy_Lines;
	int             Handoff_Blocks;
	int             Portables_In;
	int             Portables_Out;
	double          Blocking_Probability;
};

void            Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
Min_t           Cell_MinTS(struct Msg_Data *M);
void            Cell_StartUp(struct State *SV, tw_lp * lp);
void            Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            RC_Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            RC_Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            RC_Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            RC_Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            RC_Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp);
void            CellStatistics_CollectStats(struct State *, tw_lp *lp);
void            CellStatistics_Compute();
void            CellStatistics_Print();

struct CellStatistics TWAppStats;
tw_lpid nlp_per_pe = 0;
