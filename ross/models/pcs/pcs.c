#include "pcs.h"

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

tw_lpid g_vp_per_proc =0; // set in main
tw_lpid g_cells_per_vp_x = NUM_CELLS_X/NUM_VP_X;
tw_lpid g_cells_per_vp_y = NUM_CELLS_Y/NUM_VP_Y;
tw_lpid g_cells_per_vp = (NUM_CELLS_X/NUM_VP_X)*(NUM_CELLS_Y/NUM_VP_Y);

tw_lpid Cell_ComputeMove( tw_lpid lpid, int direction )
{
  tw_lpid lpid_x, lpid_y;
  tw_lpid n_x, n_y;
  tw_lpid dest_lpid;

  lpid_y = lpid / NUM_CELLS_X;
  lpid_x = lpid - (lpid_y * NUM_CELLS_X);

  switch( direction )
    {
    case 0:
      n_x = ((lpid_x - 1) + NUM_CELLS_X) % NUM_CELLS_X;
      n_y = lpid_y;
      break;

    case 1:
      n_x = (lpid_x + 1) % NUM_CELLS_X;
      n_y = lpid_y;
      break;

    case 2:
      n_x = lpid_x;
      n_y = ((lpid_y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;
      break;

    case 3:
      n_x = lpid_x;
      n_y = (lpid_y + 1) % NUM_CELLS_Y;
      break;
      
    default:
      tw_error( TW_LOC, "Bad direction value \n");
    }

  dest_lpid = (tw_lpid) (n_x + (n_y * NUM_CELLS_X));
  // printf("ComputeMove: Src LP %llu (%d, %d), Dir %u, Dest LP %llu (%d, %d)\n", lpid, lpid_x, lpid_y, direction, dest_lpid, n_x, n_y);
  return( dest_lpid );
}

tw_peid
CellMapping_lp_to_pe(tw_lpid lpid) 
{
  long lp_x = lpid % NUM_CELLS_X;
  long lp_y = lpid / NUM_CELLS_X;
  long vp_num_x = lp_x/g_cells_per_vp_x;
  long vp_num_y = lp_y/g_cells_per_vp_y;
  long vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  tw_peid peid = vp_num/g_vp_per_proc;  
  return peid;
}

tw_lp *CellMapping_to_lp(tw_lpid lpid) 
{
  tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
  tw_lpid lp_y = lpid / NUM_CELLS_X;
  tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
  tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
  tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
  tw_lpid vp_num_x = lp_x/g_cells_per_vp_x;
  tw_lpid vp_num_y = lp_y/g_cells_per_vp_y;
  tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  vp_num = vp_num % g_vp_per_proc;
  tw_lpid index = vp_index + vp_num*g_cells_per_vp;

#ifdef ROSS_runtime_check  
  if( index >= g_tw_nlp )
    tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);
#endif /* ROSS_runtime_check */
  
  return g_tw_lp[index];
}

tw_lpid CellMapping_to_local_index(tw_lpid lpid) 
{
  tw_lpid lp_x = lpid % NUM_CELLS_X; //lpid -> (lp_x,lp_y)
  tw_lpid lp_y = lpid / NUM_CELLS_X;
  tw_lpid vp_index_x = lp_x % g_cells_per_vp_x;
  tw_lpid vp_index_y = lp_y % g_cells_per_vp_y;
  tw_lpid vp_index = vp_index_x + (vp_index_y * (g_cells_per_vp_x));
  tw_lpid vp_num_x = lp_x/g_cells_per_vp_x;
  tw_lpid vp_num_y = lp_y/g_cells_per_vp_y;
  tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  vp_num = vp_num % g_vp_per_proc;
  tw_lpid index = vp_index + vp_num*g_cells_per_vp;
  
  if( index >= g_tw_nlp )
    tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);
  
  return( index );
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
	  newcell = lp->gid;
	  while (TMsg.MoveCallTS < TMsg.NextCallTS)
	    {
	      double          result;

	      currentcell = newcell;
	      dest_index = tw_rand_integer(lp->rng, 0, 3);
	      newcell = Cell_ComputeMove( currentcell, dest_index ); // Neighbors[currentcell][dest_index];
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
	  newcell = lp->gid;
	  while (TMsg.MoveCallTS < TMsg.NextCallTS)
	    {
	      M->RC.wl1++;
	      currentcell = newcell;
	      dest_index = tw_rand_integer(lp->rng, 0, 3);
	      newcell = Cell_ComputeMove( currentcell, dest_index ); //Neighbors[currentcell][dest_index];
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
      newcell = lp->gid;
      while (TMsg.MoveCallTS < TMsg.NextCallTS)
	{
	  M->RC.wl1++;
	  currentcell = newcell;
	  dest_index = tw_rand_integer(lp->rng, 0, 3);
	  newcell = Cell_ComputeMove( currentcell, dest_index ); //Neighbors[currentcell][dest_index];

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
	      newcell = lp->gid;
	      while (TMsg.MoveCallTS < TMsg.NextCallTS)
		{
		  M->RC.wl1++;
		  currentcell = newcell;
		  dest_index = tw_rand_integer(lp->rng, 0, 3);
		  newcell = Cell_ComputeMove( currentcell, dest_index ); // Neighbors[currentcell][dest_index];
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
  newcell = Cell_ComputeMove( lp->gid, dest_index ); //Neighbors[lp->id][dest_index];
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
      (map_f) CellMapping_lp_to_pe,
      sizeof(struct State)
    },
    {0},
  };

void pcs_grid_mapping()
{
  tw_lpid         x, y;
  tw_lpid         lpid, kpid;
  tw_lpid         num_cells_per_kp, vp_per_proc;
  tw_lpid         local_lp_count;
  tw_lpid         local_kp_count;

  num_cells_per_kp = (NUM_CELLS_X * NUM_CELLS_Y) / (NUM_VP_X * NUM_VP_Y);
  vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
  g_tw_nlp = nlp_per_pe;
  g_tw_nkp = vp_per_proc;

  local_lp_count=0;
  for (y = 0; y < NUM_CELLS_Y; y++)
    {
      for (x = 0; x < NUM_CELLS_X; x++)
	{
	  lpid = (x + (y * NUM_CELLS_X));
	  if( g_tw_mynode == CellMapping_lp_to_pe(lpid) )
	    {
	   
	      kpid = local_lp_count/num_cells_per_kp;
	      local_lp_count++; // MUST COME AFTER!! DO NOT PRE-INCREMENT ELSE KPID is WRONG!!

	      if( kpid >= g_tw_nkp )
		tw_error(TW_LOC, "Attempting to mapping a KPid (%llu) for Global LPid %llu that is beyond g_tw_nkp (%llu)\n",
			 kpid, lpid, g_tw_nkp );

	      tw_lp_onpe(CellMapping_to_local_index(lpid), g_tw_pe[0], lpid);
	      if( g_tw_kp[kpid] == NULL )
		tw_kp_onpe(kpid, g_tw_pe[0]);
	      tw_lp_onkp(g_tw_lp[CellMapping_to_local_index(lpid)], g_tw_kp[kpid]);
	      tw_lp_settype( CellMapping_to_local_index(lpid), &mylps[0]);
	    }
	}
    }
}

int
main(int argc, char **argv)
{
  tw_lpid         num_cells_per_kp, vp_per_proc;
  unsigned int    additional_memory_buffers;
  
  // printf("Enter TWnpe, TWnkp, additional_memory_buffers \n" );
  // scanf("%d %d %d", 
  //	&TWnpe, &TWnkp, &additional_memory_buffers );

  tw_init(&argc, &argv);

  nlp_per_pe = (NUM_CELLS_X * NUM_CELLS_Y) / (tw_nnodes() * g_tw_npe);
  additional_memory_buffers = 2 * g_tw_mblock * g_tw_gvt_interval;

  g_tw_events_per_pe = (nlp_per_pe * (unsigned int)BIG_N) + 
    additional_memory_buffers;

  if( tw_ismaster() )
    {
      printf("Running simulation with following configuration: \n" );
      printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
      printf("\n\n");
    }
 
  num_cells_per_kp = (NUM_CELLS_X * NUM_CELLS_Y) / (NUM_VP_X * NUM_VP_Y);
  vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
  g_vp_per_proc = vp_per_proc;
  g_tw_nlp = nlp_per_pe;
  g_tw_nkp = vp_per_proc;

  g_tw_mapping = CUSTOM;
  g_tw_custom_initial_mapping = &pcs_grid_mapping;
  g_tw_custom_lp_global_to_local_map = &CellMapping_to_lp;

  /*
   * Some some of the settings.
   */
  if( tw_ismaster() )
    {
      printf("\n\n");
      printf("/**********************************************/\n");
      printf("MOVE CALL MEAN   = %f\n", MOVE_CALL_MEAN);
      printf("NEXT CALL MEAN   = %f\n", NEXT_CALL_MEAN);
      printf("CALL TIME MEAN   = %f\n", CALL_TIME_MEAN);
      printf("NUM CELLS X      = %d\n", NUM_CELLS_X);
      printf("NUM CELLS Y      = %d\n", NUM_CELLS_Y);
      printf("NUM KPs per PE   = %llu \n", g_tw_nkp);
      printf("NUM LPs per PE   = %llu \n", g_tw_nlp);
      printf("g_vp_per_proc    = %llu \n", g_vp_per_proc);
      printf("/**********************************************/\n");
      printf("\n\n");
      fflush(stdout);
    }

  tw_define_lps(nlp_per_pe, sizeof(struct Msg_Data), 0);

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

  tw_run();

  if( tw_ismaster() )
    {
      CellStatistics_Compute(&TWAppStats);
      CellStatistics_Print(&TWAppStats);
    }

  tw_end();

  return 0;
}
