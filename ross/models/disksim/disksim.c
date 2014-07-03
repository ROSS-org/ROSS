#include "disksim.h"

tw_peid disksim_map(tw_lpid gid);

void disksim_ras_init(disksim_ras_state * s, tw_lp * lp);
void disksim_ras_event_handler(disksim_ras_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp);
void disksim_ras_event_handler_rc(disksim_ras_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp);
void disksim_ras_finish(disksim_ras_state * s, tw_lp * lp);

void disksim_init(disksim_state * s, tw_lp * lp);
void disksim_event_handler(disksim_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp);
void disksim_event_handler_rc(disksim_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp);
void disksim_finish(disksim_state * s, tw_lp * lp);

tw_lptype       mylps[] = 
  {
    {(init_f) disksim_ras_init,
     (pre_run_f) NULL,
     (event_f) disksim_ras_event_handler,
     (revent_f) disksim_ras_event_handler_rc,
     (final_f) disksim_ras_finish,
     (map_f) disksim_map,
     sizeof(disksim_ras_state)},
    {(init_f) disksim_init,
     (pre_run_f) NULL,
     (event_f) disksim_event_handler,
     (revent_f) disksim_event_handler_rc,
     (final_f) disksim_finish,
     (map_f) disksim_map,
     sizeof(disksim_state)},
    {0},
};


tw_peid disksim_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

void disksim_ras_init(disksim_ras_state * s, tw_lp * lp)
{
  int i;
  for( i = 0; i < MAX_HOURS; i++)
    s->disk_failure_per_hour[i] = 0;
}

void disksim_ras_event_handler(disksim_ras_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp)
{
  if( (unsigned int)(m->time_of_failure) <= MAX_HOURS )
    s->disk_failure_per_hour[ (unsigned int)(m->time_of_failure)]++;
}

void disksim_ras_event_handler_rc(disksim_ras_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp)
{
  if( (unsigned int)(m->time_of_failure) <= MAX_HOURS )
    s->disk_failure_per_hour[ (unsigned int)(m->time_of_failure)]--;
}

void disksim_ras_finish(disksim_ras_state * s, tw_lp * lp)
{
  int i;
  unsigned long long total_disk_failures=0;
  for( i = 0; i < MAX_HOURS; i++ )
    {
/*       if( s->disk_failure_per_hour[i] ) */
/* 	printf("RAS LOGGER %llu: FAILURE STATS: %u %u \n",  */
/* 	       lp->gid, i, s->disk_failure_per_hour[i]); */
      total_disk_failures += s->disk_failure_per_hour[i];
    }
  printf("RAS LOGGER %lu: TOTAL DISK FAILURES FOR THE 5 YEARs ARE %llu \n", 
	 lp->gid, total_disk_failures );
}

void disksim_init(disksim_state * s, tw_lp * lp)
{
  tw_bf init_bf;
  disksim_event_handler(s, &init_bf, NULL, lp);
}

void disksim_event_handler(disksim_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp)
{
  tw_event *e;
  disksim_message *failure;
  tw_stime fail_time;
  tw_lpid dest_lpid=0;

  switch( disksim_distribution )
    {
    case 0:
      fail_time = (tw_stime)((double)tw_rand_integer( lp->rng, 0, 2*MTTF ));
      break;

    case 1:
      fail_time = (tw_stime)tw_rand_exponential( lp->rng, MTTF);
      break;

    case 2:
      fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 0.10 );
      break;

    case 3:
      fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 0.5 );
      break;

    case 4:
      fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 1.0 );
      break;

    case 5:
      fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 5.0 );
      break;

    case 6:
      fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 10.0 );
      break;

    default:
      printf("Bad disksim_distribution setting (%d)..exiting now!! \n", 
	     disksim_distribution);
      exit(-1);
    }

  // zeroth LP on each PE is the ras logger assuming LINEAR default mapping!! 
  dest_lpid = g_tw_nlp * disksim_map( lp->gid );
  e = tw_event_new( dest_lpid, fail_time, lp);
 
  failure = (disksim_message *)tw_event_data(e);
  failure->time_of_failure = tw_now(lp) + fail_time;
  failure->disk_that_failed = lp->gid;
  if( failure->time_of_failure < (tw_stime)MAX_HOURS )
    {
      bf->c0 = 1;
      s->number_of_failures++;
    }
  tw_event_send( e );
  
  /* send next test to self */
  tw_event_send(tw_event_new(lp->gid, (tw_stime)fail_time, lp));
}

void disksim_event_handler_rc(disksim_state * s, tw_bf * bf, disksim_message * m, tw_lp * lp)
{
  tw_rand_reverse_unif(lp->rng);
  if( bf->c0 )
    s->number_of_failures--;
}

void disksim_finish(disksim_state * s, tw_lp * lp)
{
/*   if( s->number_of_failures ) */
/*     printf("Disk %llu had to be replaced %u times \n", lp->gid, s->number_of_failures); */
}


const tw_optdef app_opt[] =
{
	TWOPT_GROUP("DISKSIM Model"),
	TWOPT_UINT("nlp", nlp_per_pe, "number of Disk LPs per processor"),
	TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
	TWOPT_STIME("lookahead", lookahead, "lookahead for events"),
	TWOPT_UINT("start-events", g_disksim_start_events, "number of initial messages per LP"),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_END()
};

int
main(int argc, char **argv, char **env)
{
	int		 i;

	// set a min lookahead of 1.0
	lookahead = 1.0;
	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

        g_tw_memory_nqueues = 16; // give at least 16 memory queue event

	offset_lpid = g_tw_mynode * nlp_per_pe;
	ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
	g_tw_events_per_pe = (mult * nlp_per_pe * g_disksim_start_events) + 
				optimistic_memory;
	//g_tw_rng_default = TW_FALSE;
	g_tw_lookahead = lookahead;

	tw_define_lps(nlp_per_pe, sizeof(disksim_message), 0);

	tw_lp_settype(0, &mylps[0]);

	for(i = 1; i < g_tw_nlp; i++)
	  tw_lp_settype(i, &mylps[1]);

	tw_run();
	tw_end();

	return 0;
}
