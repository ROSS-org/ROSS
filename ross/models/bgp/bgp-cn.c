/*
 *  Blue Gene/P model
 *  Compute node
 *  Ning Liu 
 */
#include "bgp.h"

void bgp_parse_cf(int task_rank, bgp_app_cf_info_t * task_info)
{
  int foundit = 0;
  char line[8192];
  FILE * ikmp = NULL;

  /* open the config file */
  ikmp = fopen(io_kernel_meta_path, "r");

  /* for each line in the config file */
  while(fgets(line, 8192, ikmp) != NULL)
    {

      char * token = NULL;
      int min = 0;
      int max = 0;
      int gid = 0;
      int len = strlen(line);
      char * ctx = NULL;

      /* parse the first element... the gid */
      token = strtok_r(line, " \n", &ctx);
      if(token)
	gid = atoi(token);

      if(gid == BGP_DEFAULT_GID)
	{
	  fprintf(stderr, 
		  "%s:%i incorrect GID detected in kernel metafile. Cannot use the reserved GID BGP_DEFAULT_GID(%i)\n", 
		  __func__, 
		  __LINE__,
		  BGP_DEFAULT_GID);
	}


      /* parse the first element... min rank */
      token = strtok_r(NULL, " \n", &ctx);
      if(token)
	min = atoi(token);

      /* parse the second element... max rank */
      token = strtok_r(NULL, " \n", &ctx);
      if(token)
	max = atoi(token);

      /* parse the last element... kernel path */
      token = strtok_r(NULL, " \n", &ctx);
      if(token)
	strcpy(io_kernel_path, token);

      /* if our rank is on this range... end processing of the config file */
      if(task_rank >= min && task_rank <= max)
	{
	  task_info->gid = gid;
	  task_info->min = min;
	  task_info->max = max;
	  task_info->lrank = task_rank - min;
	  task_info->num_lrank = max - min + 1;
 
	  foundit = 1;
	  break;
	}

    }

  /* close the config file */
  fclose(ikmp);

  /* if we did not find the config file, set it to the default */
  if(foundit == 0)
    strcpy(io_kernel_path, io_kernel_def_path);

  return;
}


int codeslang_parse_input(CodesIOKernel_pstate * ps, CodesIOKernelContext * c,
			  codeslang_bgp_inst * inst)
{
  int yychar;
  int status;
  int codes_inst = CL_NOOP;

#ifdef CODESLANG_DEBUG
  fprintf(stderr, "get next instruction\n");
  fflush(stderr);
#endif
  CodesIOKernelScannerSetSymTable(c);

        do
	  {
            c->locval = CodesIOKernel_get_lloc(*((yyscan_t *)c->scanner_));
            yychar = CodesIOKernel_lex(c->lval, c->locval, c->scanner_);
            c->locval = NULL;
            if(yychar == WRITEAT)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s writeat\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_WRITEAT;
	      }
#if ENABLE_ALL_CODES_INST
            else if(yychar == GETGROUPRANK)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s getgrouprank\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_GETRANK;
	      }
#endif
#if ENABLE_ALL_CODES_INST
            else if(yychar == GETGROUPSIZE)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s getgroupsize\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_GETSIZE;
	      }
#endif
            else if(yychar == CLOSE)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s close\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_CLOSE;
	      }
	    else if(yychar == OPEN)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s open\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_OPEN;
	      }
            else if(yychar == SYNC)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s sync\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_SYNC;
	      }
	    //#if ENABLE_ALL_CODES_INST
            else if(yychar == SLEEP)
	      {
		#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s sleep\n", __func__);
                fflush(stderr);
		#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_SLEEP;
	      }
	    //#endif
            else if(yychar == EXIT)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s exit\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_EXIT;
	      }
#if ENABLE_ALL_CODES_INST
            else if(yychar == DELETE)
	      {
#ifdef CODESLANG_DEBUG
                fprintf(stderr, "%s delete\n", __func__);
                fflush(stderr);
#endif
                c->inst_ready = 0;
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
                codes_inst = CL_DELETE;
	      }
#endif
            else
	      {
                status = CodesIOKernel_push_parse(ps, yychar, c->lval, c->locval, c);
	      }

	    if(c->inst_ready)
	      {
                c->inst_ready = 0;
                if(codes_inst == CL_GETRANK)
		  {
                    inst->event_type = CL_GETRANK;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1]= c->var[2];
		  }
                else if(codes_inst == CL_GETSIZE)
		  {
                    inst->event_type = CL_GETSIZE;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1]= c->var[2];
		  }
                else if(codes_inst == CL_WRITEAT)
		  {
                    inst->event_type = CL_WRITEAT;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
                    inst->var[2] = c->var[3];
                    /* fprintf(stderr, "INST WRITEAT %li %li %li %li\n", c->var[0], */
                    /*         c->var[1], c->var[2], c->var[3]); */
                    /* fflush(stderr); */
		  }
                else if(codes_inst == CL_OPEN)
		  {
                    inst->event_type = CL_OPEN;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
                    /* fprintf(stderr, "INST OPEN %li %li\n", c->var[0], c->var[1]); */
                    /* fflush(stderr); */
		  }
                else if(codes_inst == CL_CLOSE)
		  {
                    inst->event_type = CL_CLOSE;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
                    /* fprintf(stderr, "INST CLOSE %li %li\n", c->var[0], c->var[1]); */
                    /* fflush(stderr); */
		  }
                else if(codes_inst == CL_SYNC)
		  {
                    inst->event_type = CL_SYNC;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
		  }
                else if(codes_inst == CL_EXIT)
		  {
                    inst->event_type = CL_EXIT;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
                    /* fprintf(stderr, "INST EXIT %li %li\n", c->var[0], */
                    /*         c->var[1]); */
                    fflush(stderr);
		  }
                else if(codes_inst == CL_DELETE)
		  {
                    inst->event_type = CL_DELETE;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    inst->var[1] = c->var[2];
		  }
                else if(codes_inst == CL_SLEEP)
		  {
                    inst->event_type = CL_SLEEP;
                    inst->num_var = c->var[0];
                    inst->var[0] = c->var[1];
                    //inst->var[1] = c->var[2];
		    //printf("sleep **  num var ll%d, func var %lld\n",inst->num_var,inst->var[0]);
		  }
                else
		  {
                    continue;
                    fprintf(stderr, "INST OTHER\n");
                    fflush(stderr);
		  }
                break;
	      }

	  }while(status == YYPUSH_MORE);

        return codes_inst;
}


void bgp_cn_init( CN_state* s,  tw_lp* lp )
{

  tw_event *e;
  tw_stime ts;
  MsgData *m;

  int N_PE = tw_nnodes();

  nlp_DDN = NumDDN / N_PE;
  nlp_Controller = nlp_DDN * NumControllerPerDDN;
  nlp_FS = nlp_Controller * NumFSPerController;
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION;

  int basePE = lp->gid/nlp_per_pe;
  // CN ID in the local process
  int localID = lp->gid - basePE * nlp_per_pe;
  int basePset = localID/N_CN_per_ION;
  s->CN_ID_in_tree = localID % N_CN_per_ION;

  s->CN_ID = basePE * nlp_CN + basePset * N_CN_per_ION + s->CN_ID_in_tree;
  
  s->tree_next_hop_id = basePE * nlp_per_pe + nlp_CN + basePset;
  s->sender_next_available_time = 0;

  N_ION_active = min (N_ION_active, nlp_ION*N_PE);

#ifdef PRINTid
  printf("CN %d local ID is %d next hop is %d and CN ID is %d\n", 
	 lp->gid,
	 s->CN_ID_in_tree,
	 s->tree_next_hop_id,
	 s->CN_ID );
#endif

  //CONT message stream
  // avoid 0 time stamp events

  // configure step
  // each CN tells root CN its lp ID and ID in CNs
  // root CN register all these 
  // sync up at this step

  s->bandwidth = 10000000000;
  s->compute_node = (int *)calloc(nlp_CN*N_PE, sizeof(int));
  s->time_stamp = (double *)calloc(N_checkpoint, sizeof(double));
  s->sync_counter = 0;
  s->checkpoint_counter = 0;
  s->write_counter = 0;
  s->cumulated_data_size = 0;
  s->w_counter = 0;

  /* get the kernel from the file */
  bgp_parse_cf(s->CN_ID,&(s->task_info));

  // reverse compute the group master's lp->gid
  // group master is the task_info.min
  int a,b,c,d,ee,local_root_gid;
  a = s->task_info.min;
  b = a/N_CN_per_ION;// b is the # ION
  c = b/N_ION_per_FS;// c is the # FS
  d = c/NumFSPerController; //d is the # controller
  ee = d/NumControllerPerDDN; //e is the # DDN
  local_root_gid = a+b+c+d+ee;
  s->local_root_gid = local_root_gid;

  /* printf("My lp id is %d and a is %d b is %d c is %d d is %d e is %d and local root gid is %d\n", */
  /* 	 lp->gid,  */
  /* 	 a, */
  /* 	 b, */
  /* 	 c, */
  /* 	 d, */
  /* 	 ee, */
  /* 	 local_root_gid); */

  int i;

  for (i=0; i<N_sample; i++)
    {
      s->committed_size1[i] = 0;
      s->committed_size2[i] = 0;
      s->committed_size3[i] = 0;
    }

  ts = s->CN_ID_in_tree;

  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = PRE_CONFIGURE;

  m->collective_group_rank = s->CN_ID;
  m->message_CN_source = lp->gid;

      int t = CL_NOOP;

      int ret = 0;
      char kbuffer[32768];
      int fd = 0;

      /* m->MC.group_id = s->task_info.gid; */
      /* m->MC.num_in_group = s->task_info.num_lrank; */
      /* //printf("group id is %d\n",m->MC.group_id); */
      /* m->MC.Local_CN_root = local_root_gid; */

      
      /* fd = open(io_kernel_path, O_RDONLY); */
      /* ret = pread(fd, kbuffer, 32768, 0); */
      /* close(fd); */

      /* CodesIOKernelScannerInit(&(s->c)); */
      /* if(ret < 32768) */
      /* 	{ */
      /* 	  //	  fprintf(stderr, "read %i bytes of the kernel\n", ret); */


      /* 	  CodesIOKernel__scan_string(kbuffer, s->c.scanner_); */

      /* 	  s->ps = CodesIOKernel_pstate_new(); */

      /* 	  (s->c).GroupRank=s->CN_ID; */
      /* 	  (s->c).GroupSize=N_ION_active * N_CN_per_ION; */

      /* 	  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event)); */

      /* 	} */
      /* else */
      /* 	{ */
      /* 	  fprintf(stderr, "not enough buffer space... bail\n"); */
      /* 	} */

      tw_event_send(e);

}

void cn_pre_configure( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  ts = s->CN_ID_in_tree;

  e = tw_event_new( s->local_root_gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = CONFIGURE;

  m->collective_group_rank = s->CN_ID;
  m->message_CN_source = lp->gid;

      int t = CL_NOOP;

      int ret = 0;
      char kbuffer[40000];
      int fd = 0;

      m->MC.group_id = s->task_info.gid;
      m->MC.num_in_group = s->task_info.num_lrank;
      //printf("group id is %d\n",m->MC.group_id);
      m->MC.Local_CN_root = s->local_root_gid;

      fd = open(io_kernel_path, O_RDONLY);
      ret = pread(fd, kbuffer, 40000, 0);
      close(fd);

      CodesIOKernelScannerInit(&(s->c));
      if(ret < 40000)
	{
	  //	  fprintf(stderr, "read %i bytes of the kernel\n", ret);

	  /* load the string into the parser */
	  CodesIOKernel__scan_string(kbuffer, s->c.scanner_);

	  s->ps = CodesIOKernel_pstate_new();

	  (s->c).GroupRank=s->CN_ID;
	  (s->c).GroupSize=N_ION_active * N_CN_per_ION;

	  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event));

	}
      else
	{
	  fprintf(stderr, "not enough buffer space... bail\n");
	}

      if(t==CL_OPEN)
      	{
	  tw_event_send(e);
	}

}


void cn_configure( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  s->sync_counter++;
  // register cn id
  //printf("msg local cn id is %d\n",msg->MC.Local_CN_root);
  
  s->compute_node[s->sync_counter-1] = msg->message_CN_source;
  //?
  //s->compute_node[msg->collective_group_rank] = msg->message_CN_source;
  

  //local configure instead of global
  if(s->sync_counter==s->task_info.num_lrank)
  //?
  //if( s->sync_counter==nlp_CN*tw_nnodes() )
    {

      s->sync_counter = 0;
      printf("Sync here 1 time!\n");

      for(i=0; i<s->task_info.num_lrank; i++)
      //?
      //for (i=0; i<nlp_CN*tw_nnodes(); i++)
	{
	  //printf("compute node %d lp id is %d\n",i,s->compute_node[i]);
	  // avoid 0 time step event
	  // mod avoid significant delay at high rank 
	  ts = i%nlp_CN;

	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->event_type = BGP_SYNC;

	  m->MC = msg->MC;
	  tw_event_send(e);
	}
    }

}

void cn_sync( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  s->sync_counter++;

  if(s->sync_counter==s->task_info.num_lrank)
  //?
  //if( s->sync_counter==nlp_CN*tw_nnodes() )
    {
      s->sync_counter = 0;
      s->checkpoint_counter++;

      if( s->checkpoint_counter <= N_checkpoint )
	{
	  s->time_stamp[s->checkpoint_counter-1] = tw_now(lp);
	  printf("\n Sync %d here ****************  %d  ************* at %lf \n", 
		 s->local_root_gid,
		 s->checkpoint_counter,
		 tw_now(lp) );
	  // after computation time, send out another request
	  for (i=0; i<s->task_info.num_lrank; i++)
	  //?
	  //for (i=0; i<nlp_CN*tw_nnodes(); i++)
	    {
	      e = tw_event_new( s->compute_node[i], computation_time, lp );
	      m = tw_event_data(e);
	      m->event_type = APP_IO_REQUEST;
	      
	      m->MC = msg->MC;
	      tw_event_send(e);
	    }
	}
    }
 
}

void cn_write_sync( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  s->w_counter++;

  if( s->w_counter==s->task_info.num_lrank)
  //?
  //if( s->w_counter==N_ION_active * N_CN_per_ION )
    {
      s->w_counter = 0;

      printf("\n  Sync %d ***************************** at %lf \n",
	     s->local_root_gid,
	     tw_now(lp) );
      // after computation time, send out another request
      for (i=0; i<s->task_info.num_lrank; i++)
	{
	  e = tw_event_new( s->compute_node[i], 1, lp );
	  m = tw_event_data(e);
	  m->event_type = APP_SYNC_DONE;
	      
	  m->MC = msg->MC;
	  tw_event_send(e);
	}
    }
 
}


void cn_checkpoint( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  if ( s->bandwidth > msg->travel_start_time )
    s->bandwidth = msg->travel_start_time; 

  s->sync_counter++;

  //if( s->sync_counter==nlp_CN*tw_nnodes() )
  // count close ack message from each ION
  if( s->sync_counter == s->task_info.num_lrank/N_CN_per_ION )
    //if( s->sync_counter==N_ION_active )
    {
      //#ifdef ALIGNED
      if (msg->io_type == WRITE_ALIGNED)
	printf("\n Observed aligned %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif

      //#ifdef UNALIGNED
      if (msg->io_type == WRITE_UNALIGNED)
      	printf("\n Observed unaligned %d K processes bandwidth is %lf GB/sec at time %lf\n\n",
      	       N_ION_active * N_CN_per_ION / 1024,
      	       s->bandwidth,
	       tw_now(lp));
      //#endif
      //#ifdef UNIQUE
      if (msg->io_type == WRITE_UNIQUE)
	printf("\n Observed unique %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif

      s->sync_counter = 0;
      s->checkpoint_counter++;

      // do N steps of checkpoint
      if( s->checkpoint_counter <= N_checkpoint )
	{
	  s->time_stamp[s->checkpoint_counter-1] = tw_now(lp);
	  printf("\n Sync here ****************  %d  ************* at %lf \n", 
		 s->checkpoint_counter,
		 tw_now(lp) );
	  // after computation time, send out another request
	  for (i=0; i<s->task_info.num_lrank; i++)
	  //?
	  //for (i=0; i<nlp_CN*tw_nnodes(); i++)
	    {
	      e = tw_event_new( s->compute_node[i], computation_time, lp );
	      m = tw_event_data(e);
	      m->event_type = APP_IO_REQUEST;
	      
	      m->MC = msg->MC;
	      tw_event_send(e);
	    }
	}
      s->bandwidth = 1000000000000;
    }
 
}


void cn_io_request( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Message %d start from CN %d travel time is %lf\n",
	 lp->gid,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = s->CN_ID_in_tree;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = APP_OPEN;

  m->travel_start_time = tw_now(lp) + ts;

  // aligned 
#ifdef ALIGNED
  m->io_offset = s->CN_ID * 16 * PVFS_payload_size;
  //m->io_payload_size = 16 * PVFS_payload_size;
  m->io_payload_size = PVFS_payload_size;

  m->collective_group_size = nlp_CN * N_PE;
  m->collective_group_rank = s->CN_ID;

  m->collective_master_node_id = 0;
  m->io_type = WRITE_ALIGNED;
  m->io_tag = 12;
#endif

  // unaligned
#ifdef UNALIGNED
  m->io_offset = s->CN_ID * 16 * payload_size;
  //m->io_payload_size = 16 * payload_size;
  m->io_payload_size = payload_size;

  m->collective_group_size = nlp_CN * N_PE;
  m->collective_group_rank = s->CN_ID;
  
  m->collective_master_node_id = 0;
  m->io_type = WRITE_UNALIGNED;
  m->io_tag = 12;
#endif

  // unique
#ifdef UNIQUE
  m->io_offset = 0;
  m->io_payload_size = 16 * PVFS_payload_size;

  m->collective_group_size = 1;
  m->collective_group_rank = 0;

  m->collective_master_node_id = 0;
  m->io_type = WRITE_UNIQUE;
  //m->io_tag = 12;
  m->io_tag = tw_rand_integer(lp->rng,0,nlp_FS*N_PE);
#endif

  m->message_CN_source = lp->gid;
  
  m->MC.Local_CN_root = s->compute_node[s->task_info.min];

  //printf("local root is %d and gid is %d\n",s->task_info.min,s->compute_node[s->task_info.min]);

  /* int i; */
  /* for (i=0; i<nlp_CN*tw_nnodes(); i++) */
  /*   { */
  /*     printf("compute node %d gid is %d\n",i,s->compute_node[i]); */
  /*   } */

  m->MC = msg->MC;
  tw_event_send(e);

}

void cn_close_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;

  double virtual_delay;
  unsigned long long size;
  double payload = PVFS_payload_size;

#ifdef TRACE
      printf("close %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif

/* #ifdef UNALIGNED */
/*       payload = payload_size; */
/* #endif */

      if ( msg->io_type==WRITE_UNALIGNED)
	payload = payload_size;

      virtual_delay = (tw_now(lp) - msg->travel_start_time) ;
      size = (unsigned long long)N_ION_active*s->cumulated_data_size*(unsigned long long)N_CN_per_ION/1.024/1.024/1.024;
      //size = s->cumulated_data_size * (unsigned long long)msg->MC.num_in_group/1.024/1.024/1.024;

      /* //if (msg->message_CN_source == 0) */
      /* printf("Round %d, close %d ACKed by CN, travel time is %lf, bandwidth is %lf GB/s\n", */
      /* 	     s->checkpoint_counter, */
      /* 	     msg->message_CN_source, */
      /* 	     virtual_delay, */
      /* 	     size/virtual_delay); */
      
      // send sync signal to root 0

      e = tw_event_new( s->local_root_gid, 1, lp );
      //?
      //e = tw_event_new( 0, 1, lp );
      m = tw_event_data(e);
      m->event_type = CHECKPOINT;

      // use travel_start_time (double) to
      // piggyback the bandwidth value
      m->travel_start_time = size/virtual_delay;

      m->io_type = msg->io_type;

      /* if (msg->io_type==WRITE_ALIGNED) */
      /* 	printf("It works ******* \n"); */

      //printf("Close ack here\n");
      m->MC = msg->MC;
      tw_event_send(e);

}


void cn_handshake_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Handshake %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
  ts = s->sender_next_available_time - tw_now(lp);

  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void cn_app_open( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("APP open %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  ts = 1;

  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void cn_app_sync( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("APP sync %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  ts = 1;

  e = tw_event_new( s->local_root_gid, ts, lp );
  //?
  //e = tw_event_new( 0, ts, lp );
  m = tw_event_data(e);
  m->event_type = WRITE_SYNC;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;

  s->SavedMessage = *msg;

  tw_event_send(e);

}


void cn_handshake_end( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Handshake %d END from CN %d travel time is %lf\n ",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  
  ts = CN_CONT_msg_prep_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = APP_OPEN_DONE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);
  
}

void cn_app_open_done( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int t;

#ifdef TRACE
  printf("APP open done %d from CN %d travel time is %lf\n ",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);

  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event));
  /* fprintf(stderr,"OT is %d num var is %d var1 is %d var2 is %d var3 is %ld\n", */
  /* 	 t, */
  /* 	 (s->next_event).num_var, */
  /* 	 (s->next_event).var[0], */
  /* 	 (s->next_event).var[1], */
  /* 	 (s->next_event).var[2]); */
  /* fflush(stderr); */

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;

  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  //#define PRINTtrace
  if(t == CL_WRITEAT)
    {
      m->event_type = APP_WRITE;
      m->io_payload_size = (s->next_event).var[1];
      //m->io_offset = m->io_payload_size * s->CN_ID;
      m->io_offset = (s->next_event).var[2];
      s->cumulated_data_size += m->io_payload_size;

      m->MC = msg->MC;

#ifdef PRINTtrace      
      printf("write at detected, size is %ld\n",m->io_payload_size);
#endif
    }
  if(t == CL_CLOSE)
    {
      m->event_type = APP_CLOSE;
#ifdef PRINTtrace      
      printf("close at detected\n");
#endif
    }
  if(t == CL_OPEN)
    {
      m->event_type = APP_OPEN;
#ifdef PRINTtrace      
      printf("open at detected\n");
#endif
    }
  if(t == CL_SYNC)
    {
      m->event_type = APP_SYNC;
#ifdef PRINTtrace      
      printf("sync at detected\n");
#endif
    }
  if(t == CL_SLEEP)
    {
      m->event_type = APP_SLEEP;
      m->MC.sleep_time = (s->next_event).var[0];
      //printf("sleep %lld\n",m->MC.sleep_time);
#ifdef PRINTtrace      
      printf("sleep at detected\n");
#endif
    }

  tw_event_send(e);
  
}

void cn_app_close( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("app close %d ACKed at CN travel time is %lf, IO tag is %d\n",
	 msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

      ts = 1;

      e = tw_event_new( lp->gid, ts , lp );
      m = tw_event_data(e);
      m->event_type = CLOSE_SEND;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;

      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = lp->gid;
      m->message_FS_source = msg->message_FS_source;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);

}

void cn_app_close_done( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("close %d ACKed at ION travel time is %lf, IO tag is %d\n",
	 msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = lp->gid;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cn_app_write_done( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int t;

#ifdef TRACE
  printf("APP write done %d from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;

  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;


  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event));
  /* printf("WT is %d num var is %d var1 is %d var2 is %ld var3 is %ld\n", */
  /* 	 t, */
  /* 	 (s->next_event).num_var, */
  /* 	 (s->next_event).var[0], */
  /* 	 (s->next_event).var[1], */
  /* 	 (s->next_event).var[2]); */

  //#define PRINTtrace

  m->MC = msg->MC;

  if(t == CL_WRITEAT)
    {
      m->event_type = APP_WRITE;
      m->io_payload_size = (s->next_event).var[1];
      //m->io_offset = m->io_payload_size * s->CN_ID;
      m->io_offset = (s->next_event).var[2];
      s->cumulated_data_size += m->io_payload_size;

#ifdef PRINTtrace      
      printf("write at detected size is %ld\n",m->io_payload_size);
#endif
    }
  if(t == CL_CLOSE)
    {
      m->event_type = APP_CLOSE;
#ifdef PRINTtrace      
      printf("close at detected\n");
#endif
    }
  if(t == CL_OPEN)
    {
      m->event_type = APP_OPEN;
#ifdef PRINTtrace      
      printf("open at detected\n");
#endif
    }
  if(t == CL_SYNC)
    {
      m->event_type = APP_SYNC;
#ifdef PRINTtrace      
      printf("sync at detected\n");
#endif
    }
  if(t == CL_SLEEP)
    {
      m->event_type = APP_SLEEP;
      m->MC.sleep_time = (s->next_event).var[0];
      //printf("sleep %lld\n",m->MC.sleep_time);

#ifdef PRINTtrace      
      printf("sleep at detected\n");
#endif
    }

  tw_event_send(e);

}

void cn_app_sync_done( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int t;

#ifdef TRACE
  printf("APP sync done %d from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);

  *m = s->SavedMessage;

  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event));
  /* printf("WT is %d num var is %d var1 is %d var2 is %ld var3 is %ld\n", */
  /* 	 t, */
  /* 	 (s->next_event).num_var, */
  /* 	 (s->next_event).var[0], */
  /* 	 (s->next_event).var[1], */
  /* 	 (s->next_event).var[2]); */

  //#define PRINTtrace
  m->MC = msg->MC;

  if(t == CL_WRITEAT)
    {
      m->event_type = APP_WRITE;
      m->io_payload_size = (s->next_event).var[1];
      //m->io_offset = m->io_payload_size * s->CN_ID;
      m->io_offset = (s->next_event).var[2];
      s->cumulated_data_size += m->io_payload_size;

#ifdef PRINTtrace      
      printf("write at detected size is %ld\n",m->io_payload_size);
#endif
    }
  if(t == CL_CLOSE)
    {
      m->event_type = APP_CLOSE;
#ifdef PRINTtrace      
      printf("close at detected\n");
#endif
    }
  if(t == CL_OPEN)
    {
      m->event_type = APP_OPEN;
#ifdef PRINTtrace      
      printf("open at detected\n");
#endif
    }
  if(t == CL_SYNC)
    {
      m->event_type = APP_SYNC;
#ifdef PRINTtrace      
      printf("sync at detected\n");
#endif
    }
  if(t == CL_SLEEP)
    {
      m->event_type = APP_SLEEP;
      m->MC.sleep_time = (s->next_event).var[0];
      //printf("sleep %lld\n",m->MC.sleep_time);

#ifdef PRINTtrace      
      printf("sleep at detected\n");
#endif
    }

  tw_event_send(e);

}


void cn_app_sleep_done( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int t;

#ifdef TRACE
  printf("APP sleep done %d from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;

  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  t = codeslang_parse_input(s->ps, &(s->c),&(s->next_event));

  m->MC = msg->MC;

  //#define PRINTtrace
  if(t == CL_WRITEAT)
    {
      m->event_type = APP_WRITE;
      m->io_payload_size = (s->next_event).var[1];
      //m->io_offset = m->io_payload_size * s->CN_ID;
      m->io_offset = (s->next_event).var[2];
      s->cumulated_data_size += m->io_payload_size;

      m->MC = msg->MC;

#ifdef PRINTtrace      
      printf("write at detected size is %ld\n",m->io_payload_size);
#endif
    }
  if(t == CL_CLOSE)
    {
      m->event_type = APP_CLOSE;
#ifdef PRINTtrace      
      printf("close at detected\n");
#endif
    }
  if(t == CL_OPEN)
    {
      m->event_type = APP_OPEN;
#ifdef PRINTtrace      
      printf("open at detected\n");
#endif
    }
  if(t == CL_SYNC)
    {
      m->event_type = APP_SYNC;
#ifdef PRINTtrace      
      printf("sync at detected\n");
#endif
    }
  if(t == CL_SLEEP)
    {
      m->event_type = APP_SLEEP;
      m->MC.sleep_time = (s->next_event).var[0];
      //printf("sleep %lld\n",m->MC.sleep_time);

#ifdef PRINTtrace      
      printf("sleep at detected\n");
#endif
    }

  tw_event_send(e);

}

void cn_app_sleep( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("APP sleep %d from CN %d travel time is %lf, data size is %ld\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_payload_size );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  ts = msg->MC.sleep_time*1000000;
  //printf("sleep %lld and ts is %lf\n",msg->MC.sleep_time,ts);

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = APP_SLEEP_DONE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;

  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);
  
}


void cn_app_write( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("APP write %d from CN %d travel time is %lf, data size is %ld\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_payload_size );
#endif
  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);
  ts = 1;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = DATA_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;

  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);
  
}

void cn_data_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;
  double piece_size;

#ifdef TRACE
  printf("data %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif


  if (msg->io_type==WRITE_ALIGNED)
    {
      s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
      ts = s->sender_next_available_time - tw_now(lp);

      s->sender_next_available_time += msg->io_payload_size/CN_out_bw;

      e = tw_event_new( s->tree_next_hop_id, ts +  msg->io_payload_size/CN_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = DATA_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      // change of piece size
      s->write_counter++;
      m->io_offset = msg->io_offset + msg->io_payload_size * s->write_counter++;
      m->io_payload_size = msg->io_payload_size;

      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;

      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = msg->message_ION_source;
      m->message_FS_source = msg->message_FS_source;

      tw_event_send(e);
      
    }


  //#ifdef ALIGNED
  /* if (msg->io_type==WRITE_ALIGNED) */
  /*   { */
  /*     piece_size = msg->io_payload_size / 16; */

  /*     for ( i=0; i<16; i++ ) */
  /* 	{ */
  /* 	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp)); */
  /* 	  ts = s->sender_next_available_time - tw_now(lp); */

  /* 	  s->sender_next_available_time += piece_size/CN_out_bw; */

  /* 	  e = tw_event_new( s->tree_next_hop_id, ts + piece_size/CN_out_bw, lp ); */
  /* 	  m = tw_event_data(e); */
  /* 	  m->event_type = DATA_ARRIVE; */

  /* 	  m->travel_start_time = msg->travel_start_time; */

  /* 	  // change of piece size */
  /* 	  m->io_offset = msg->io_offset + piece_size * i; */
  /* 	  m->io_payload_size = piece_size; */

  /* 	  m->collective_group_size = msg->collective_group_size; */
  /* 	  m->collective_group_rank = msg->collective_group_rank; */

  /* 	  m->collective_master_node_id = msg->collective_master_node_id; */
  /* 	  m->io_type = msg->io_type; */
  /* 	  m->io_tag = msg->io_tag; */

  /* 	  m->message_CN_source = msg->message_CN_source; */
  /* 	  m->message_ION_source = msg->message_ION_source; */
  /* 	  m->message_FS_source = msg->message_FS_source; */

  /* 	  tw_event_send(e); */
      
  /* 	} */
  /*   } */
  //#endif

  if(msg->io_type==WRITE_UNALIGNED)
    {
      s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
      ts = s->sender_next_available_time - tw_now(lp);

      s->sender_next_available_time += msg->io_payload_size/CN_out_bw;

      /* printf("\n***** IO offset is %ld cn sender available time is %lf and ts is %lf trans time is %lf now is %lf\n", */
      /*        msg->io_offset, */
      /* 	     s->sender_next_available_time, */
      /*        ts, */
      /*        msg->io_payload_size/CN_out_bw, */
      /*        tw_now(lp)); */

      e = tw_event_new( s->tree_next_hop_id, ts + msg->io_payload_size/CN_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = DATA_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      // change of piece size
      // s->write_counter++;
      m->io_offset = msg->io_offset;
      //m->io_offset = msg->io_offset + msg->io_payload_size; // * s->write_counter++;
      m->io_payload_size = msg->io_payload_size;

      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      //m->io_tag = i;
      m->io_tag = msg->io_tag;

      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = msg->message_ION_source;
      m->message_FS_source = msg->message_FS_source;

      m->MC = msg->MC;
      tw_event_send(e);

    }

  /* if(msg->io_type==WRITE_UNALIGNED) */
  /*   { */
  /*     piece_size = msg->io_payload_size / 16; */

  /*     for ( i=0; i<16; i++ ) */
  /* 	{ */
  /* 	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp)); */
  /* 	  ts = s->sender_next_available_time - tw_now(lp); */

  /* 	  s->sender_next_available_time += piece_size/CN_out_bw; */

  /* 	  e = tw_event_new( s->tree_next_hop_id, ts + piece_size/CN_out_bw, lp ); */
  /* 	  m = tw_event_data(e); */
  /* 	  m->event_type = DATA_ARRIVE; */

  /* 	  m->travel_start_time = msg->travel_start_time; */

  /* 	  // change of piece size */
  /* 	  m->io_offset = msg->io_offset + piece_size * i; */
  /* 	  m->io_payload_size = piece_size; */

  /* 	  m->collective_group_size = msg->collective_group_size; */
  /* 	  m->collective_group_rank = msg->collective_group_rank; */

  /* 	  m->collective_master_node_id = msg->collective_master_node_id; */
  /* 	  m->io_type = msg->io_type; */
  /* 	  //m->io_tag = i; */
  /* 	  m->io_tag = msg->io_tag; */

  /* 	  m->message_CN_source = msg->message_CN_source; */
  /* 	  m->message_ION_source = msg->message_ION_source; */
  /* 	  m->message_FS_source = msg->message_FS_source; */

  /* 	  tw_event_send(e); */
      
  /* 	} */
  /*   } */

  //#ifdef UNIQUE
  if(msg->io_type==WRITE_UNIQUE)
    {
      s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
      ts = s->sender_next_available_time - tw_now(lp);

      s->sender_next_available_time += msg->io_payload_size/CN_out_bw;

      e = tw_event_new( s->tree_next_hop_id, ts + msg->io_payload_size/CN_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = DATA_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;

      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = msg->message_ION_source;
      m->message_FS_source = msg->message_FS_source;

      m->MC = msg->MC;
      tw_event_send(e);
    }
  //#endif
}

void cn_data_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("data %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif

  int index = tw_now(lp)/timing_granuality;
  if (index < N_sample)
    {
      if (msg->MC.group_id == 1)
	s->committed_size1[index] += msg->io_payload_size;
      else if (msg->MC.group_id == 2)
	s->committed_size2[index] += msg->io_payload_size;
      else if (msg->MC.group_id == 3)
	s->committed_size3[index] += msg->io_payload_size;
      else
	printf("Not recorded\n");
    }


      if( msg->io_type == WRITE_ALIGNED )
	{
	  ts = ION_CONT_msg_prep_time;

	  e = tw_event_new( lp->gid, ts , lp );
	  m = tw_event_data(e);
	  m->event_type = APP_WRITE_DONE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  m->io_payload_size = msg->io_payload_size;

	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  m->io_tag = msg->io_tag;
	  m->message_ION_source = msg->message_ION_source;
	  m->message_CN_source = lp->gid;
	  m->message_FS_source = msg->message_FS_source;

	  m->IsLastPacket = msg->IsLastPacket;

	  m->MC = msg->MC;
	  tw_event_send(e);
	}


	//#ifdef UNALIGNED
      if(msg->io_type==WRITE_UNALIGNED)
	{
	  ts = ION_CONT_msg_prep_time;

	  e = tw_event_new( lp->gid, ts , lp );
	  m = tw_event_data(e);
	  m->event_type = APP_WRITE_DONE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  m->io_payload_size = msg->io_payload_size;

	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  m->io_tag = msg->io_tag;
	  m->message_ION_source = msg->message_ION_source;
	  m->message_CN_source = lp->gid;
	  m->message_FS_source = msg->message_FS_source;

	  m->IsLastPacket = msg->IsLastPacket;

	  m->MC = msg->MC;
	  tw_event_send(e);

	}
      //#endif

      /* if(msg->io_type==WRITE_UNALIGNED) */
      /* 	{ */
      /* 	  s->write_counter++; */
      /* 	  if ( s->write_counter == 16 ) */
      /* 	    { */
      /* 	      s->write_counter = 0; */

      /* 	      ts = ION_CONT_msg_prep_time; */

      /* 	      e = tw_event_new( lp->gid, ts , lp ); */
      /* 	      m = tw_event_data(e); */
      /* 	      m->event_type = CLOSE_SEND; */

      /* 	      m->travel_start_time = msg->travel_start_time; */

      /* 	      m->io_offset = msg->io_offset; */
      /* 	      m->io_payload_size = msg->io_payload_size; */

      /* 	      m->collective_group_size = msg->collective_group_size; */
      /* 	      m->collective_group_rank = msg->collective_group_rank; */

      /* 	      m->collective_master_node_id = msg->collective_master_node_id; */
      /* 	      m->io_type = msg->io_type; */
      /* 	      m->io_tag = msg->io_tag; */
      /* 	      m->message_ION_source = msg->message_ION_source; */
      /* 	      m->message_CN_source = lp->gid; */
      /* 	      m->message_FS_source = msg->message_FS_source; */

      /* 	      m->IsLastPacket = msg->IsLastPacket; */

      /* 	      tw_event_send(e); */
      /* 	    } */
      /* 	} */


      //#ifdef UNIQUE
      if(msg->io_type==WRITE_UNIQUE)
	{
	  ts = ION_CONT_msg_prep_time;
	  
	  e = tw_event_new( lp->gid, ts , lp );
	  m = tw_event_data(e);
	  m->event_type = CLOSE_SEND;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  m->io_payload_size = msg->io_payload_size;

	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  m->io_tag = msg->io_tag;
	  m->message_ION_source = msg->message_ION_source;
	  m->message_CN_source = lp->gid;
	  m->message_FS_source = msg->message_FS_source;

	  m->IsLastPacket = msg->IsLastPacket;

	  m->MC = msg->MC;
	  tw_event_send(e);
	}
	//#endif
}


void cn_close_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Close %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif

  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
  ts = s->sender_next_available_time - tw_now(lp);

  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = lp->gid;
  m->message_FS_source = msg->message_FS_source;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  m->MC.Local_CN_root = s->local_root_gid;
  //printf("local root is %d\n",s->local_root_gid);
  tw_event_send(e);

}


void bgp_cn_eventHandler( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  
  switch(msg->event_type)
    {
    case CONFIGURE:
      cn_configure( s, bf, msg, lp );
      break;
    case PRE_CONFIGURE:
      cn_pre_configure( s, bf, msg, lp );
      break;
    case BGP_SYNC:
      cn_sync( s, bf, msg, lp );
      break;
    case WRITE_SYNC:
      cn_write_sync( s, bf, msg, lp );
      break;
    case CHECKPOINT:
      cn_checkpoint( s, bf, msg, lp );
      break;
    case APP_IO_REQUEST:
      cn_io_request( s, bf, msg, lp );
      break;
    case APP_OPEN:
      cn_app_open( s, bf, msg, lp );
      break;
    case APP_OPEN_DONE:
      cn_app_open_done( s, bf, msg, lp );
      break;
    case APP_SYNC:
      cn_app_sync( s, bf, msg, lp );
      break;
    case APP_SYNC_DONE:
      cn_app_sync_done( s, bf, msg, lp );
      break;
    case APP_SLEEP:
      cn_app_sleep( s, bf, msg, lp );
      break;
    case APP_SLEEP_DONE:
      cn_app_sleep_done( s, bf, msg, lp );
      break;
    case APP_WRITE:
      cn_app_write( s, bf, msg, lp );
      break;
    case APP_WRITE_DONE:
      cn_app_write_done( s, bf, msg, lp );
      break;
    case APP_CLOSE:
      cn_app_close( s, bf, msg, lp );
      break;
    case APP_CLOSE_DONE:
      cn_app_close_done( s, bf, msg, lp );
      break;
    case HANDSHAKE_SEND:
      cn_handshake_send( s, bf, msg, lp );
      break;
    case HANDSHAKE_END:
      cn_handshake_end( s, bf, msg, lp );
      break;
    case DATA_SEND:
      cn_data_send( s, bf, msg, lp );
      break;
    case DATA_ACK:
      cn_data_ack( s, bf, msg, lp );
      break;
    case CLOSE_SEND:
      cn_close_send( s, bf, msg, lp );
      break;
    case CLOSE_ACK:
      cn_close_ack( s, bf, msg, lp );
      break;
    default:
      printf("Scheduled the wrong events  %d in compute NODEs!\n",msg->event_type);
    }
}

void bgp_cn_eventHandler_rc( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{}

void bgp_cn_finish( CN_state* s, tw_lp* lp )
{
  int i;
  for (i=0; i<N_sample; i++)
    {
      CN_monitor1[i] += s->committed_size1[i];
      CN_monitor2[i] += s->committed_size2[i];
      CN_monitor3[i] += s->committed_size3[i];
    }

}

int get_tree_next_hop( int TreeNextHopIndex )
{
  // tree structure is hidden here
  // only called in init phase
  // give me the index and return the next hop ID
  int TreeMatrix[32];

  TreeMatrix[0]=4;
  TreeMatrix[1]=17;
  TreeMatrix[2]=6;
  TreeMatrix[3]=7;
  TreeMatrix[4]=20;
  TreeMatrix[5]=1;
  TreeMatrix[6]=22;
  TreeMatrix[7]=-1;
  TreeMatrix[8]=24;
  TreeMatrix[9]=13;
  TreeMatrix[10]=26;
  TreeMatrix[11]=10;
  TreeMatrix[12]=28;
  TreeMatrix[13]=29;
  TreeMatrix[14]=10;
  TreeMatrix[15]=11;
  TreeMatrix[16]=20;
  TreeMatrix[17]=21;
  TreeMatrix[18]=2;
  TreeMatrix[19]=3;
  TreeMatrix[20]=21;
  TreeMatrix[21]=6;
  TreeMatrix[22]=7;
  TreeMatrix[23]=19;
  TreeMatrix[24]=25;
  TreeMatrix[25]=26;
  TreeMatrix[26]=22;
  TreeMatrix[27]=11;
  TreeMatrix[28]=24;
  TreeMatrix[29]=25;
  TreeMatrix[30]=14;
  TreeMatrix[31]=15;

  if ( TreeNextHopIndex == 39 )// BGP specific
    { return 35; }
  else if ( TreeNextHopIndex == 35 )// BGP specific
    return 51;
  else if ( TreeNextHopIndex == 51 )// BGP specific
    return 55;
  else if ( TreeNextHopIndex == 55 )// BGP specific
    return 23;
  else if ( TreeNextHopIndex > 31 )// BGP specific
    return TreeMatrix[TreeNextHopIndex-32] + 32;
  else
    return TreeMatrix[TreeNextHopIndex];

}


/* void cn_configure( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp ) */
/* { */
/*   tw_event * e;  */
/*   tw_stime ts; */
/*   MsgData * m; */
/*   int i; */

/*   switch(msg->message_type) */
/*     { */
/*     case CONT: */
/*       { */
/* 	// when receive data message, record msg source */
/* 	if ( s->tree_previous_hop_id[0] == -1) */
/* 	  s->tree_previous_hop_id[0] = msg->msg_src_lp_id; */
/* 	else */
/* 	  s->tree_previous_hop_id[1] = msg->msg_src_lp_id; */
	
/* #ifdef TRACE  */
/* 	printf("CN %d received CONFIG CONT message\n",s->CN_ID_in_tree); */
/* 	for ( i=0; i<2; i++ ) */
/* 	  printf("CN %d previous hop [%d] is %d\n", */
/* 		 s->CN_ID_in_tree, */
/* 		 i, */
/* 		 s->tree_previous_hop_id[i]); */
/* #endif */

/* 	// configure the tree, identify previous hops */
/* 	ts = s->MsgPrepTime + lp->gid % 64; */
	    
/* 	// handshake: send back */
/* 	for ( i=0; i<2; i++ ) */
/* 	  { */
/* 	    if ( s->tree_previous_hop_id[i] >0 ) */
/* 	      { */
/* 		e = tw_event_new( s->tree_previous_hop_id[i], ts, lp ); */
/* 		m = tw_event_data(e); */
		    
/* 		m->type = CONFIG; */
/* 		m->msg_src_lp_id = lp->gid; */
/* 		m->travel_start_time = msg->travel_start_time; */
		    
/* 		m->collective_msg_tag = 12180; */
/* 		m->message_type = ACK; */
		    
/* 		tw_event_send(e); */
/* 	      } */
/* 	  } */
	    
/*       } */
/*       break; */
/*     case ACK: */
/* #ifdef TRACE */
/*       printf("CN %d received CONFIG ACK message, travel time is %lf\n", */
/* 	     s->CN_ID_in_tree, */
/* 	     tw_now(lp) - msg->travel_start_time ); */
	  
/* #endif */
/*       break; */
/*     } */
  
/* } */
