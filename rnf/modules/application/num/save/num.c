#include <num.h>

// Turn off all but first NUM LP
#define ONE		0

// Print VERIFY stmts for only this LP
#define DEBUG_LP	504065

/*
 * num_init: initialize the NUM LP
 */
void
num_init(num_state * state, tw_lp * lp)
{
	state->stats = tw_vector_create(sizeof(num_statistics), 1);
	state->file_size = MIN_FILE_SIZE;
}

/*
 * num_update: invokes NUM update statistics
 *
 * Each network user must add it's stats to the global level table.
 * Therefore, if an agent attempted to utilize the network during the day,
 * we must create a timer event for midnight to record the stats.  This 
 * function is called when that timer fires.
 */
void
num_update(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	num_profile	*np;
	float		 tput;

	if(!g_num_log_f)
		return;

	np = &g_num_profiles[state->type];
	tput = (state->stats->s_total_time > 0) ?
			state->stats->s_total_kb / state->stats->s_total_time : 0.0;

	// is network on?
	if(g_num_mod > 0)
	{
		if(state->stats->s_nfailures > 0)
		{
			g_num_level[state->type][state->stats->s_nfailures]++;
		} else if(state->stats->s_total_kb > 0)
		{
			if (tput > np->bitrate)
				g_num_level[state->type][NUM_LEVEL_0]++;
			else if (tput > np->bitrate/ 10)
				g_num_level[state->type][NUM_LEVEL_1]++;
		}
	}

	// reset state variables
	state->stats->s_total_kb = 0; 
	state->stats->s_total_time = 0; 
	state->stats->s_nfailures = 0;
}

/*
 * num_start: invokes the NUM
 *
 * Right now, called when agent is added to home location by EPI during the
 * hours of 0900-QUIT_TIME.  Network users begin utilizing network during 
 * this period.  Of course we don't want the users to be perfectly synchronized
 * at 0900, so we add jitter to the start time using a timer event.  Jitter is
 * currently determined via an exponential random distribution around ONE_MINUTE.
 *
 * We always create a timer event for midnight so we can update the global stats.
 */
void
num_start(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;
	tw_stime	 ts;

	rn_message	*m;

	int		 today;
	int		 timeofday;

	today = (int) tw_now(lp) / TWENTY_FOUR_HOURS;;
 	timeofday = (int) tw_now(lp) % TWENTY_FOUR_HOURS;

	// set file size to zero for beginning
	state->file_size = MIN_FILE_SIZE;

	// num_starts recv'd after midnight, so report previous days results
	num_report_levels(tw_now(lp) / TWENTY_FOUR_HOURS);

	// only send to QUIT_TIME each day (e.g., 10:00 am)
	if(timeofday >= QUIT_TIME)
		return;

	// update network population here
	g_num_net_pop[state->type]++;

	// if NETWORK is off, then done
	if(g_num_mod == 0)
	{
		if(timeofday >= NINE_OCLOCK)
		{
			state->stats->s_nstart++;

			// update report log
			g_num_level[state->type][NUM_LEVEL_0]++;
		}

		return;
	}

#if ONE
	if(lp->id != DEBUG_LP)
		return;
#endif

	// if not mod day, then do not need to start anything
	if(g_num_mod > 1 && (today % g_num_mod)+1 != g_num_mod)
		return;

	// first file of the day
	state->file_size = MIN_FILE_SIZE; 

	// If before 9:00 AM set a timer to start at 9:00
	if(timeofday < NINE_OCLOCK)
	{
		ts = tw_now(lp) + NINE_OCLOCK - timeofday 
				+ tw_rand_unif(lp->id) * ONE_MINUTE;

		if((int) ts % TWENTY_FOUR_HOURS >= QUIT_TIME)
			tw_error(TW_LOC, "Next file request past quit time!");

		rn_timer_init(lp, ts);
	} else if (timeofday == NINE_OCLOCK)
	{
		// jitter time a liitle bit and start with small file
		ts = tw_now(lp) + tw_rand_unif(lp->id) * ONE_MINUTE;

		if((int) ts % TWENTY_FOUR_HOURS >= QUIT_TIME)
			tw_error(TW_LOC, "Next file request past quit time!");

		rn_timer_init(lp, ts);
	} else
	{
		num_file_start(state, bf, MIN_FILE_SIZE, lp);
	}

	// create the stats update timer, and override the TTL value 
	// in the rn_message data to indicate the event type
	e = rn_timer_init(lp, (today + 1) * 86400);
	m = tw_event_data(e);
	m->ttl = 2;
}

/*
 * num_event_handler: main event handler for NUM
 *
 * Event types:
 *
 * 	UPSTREAM - event from transport layer
 *	TIMER	 - set by NUM to start next file send
 *	TTL = 2	 - event from EPI indicating midnight (update NUM stats)
 *	TTL = 1	 - event from EPI indicating start of NUM for day
 *	ELSE	 - event from EPI indicating end of NUM for day
 */
void
num_event_handler(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	if(msg->type == UPSTREAM)
		num_file_complete(state, bf, msg, lp);
	else if(msg->ttl == 2)
		num_update(state, bf, msg, lp);
	else if(msg->type == TIMER)
		num_file_start(state, bf, state->file_size, lp);
	else if(msg->ttl == 1)
		num_start(state, bf, msg, lp);
	else
		num_file_stop(state, bf, lp);
}

/*
 * num_rc_event_handler: REVERSE COMPUTATION event handler for NUM
 */
void
num_rc_event_handler(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_error(TW_LOC, "NUM Model cannot be run with more than 1 CPU!");
}

/*
 * num_final: finalize NUM
 */
void
num_final(num_state * state, tw_lp * lp)
{
	// print out final days worth of statistics
	if(g_num_mod != -172)
	{
		num_print();
		g_num_mod = -172;
	}

	g_num_stats->s_nstop += state->stats->s_nstop;
	g_num_stats->s_nstart += state->stats->s_nstart;
	g_num_stats->s_nfiles += state->stats->s_nfiles;
}
