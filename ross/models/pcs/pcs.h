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

#include "ross.h"

#define TW_MAX_NAME_LEN 31

#define NUM_CELLS_X 1024     //256
#define NUM_CELLS_Y 1024     //256

#define NUM_VP_X 64 
#define NUM_VP_Y 64 

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
