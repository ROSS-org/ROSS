#include <num.h>

#define NUM_VERIFY	1
#define ONE		0
#define DEBUG_LP	504065

void
num_file_start(num_state * state, tw_bf * bf, unsigned int size, tw_lp * lp)
{
#if NUM_VERIFY
	if(lp->id == DEBUG_LP)
		printf("%ld T%d %s: FILE_START size: %.2lfB\n", 
			lp->id, state->type, get_tod(tw_now(lp)), (double) size / 8.0);
#endif

	// send event down to TCP
	state->file_size = size;
	state->stats->s_nstart++;
	state->start = tw_now(lp);
	rn_event_send(rn_event_new(lp, 0.0, lp, DOWNSTREAM, size/8));
}

void
num_file_stop(num_state * state, tw_bf * bf, tw_lp * lp)
{
#if NUM_VERIFY
if(lp->id == DEBUG_LP)
	printf("%ld T%d %s: FILE_STOP\n", lp->id, state->type, get_tod(tw_now(lp)));
#endif

	state->stats->s_nstop++;

	if(g_num_mod < 1)
		return;

	// send event down to TCP
	rn_event_send(rn_event_new(lp, 0.0, lp, DOWNSTREAM, 0));
}

/*
 * num_file_complete: handler routine for file completions
 *
 * Files may complete in two ways:
 *
 *	1. TCP sends an UPSTREAM event with file x-fer size
 *	2. TCP sends an UPSTREAM event with file size 0
 *		(indicates TCP failure)
 *
 * Upon completion, if throughput is good, ask for a larger file.
 * If throughput is less than desired, but acceptable, ask for
 * same size file.  If throughput is not acceptable, ask for
 * minimum size file.
 */
void
num_file_complete(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_stime	 elapsed;
	tw_stime	 ts, tn;

	num_profile	*np;

	int		 actual;
	int		 timeofday;

#if NUM_VERIFY
	if(lp->id == DEBUG_LP)
		printf("%ld T%d %s: FILE_COMPLETE\n", lp->id, state->type,
			 get_tod(tw_now(lp)));
#endif

	tn = tw_now(lp);
	timeofday = (int) tn % TWENTY_FOUR_HOURS;
 	elapsed = tn - state->start;

	if(msg->size == 40)
	{
		tw_error(TW_LOC, "Received TCP ACK!");
	} else if (msg->size > 40)
	{
		state->stats->s_total_time += elapsed;
		state->stats->s_total_kb += state->file_size;
		state->stats->s_nfiles++;
	} else
	{
		// try again?
		state->stats->s_nfailures = 3;;

		return;
	}

	actual = state->file_size / elapsed;
	np = &g_num_profiles[state->type];

#if NUM_VERIFY
	if(lp->id == DEBUG_LP)
	{
		printf("%ld T%d %s: actual: %d, desired: %u, size: %.2lfB, elapsed: %lf\n",
			lp->id, state->type, get_tod(tw_now(lp)), actual, 
			np->bitrate, (double) state->file_size / 8.0, elapsed);
	}
#endif

#if ONE
	if(lp->id != DEBUG_LP)
		return;
#endif

	// If after quitting time, then do not ask for another file
	if(timeofday >= QUIT_TIME)
		return;

	// Both rate and desired are in bits / second
	if(actual >= np->bitrate)
	{
		// Stay with file_size if achieved the desired bitrate
		ts = tn + ((state->file_size / np->bitrate) - elapsed);
		if ( ts <= tn )
			ts = tn + 1.0;
		if((int) ts % TWENTY_FOUR_HOURS >= QUIT_TIME)
			return;

		rn_timer_init(lp, ts);
	} else if (actual >= np->bitrate / 10.0)
	{
		// Ask for another file right away
		state->file_size *= 2;

		if(state->file_size > MAX_FILE_SIZE)
			state->file_size = MAX_FILE_SIZE;

		num_file_start(state, bf, state->file_size, lp);
	} else
	{
		// rate is less than acceptable, walk away from network
		// this would be stage 2
		state->stats->s_nfailures = 2;
	}
}
