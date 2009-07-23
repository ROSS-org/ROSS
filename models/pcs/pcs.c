/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2001 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */ 

#include <ross.h>

#define TW_MAX_NAME_LEN 31

#define NUM_CELLS_X 32
#define NUM_CELLS_Y 32

#define NUM_VP_X 8					
#define NUM_VP_Y 8


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
int             Neighbors[NUM_CELLS_X * NUM_CELLS_Y][4];

struct CellStatistics TWAppStats;
tw_lpid nlp_per_pe = 0;

double 
Pi_Distribution(double n, double N)
{
  double          i;
  double          nfactorial;
  
  nfactorial = 1.0;
  for (i = 1.0; i <= n; i += 1.0)
    nfactorial *= i;
  return ((pow(N, n) * exp(-N)) / nfactorial);
 }
 int 
 GenInitPortables(tw_lp * lp)
 {
	 return ((int)BIG_N);
 }
 Min_t 
 Cell_MinTS(struct Msg_Data *M)
 {
	 if (M->CompletionCallTS < M->NextCallTS)
	 {
		 if (M->CompletionCallTS < M->MoveCallTS)
			 return (COMPLETECALL);
		 else
			 return (MOVECALL);
	 } else
	 {
		 if (M->NextCallTS < M->MoveCallTS)
			 return (NEXTCALL);
		 else
			 return (MOVECALL);
	 }
 }

tw_peid
mapping(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

 void 
 Cell_StartUp(struct State *SV, tw_lp * lp)
 {
	 tw_lpid currentcell = 0, newcell = 0;
	 int             i, dest_index = 0;
	 tw_stime          ts;

	 struct Msg_Data TMsg;
	 struct Msg_Data * TWMsg;
	 tw_event *CurEvent;

	 SV->Normal_Channels = MAX_NORMAL_CHANNELS;
	 SV->Reserve_Channels = MAX_RESERVE_CHANNELS;
	 SV->Portables_In = 0;
	 SV->Portables_Out = 0;
	 SV->Call_Attempts = 0;
	 SV->Channel_Blocks = 0;
	 SV->Handoff_Blocks = 0;
	 SV->Busy_Lines = 0;
	 SV->Handoff_Blocks = 0;
	 SV->CellLocationX = lp->id % NUM_CELLS_X;
	 SV->CellLocationY = lp->id / NUM_CELLS_X;

	 if (SV->CellLocationX >= NUM_CELLS_X ||
		 SV->CellLocationY >= NUM_CELLS_Y)
	 {
		 tw_error(TW_LOC, "Cell_StartUp: Bad CellLocations %d %d \n",
				  SV->CellLocationX, SV->CellLocationY);
	 }
	 SV->Portables_In = GenInitPortables(lp);

	 for (i = 0; i < SV->Portables_In; i++)
	 {
		 TMsg.CompletionCallTS = HUGE_VAL;

		 TMsg.MoveCallTS = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
		 TMsg.NextCallTS = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);

		 switch (Cell_MinTS(&TMsg))
		 {
		 case COMPLETECALL:
			 tw_error(TW_LOC, "APP_ERROR(StartUp): CompletionCallTS(%lf) Is Min \n",
					  TMsg.CompletionCallTS);
			 break;

		 case NEXTCALL:
			 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
			 CurEvent = tw_event_new(lp->gid, ts, lp);
			 TWMsg = (struct Msg_Data *) tw_event_data(CurEvent);
			 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
			 TWMsg->MoveCallTS = TMsg.MoveCallTS;
			 TWMsg->NextCallTS = TMsg.NextCallTS;
			 TWMsg->MethodName = NEXTCALL_METHOD;
			 tw_event_send(CurEvent);
			 break;

		 case MOVECALL:
			 newcell = lp->id;
			 while (TMsg.MoveCallTS < TMsg.NextCallTS)
			 {
				 double          result;

				 currentcell = newcell;
				 dest_index = tw_rand_integer(lp->rng, 0, 3);
				 newcell = Neighbors[currentcell][dest_index];
				 result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
				 TMsg.MoveCallTS += result;
			 }

			 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
			 CurEvent = tw_event_new(currentcell, ts, lp); 
			 TWMsg = tw_event_data(CurEvent);
			 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
			 TWMsg->MoveCallTS = TMsg.MoveCallTS;
			 TWMsg->NextCallTS = TMsg.NextCallTS;
			 TWMsg->MethodName = NEXTCALL_METHOD;
			 tw_event_send(CurEvent);
			 break;
		 }
	 }
 }

 void 
 Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             done, dest_index = 0;
	 int             currentcell = 0, newcell = 0;
	 tw_stime          ts;
	 struct Msg_Data TMsg;
	 struct Msg_Data *TWMsg;
	 tw_event       *CurEvent;
	 double          result;

	 TMsg.MethodName = M->MethodName;
	 TMsg.ChannelType = M->ChannelType;
	 TMsg.CompletionCallTS = M->CompletionCallTS;
	 TMsg.NextCallTS = M->NextCallTS;
	 TMsg.MoveCallTS = M->MoveCallTS;

	 SV->Call_Attempts++;

	 if ((CV->c1 = NORM_CH_BUSY))
	 {
		 SV->Channel_Blocks++;
		 result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
		 TMsg.NextCallTS += result;

		 switch (Cell_MinTS(&TMsg))
		 {
		 case COMPLETECALL:
			 tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
					  TMsg.CompletionCallTS);
			 break;

		 case NEXTCALL:
			 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
			 CurEvent = tw_event_new(lp->gid, ts, lp);
			 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
			 TWMsg->MethodName = TMsg.MethodName;
			 TWMsg->ChannelType = TMsg.ChannelType;
			 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
			 TWMsg->NextCallTS = TMsg.NextCallTS;
			 TWMsg->MoveCallTS = TMsg.MoveCallTS;
			 TWMsg->MethodName = NEXTCALL_METHOD;
			 tw_event_send(CurEvent);
			 break;

		 case MOVECALL:
			 newcell = lp->id;
			 while (TMsg.MoveCallTS < TMsg.NextCallTS)
			 {
				 M->RC.wl1++;
				 currentcell = newcell;
				 dest_index = tw_rand_integer(lp->rng, 0, 3);
				 newcell = Neighbors[currentcell][dest_index];
				 result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
				 TMsg.MoveCallTS += result;
			 }

			 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
			 CurEvent = tw_event_new((currentcell), ts, lp);
			 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
			 TWMsg->MethodName = TMsg.MethodName;
			 TWMsg->ChannelType = TMsg.ChannelType;
			 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
			 TWMsg->NextCallTS = TMsg.NextCallTS;
			 TWMsg->MoveCallTS = TMsg.MoveCallTS;
			 TWMsg->MethodName = NEXTCALL_METHOD;
			 tw_event_send(CurEvent);
			 break;
		 }
	 } else
	 {
		 SV->Normal_Channels--;
		 TMsg.ChannelType = NORMAL_CH;

		 result = tw_rand_exponential(lp->rng, CALL_TIME_MEAN);
		 TMsg.CompletionCallTS = result + TMsg.NextCallTS;
		 result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
		 TMsg.NextCallTS += result;
		 done = 0;
		 while (1)
		 {
			 switch (Cell_MinTS(&TMsg))
			 {
			 case COMPLETECALL:
				 ts = max(0.0, TMsg.CompletionCallTS - tw_now(lp));
				 CurEvent = tw_event_new(lp->gid, ts, lp);
				 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
				 TWMsg->MethodName = TMsg.MethodName;
				 TWMsg->ChannelType = TMsg.ChannelType;
				 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
				 TWMsg->NextCallTS = TMsg.NextCallTS;
				 TWMsg->MoveCallTS = TMsg.MoveCallTS;
				 TWMsg->MethodName = COMPLETIONCALL_METHOD;
				 tw_event_send(CurEvent);
				 done = 1;
				 break;

			 case NEXTCALL:
				 M->RC.wl1++;

				 SV->Busy_Lines++;
				 SV->Call_Attempts++;
				 result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
				 TMsg.NextCallTS += result;
				 break;

			 case MOVECALL:
				 ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
				 CurEvent = tw_event_new(lp->gid, ts, lp);
				 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
				 TWMsg->MethodName = TMsg.MethodName;
				 TWMsg->ChannelType = TMsg.ChannelType;
				 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
				 TWMsg->NextCallTS = TMsg.NextCallTS;
				 TWMsg->MoveCallTS = TMsg.MoveCallTS;
				 TWMsg->MethodName = MOVECALLOUT_METHOD;
				 tw_event_send(CurEvent);
				 done = 1;
				 break;
			 }
			 if (done)
				 break;
		 }
	 }
 }

 void 
 Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             dest_index = 0;
	 int             currentcell = 0, newcell = 0;
	 struct Msg_Data TMsg;
	 double          result;
	 tw_stime          ts;
	 struct Msg_Data *TWMsg;
	 tw_event       *CurEvent;

	 TMsg.MethodName = M->MethodName;
	 TMsg.ChannelType = M->ChannelType;
	 TMsg.CompletionCallTS = HUGE_VAL;
	 TMsg.NextCallTS = M->NextCallTS;
	 TMsg.MoveCallTS = M->MoveCallTS;

	 if ((CV->c1 = (NORMAL_CH == M->ChannelType)))
		 SV->Normal_Channels++;
	 else if ((CV->c2 = RESERVE == M->ChannelType))
		 SV->Reserve_Channels++;
	 else
	 {
		 tw_error(TW_LOC, "APP_ERROR(2): CompletionCall: Bad ChannelType(%d) \n",
				  M->ChannelType);
		 tw_exit(1);
	 }
	 if (SV->Normal_Channels > MAX_NORMAL_CHANNELS ||
		 SV->Reserve_Channels > MAX_RESERVE_CHANNELS)
	 {
		 tw_error(TW_LOC, "APP_ERROR(3): Normal_Channels(%d) > MAX %d OR Reserve_Channels(%d) > MAX %d \n",
			    SV->Normal_Channels, MAX_NORMAL_CHANNELS, SV->Reserve_Channels,
				  MAX_RESERVE_CHANNELS);
		 tw_exit(1);
	 }
	 TMsg.ChannelType = NONE;

	 switch (Cell_MinTS(&TMsg))
	 {
	 case COMPLETECALL:
		 tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
				  TMsg.CompletionCallTS);
		 tw_exit(1);
		 break;

	 case NEXTCALL:
		 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
		 CurEvent = tw_event_new(lp->gid, ts, lp);
		 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

		 TWMsg->MethodName = TMsg.MethodName;
		 TWMsg->ChannelType = TMsg.ChannelType;
		 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
		 TWMsg->NextCallTS = TMsg.NextCallTS;
		 TWMsg->MoveCallTS = TMsg.MoveCallTS;

		 TWMsg->MethodName = NEXTCALL_METHOD;
		 tw_event_send(CurEvent);
		 break;

	 case MOVECALL:
		 newcell = lp->id;
		 while (TMsg.MoveCallTS < TMsg.NextCallTS)
		 {
			 M->RC.wl1++;
			 currentcell = newcell;
			 dest_index = tw_rand_integer(lp->rng, 0, 3);
			 newcell = Neighbors[currentcell][dest_index];

			 result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
			 TMsg.MoveCallTS += result;
		 }
		 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
		 CurEvent = tw_event_new((currentcell), ts, lp); 
		 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
		 TWMsg->MethodName = TMsg.MethodName;
		 TWMsg->ChannelType = TMsg.ChannelType;
		 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
		 TWMsg->NextCallTS = TMsg.NextCallTS;
		 TWMsg->MoveCallTS = TMsg.MoveCallTS;
		 TWMsg->MethodName = NEXTCALL_METHOD;
		 tw_event_send(CurEvent);
		 break;
	 }
 }

 void 
 Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             done, dest_index = 0;
	 int             currentcell = 0, newcell = 0;
	 struct Msg_Data TMsg;
	 double          result;
	 tw_stime          ts;
	 tw_event       *CurEvent;

	 struct Msg_Data *TWMsg;

	 TMsg.MethodName = M->MethodName;
	 TMsg.ChannelType = M->ChannelType;
	 TMsg.CompletionCallTS = M->CompletionCallTS;
	 TMsg.NextCallTS = M->NextCallTS;
	 result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
	 TMsg.MoveCallTS = M->MoveCallTS + result;
	 if ((CV->c1 = (TMsg.CompletionCallTS != HUGE_VAL)))
	 {
		 if ((CV->c2 = (NORM_CH_BUSY && RESERVE_CH_BUSY)))
		 {
			 SV->Handoff_Blocks++;
			 TMsg.CompletionCallTS = HUGE_VAL;

			 switch (Cell_MinTS(&TMsg))
			 {
			 case COMPLETECALL:
				 tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
						  TMsg.CompletionCallTS);
				 tw_exit(1);
				 break;

			 case NEXTCALL:
				 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
				 CurEvent = tw_event_new(lp->gid, ts, lp);
				 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

				 TWMsg->MethodName = TMsg.MethodName;
				 TWMsg->ChannelType = TMsg.ChannelType;
				 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
				 TWMsg->NextCallTS = TMsg.NextCallTS;
				 TWMsg->MoveCallTS = TMsg.MoveCallTS;

				 TWMsg->MethodName = NEXTCALL_METHOD;
				 tw_event_send(CurEvent);
				 break;

			 case MOVECALL:
				 newcell = lp->id;
				 while (TMsg.MoveCallTS < TMsg.NextCallTS)
				 {
					 M->RC.wl1++;
					 currentcell = newcell;
					 dest_index = tw_rand_integer(lp->rng, 0, 3);
					 newcell = Neighbors[currentcell][dest_index];
					 result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
					 TMsg.MoveCallTS += result;
				 }
				 ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
				 CurEvent = tw_event_new((currentcell), ts, lp);
				 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
				 TWMsg->MethodName = TMsg.MethodName;
				 TWMsg->ChannelType = TMsg.ChannelType;
				 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
				 TWMsg->NextCallTS = TMsg.NextCallTS;
				 TWMsg->MoveCallTS = TMsg.MoveCallTS;
				 TWMsg->MethodName = NEXTCALL_METHOD;
				 tw_event_send(CurEvent);
				 break;
			 }
		 } else
		 {
			 if ((CV->c3 = !NORM_CH_BUSY))
			 {
				 SV->Normal_Channels--;
				 TMsg.ChannelType = NORMAL_CH;
			 } else
			 {
				 SV->Reserve_Channels--;
				 TMsg.ChannelType = RESERVE;
			 }
			 done = 0;
			 while (1)
			 {
				 switch (Cell_MinTS(&TMsg))
				 {
				 case COMPLETECALL:
					 ts = max(0.0, TMsg.CompletionCallTS - tw_now(lp));
					 CurEvent = tw_event_new(lp->gid, ts, lp);
					 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

					 TWMsg->MethodName = TMsg.MethodName;
					 TWMsg->ChannelType = TMsg.ChannelType;
					 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
					 TWMsg->NextCallTS = TMsg.NextCallTS;
					 TWMsg->MoveCallTS = TMsg.MoveCallTS;

					 TWMsg->MethodName = COMPLETIONCALL_METHOD;
					 tw_event_send(CurEvent);
					 done = 1;
					 break;

				 case NEXTCALL:
					 M->RC.wl1++;
					 SV->Busy_Lines++;
					 SV->Call_Attempts++;
					 result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
					 TMsg.NextCallTS += result;
					 break;

				 case MOVECALL:
					 ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
					 CurEvent = tw_event_new(lp->gid, ts, lp);
					 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

					 TWMsg->MethodName = TMsg.MethodName;
					 TWMsg->ChannelType = TMsg.ChannelType;
					 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
					 TWMsg->NextCallTS = TMsg.NextCallTS;
					 TWMsg->MoveCallTS = TMsg.MoveCallTS;

					 TWMsg->MethodName = MOVECALLOUT_METHOD;
					 tw_event_send(CurEvent);
					 done = 1;
					 break;
				 }
				 if (done)
					 break;
			 }
		 }
	 } else
	 {
		 tw_error(TW_LOC, "APP_ERROR(11): MoveCallIn: Got MoveCallIn Event W/O Call In Progress!! \n");
		 tw_exit(1);
	 }
 }

 void 
 Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             dest_index;
	 int             newcell;
	 struct Msg_Data TMsg;

	 tw_stime          ts;

	 struct Msg_Data *TWMsg;
	 tw_event       *CurEvent;

	 TMsg.MethodName = M->MethodName;
	 TMsg.ChannelType = M->ChannelType;
	 TMsg.CompletionCallTS = M->CompletionCallTS;
	 TMsg.NextCallTS = M->NextCallTS;
	 TMsg.MoveCallTS = M->MoveCallTS;

	 if ((CV->c1 = TMsg.CompletionCallTS != HUGE_VAL))
	 {
		 if ((CV->c2 = NORMAL_CH == M->ChannelType))
			 SV->Normal_Channels++;
		 else if (RESERVE == M->ChannelType)
			 SV->Reserve_Channels++;
		 else
		 {
			 tw_error(TW_LOC, "APP_ERROR(7): MoveCallOut: Bad ChannelType(%d) \n",
					  M->ChannelType);
			 tw_exit(1);
		 }
		 TMsg.ChannelType = NONE;
	 } else
	 {
		 tw_error(TW_LOC, "APP_ERROR(9): MoveCallOut: NOT IN CALL: SHOULD NOT BE HERE!! \n");
		 tw_exit(1);
	 }

	 ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
	 TMsg.MethodName = MOVECALLIN_METHOD;
	 dest_index = tw_rand_integer(lp->rng, 0, 3);
	 newcell = Neighbors[lp->id][dest_index];
	 CurEvent = tw_event_new((newcell), ts, lp);
	 TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
	 TWMsg->MethodName = TMsg.MethodName;
	 TWMsg->ChannelType = TMsg.ChannelType;
	 TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
	 TWMsg->NextCallTS = TMsg.NextCallTS;
	 TWMsg->MoveCallTS = TMsg.MoveCallTS;
	 tw_event_send(CurEvent);
 }

 void 
 Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
 #ifdef LPTRACEON
	 long            seeds[4];
 #endif
	 *(int *)CV = (int)0;
	 M->RC.wl1 = 0;

	 switch (M->MethodName)
	 {
	 case NEXTCALL_METHOD:
		 Cell_NextCall(SV, CV, M, lp);
		 break;
	 case COMPLETIONCALL_METHOD:
		 Cell_CompletionCall(SV, CV, M, lp);
		 break;
	 case MOVECALLIN_METHOD:
		 Cell_MoveCallIn(SV, CV, M, lp);
		 break;
	 case MOVECALLOUT_METHOD:
		 Cell_MoveCallOut(SV, CV, M, lp);
		 break;
	 default:
		 tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d)\n",
				  lp->id, M->MethodName);
		 tw_exit(1);
	 }

 #ifdef LPTRACEON
	 rng_get_state(lp->id, seeds);
	 fprintf(LPTrace[lp->id], "CE: Type %d: Time %f: %d %d %d %d \n", M->MethodName,
			 tw_now(lp), seeds[0], seeds[1], seeds[2], seeds[3]);
 #endif
 }

 void 
 RC_Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             i;

	 SV->Call_Attempts--;
	 if (CV->c1)
	 {
		 SV->Channel_Blocks--;
		 tw_rand_reverse_unif(lp->rng);
		 for (i = 0; i < M->RC.wl1; i++)
		 {
			 tw_rand_reverse_unif(lp->rng);
			 tw_rand_reverse_unif(lp->rng);
		 }
	 } else
	 {
		 SV->Normal_Channels++;
		 tw_rand_reverse_unif(lp->rng);
		 tw_rand_reverse_unif(lp->rng);
		 for (i = 0; i < M->RC.wl1; i++)
		 {
			 SV->Busy_Lines--;
			 SV->Call_Attempts--;
			 tw_rand_reverse_unif(lp->rng);
		 }
	 }
 }

 void 
 RC_Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             i;

	 if (CV->c1)
		 SV->Normal_Channels--;
	 else if (CV->c2)
		 SV->Reserve_Channels--;

	 for (i = 0; i < M->RC.wl1; i++)
	 {
		 tw_rand_reverse_unif(lp->rng);
		 tw_rand_reverse_unif(lp->rng);
	 }
 }

 void 
 RC_Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 int             i;

	 tw_rand_reverse_unif(lp->rng);
	 if (CV->c1)
	 {
		 if (CV->c2)
		 {
			 SV->Handoff_Blocks--;

			 for (i = 0; i < M->RC.wl1; i++)
			 {
				 tw_rand_reverse_unif(lp->rng);
				 tw_rand_reverse_unif(lp->rng);
			 }
		 } else
		 {
			 if (CV->c3)
				 SV->Normal_Channels++;
			 else
				 SV->Reserve_Channels++;
			 for (i = 0; i < M->RC.wl1; i++)
			 {
				 SV->Busy_Lines--;
				 SV->Call_Attempts--;
				 tw_rand_reverse_unif(lp->rng);
			 }
		 }
	 }
 }

 void 
 RC_Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
 {
	 if (CV->c1)
	 {
		 if (CV->c2)
						SV->Normal_Channels--;
		else
			SV->Reserve_Channels--;
	}
	tw_rand_reverse_unif(lp->rng);
}


void 
RC_Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
#ifdef LPTRACEON
	long            seeds[4];
#endif

	switch (M->MethodName)
	{
	case NEXTCALL_METHOD:
		RC_Cell_NextCall(SV, CV, M, lp);
		break;
	case COMPLETIONCALL_METHOD:
		RC_Cell_CompletionCall(SV, CV, M, lp);
		break;
	case MOVECALLIN_METHOD:
		RC_Cell_MoveCallIn(SV, CV, M, lp);
		break;
	case MOVECALLOUT_METHOD:
		RC_Cell_MoveCallOut(SV, CV, M, lp);
		break;
	}

#ifdef LPTRACEON
	rng_get_state(lp->id, seeds);
	fprintf(LPTrace[lp->id], "RC_CE: Type %d: %d %d %d %d \n", M->MethodName,
			seeds[0], seeds[1], seeds[2], seeds[3]);
#endif
}

void 
CellStatistics_CollectStats(struct State *SV, tw_lp * lp)
{
	TWAppStats.Call_Attempts += SV->Call_Attempts;
	TWAppStats.Channel_Blocks += SV->Channel_Blocks;
	TWAppStats.Busy_Lines += SV->Busy_Lines;
	TWAppStats.Handoff_Blocks += SV->Handoff_Blocks;
	TWAppStats.Portables_In += SV->Portables_In;
	TWAppStats.Portables_Out += SV->Portables_Out;
}

void 
CellStatistics_Compute(struct CellStatistics *CS)
{
	CS->Blocking_Probability = ((double)CS->Channel_Blocks + (double)CS->Handoff_Blocks) /
		((double)CS->Call_Attempts - (double)CS->Busy_Lines);
}

void 
CellStatistics_Print(struct CellStatistics *CS)
{
	printf("Call Attempts......................................%d\n",
		   CS->Call_Attempts);
	printf("Channel Blocks.....................................%d\n",
		   CS->Channel_Blocks);
	printf("Busy Lines.........................................%d\n",
		   CS->Busy_Lines);
	printf("Handoff Blocks.....................................%d\n",
		   CS->Handoff_Blocks);
	printf("Portables In.......................................%d\n",
		   CS->Portables_In);
	printf("Portables Out......................................%d\n",
		   CS->Portables_Out);
	printf("Blocking Probability...............................%f\n",
		   CS->Blocking_Probability);
}

void 
FindMostSquare(int *xpe, int *ype)
{
	int             x, y, npe;

	*xpe = g_tw_npe;
	*ype = 1;

	npe = (int)sqrt((double)g_tw_npe);

	if (npe * npe == g_tw_npe)
	{
		*xpe = npe;
		*ype = npe;
	} else
	{
		x = npe;
		y = npe + 1;

		while (x * y != g_tw_npe)
		{
			y++;
			if (y == g_tw_npe + 1)
			{
				x--;
				y = npe + 1;
				if (x == 0)
					break;
			}
		}
		*xpe = x;
		*ype = y;
	}
#if 0
	printf("The Most Square Processor Grid for %d PEs is %d x %d\n",
		   g_tw_npe, *xpe, *ype);
#endif
}

/******** Initialize_Appl *************************************************/

#define	TW_CELL	1

tw_lptype       mylps[] =
{
	{
	 (init_f) Cell_StartUp,
	 (event_f) Cell_EventHandler,
	 (revent_f) RC_Cell_EventHandler,
	 (final_f) CellStatistics_CollectStats,
	 (map_f) mapping,
	 sizeof(struct State)
	},
	{0},
};

int
main(int argc, char **argv)
{
	int             x, y, xvp, yvp;
	int             neighbor_x[4], neighbor_y[4];
	int             vp_per_proc;
	int             i;
	int             TWnlp;
	int             TWnkp;
	int             TWnpe;
	tw_lp          *lp;
	tw_kp          *kp;
        int             num_cells_per_kp;
        int             additional_memory_buffers;

printf("Enter TWnpe, TWnkp, additional_memory_buffers \n" );
	scanf("%d %d %d", 
               &TWnpe, &TWnkp, &additional_memory_buffers );

	tw_init(&argc, &argv);

	TWnlp = NUM_CELLS_X * NUM_CELLS_Y;
	g_tw_events_per_pe = (TWnlp * (int)BIG_N)/TWnpe + 
                             additional_memory_buffers;

	printf("Running simulation with following configuration: \n" );
        printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
	printf("\n\n");
 
        num_cells_per_kp = TWnlp / TWnkp;
	vp_per_proc = ((double)NUM_VP_X * (double)NUM_VP_Y) / (double)TWnpe;

	tw_define_lps(nlp_per_pe, sizeof(struct Msg_Data), 0);

	for (x = 0; x < NUM_CELLS_X; x++)
	{
		for (y = 0; y < NUM_CELLS_Y; y++)
		{
			neighbor_x[0] = ((x - 1) + NUM_CELLS_X) % NUM_CELLS_X;
			neighbor_y[0] = y;

			neighbor_x[1] = (x + 1) % NUM_CELLS_X;
			neighbor_y[1] = y;

			neighbor_x[2] = x;
			neighbor_y[2] = ((y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;

			neighbor_x[3] = x;
			neighbor_y[3] = (y + 1) % NUM_CELLS_Y;

			for (i = 0; i < 4; i++)
				Neighbors[x + (y * NUM_CELLS_X)][i] = neighbor_x[i] + (neighbor_y[i] * NUM_CELLS_X);

			xvp = (int)((double)x * ((double)NUM_VP_X / (double)NUM_CELLS_X));
			yvp = (int)((double)y * ((double)NUM_VP_Y / (double)NUM_CELLS_Y));

			lp = tw_getlp(x + (y * NUM_CELLS_X));
			kp = tw_getkp((x + (y * NUM_CELLS_X))/num_cells_per_kp);

			tw_lp_settype(x + (y * NUM_CELLS_X), &mylps[0]);
                        tw_lp_onkp(lp, kp);
			tw_lp_onpe(lp, tw_getpe((xvp + (yvp * NUM_VP_X)) / vp_per_proc));
			tw_kp_onpe(kp, tw_getpe((xvp + (yvp * NUM_VP_X)) / vp_per_proc));
		}
	}

	/*
	 * Initialize App Stats Structure 
	 */
	TWAppStats.Call_Attempts = 0;
	TWAppStats.Call_Attempts = 0;
	TWAppStats.Channel_Blocks = 0;
	TWAppStats.Busy_Lines = 0;
	TWAppStats.Handoff_Blocks = 0;
	TWAppStats.Portables_In = 0;
	TWAppStats.Portables_Out = 0;
	TWAppStats.Blocking_Probability = 0.0;

	/*
	 * Some some of the settings.
	 */
	printf("MOVE CALL MEAN   = %f\n", MOVE_CALL_MEAN);
	printf("NEXT CALL MEAN   = %f\n", NEXT_CALL_MEAN);
	printf("CALL TIME MEAN   = %f\n", CALL_TIME_MEAN);
	printf("NUM CELLS X      = %d\n", NUM_CELLS_X);
	printf("NUM CELLS Y      = %d\n", NUM_CELLS_Y);
	fflush(stdout);

	tw_run();

	CellStatistics_Compute(&TWAppStats);
	CellStatistics_Print(&TWAppStats);

	return 0;
}
