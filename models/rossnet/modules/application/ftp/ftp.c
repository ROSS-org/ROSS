#include <ftp.h>

	/*
	 * The LP initialization routine
	 */
void
ftp_init(ftp_lp_state * state, tw_lp * lp)
{
	tw_event	*new_event;
	tw_memory	*b;

	ftp_message	*ftp_msg;

	bzero(state, sizeof(ftp_lp_state));

	printf("%d: FTP startup complete\n", lp->id);

	if(lp->id != 0)
		return;

	new_event = rn_event_new(lp->gid, 0.0, lp, DOWNSTREAM, g_ftp_chunk_size);

	b = tw_memory_alloc(lp, g_ftp_fd);
	ftp_msg = tw_memory_data(b);
	ftp_msg->type = FTP_SEND;
			
	tw_event_memory_set(new_event, b, g_tcp_fd);
	rn_event_send(new_event);

	state->bytes_sent += g_ftp_chunk_size;
}

	/*
	 * The forward computation event-handler routine
	 */
void
ftp_event_handler(ftp_lp_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*new_event;
	tw_memory	*b;
	tw_memory	*b_out;

	ftp_message	*ftp_msg;
	ftp_message	*new_msg;
	tw_stime	 ts;

	tw_lpid		 dst;

	ts = 0.0;
	*(int *)bf = (int)0;

	if(NULL == (b = tw_event_memory_get(lp)))
		tw_error(TW_LOC, "No membuf on event!");

	ftp_msg = tw_memory_data(b);

	switch(msg->type)
	{
		case UPSTREAM:
#if VERIFY_FTP_STATE
			printf("%d: FTP UPSTREAM\n", lp->id);
#endif

			if(ftp_msg->type == COMPLETE)
			{
				bf->c1 = 1;
			
				state->bytes_read += msg->size;
				state->files_read++;
			}
			else
			{
				if(state->bytes_sent % g_ftp_buffer_size == 0)
					state->files_sent++;

				break;
			}

			// keep going and send the file back
			// un-comment this line in order to be a sink
			break;
		case DOWNSTREAM:
#if VERIFY_FTP_STATE
			printf("%d: FTP DOWNSTREAM\n", lp->id);
#endif

			ts += tw_rand_exponential(lp->rng, FTP_SEND_TIME);
			new_event = rn_event_new(dst, ts, lp, 
					DOWNSTREAM, g_ftp_chunk_size);

			b_out = tw_memory_alloc(lp, g_ftp_fd);
			new_msg = tw_memory_data(b_out);

			ftp_msg->type = FTP_READ;
			ftp_msg->size = g_ftp_chunk_size;
		
#if VERIFY_FTP	
			printf("%d: ftp sending %d bytes to lp %d %g\n",
			       lp->id, g_ftp_chunk_size, dst, tw_now(lp));
#endif
			tw_event_memory_set(new_event, b_out, g_tcp_fd);
			rn_event_send(new_event);
			
			state->bytes_sent += g_ftp_chunk_size;

			break;

		default:
			tw_error(TW_LOC, "Unknown ftp event type!");
	}

	tw_memory_free(lp, b, g_ftp_fd);
}

	/*
	 * The reverse computation event-handler routine
	 */
void
ftp_rc_event_handler(ftp_lp_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	if(NULL == (b = tw_memory_free_rc(lp, g_ftp_fd)))
		tw_error(TW_LOC, "No membuf on RC event!");

	switch(msg->type)
	{
		case UPSTREAM:

			if(state->bytes_sent % g_ftp_buffer_size == 0)
				state->files_sent++;

			state->bytes_read -= msg->size;

			if(1 == bf->c1)
			{
				state->files_read--;
				break;
			}

			state->files_read--;

		case DOWNSTREAM:
			tw_rand_reverse_unif(lp->rng);
			state->bytes_sent -= g_ftp_chunk_size;

			break;
	}

	tw_event_memory_get_rc(lp, b, g_tcp_fd);
}

	/*
	 * Collect the statistics from each LP
	 */
void
ftp_final(ftp_lp_state * state, tw_lp * lp)
{
	g_ftp_stats.s_bytes_sent += state->bytes_sent;	
	g_ftp_stats.s_bytes_read += state->bytes_read;	

	g_ftp_stats.s_files_sent += state->files_sent;	
	g_ftp_stats.s_files_read += state->files_read;	
}

void
ftp_statistics_print()
{
	printf("FTP Model Statistics:\n");

        printf("\t%-32s %11ld\n", "Total Bytes Sent", 
			g_ftp_stats.s_bytes_sent);
        printf("\t%-32s %11ld\n", "Total Bytes Read", 
			g_ftp_stats.s_bytes_read);

        printf("\t%-32s %11ld\n", "Total Files Sent", 
			g_ftp_stats.s_files_sent);
        printf("\t%-32s %11ld\n", "Total Files Read", 
			g_ftp_stats.s_files_read);
}

#if 0

tw_lptype ftp_lps [] = {
        {FTP_MODEL, sizeof(ftp_lp_state),
         (init_f) ftp_init,
         (event_f) ftp_event_handler,
         (revent_f) ftp_rc_event_handler,
         (final_f) ftp_final,
         (statecp_f) NULL}
};

int
main(int argc, char **argv, char **env)
{
	int	 i;

	tw_lp 	*lp;
	tw_kp	*kp;
	tw_pe	*pe;

	ftp_init(NULL, NULL);

	g_tw_npe = 2;
	g_tw_nkp = 2;
	g_tw_nlp = 2;

	g_tw_gvt_interval = g_ftp_chunk_size;
	g_tw_ts_end = 1000;

	g_tw_events_per_pe = (FTP_START_EVENTS * g_tw_nlp / g_tw_npe) + 
				(g_ftp_buffer_size / g_ftp_chunk_size) + g_ftp_chunk_size;

	if(!tw_init(ftp_lps, g_tw_npe, g_tw_nkp, g_tw_nlp, sizeof(ftp_message)))
		tw_error(TW_LOC, "Could not initialize ROSS!");

	for(i = 0; i < g_tw_nlp; i++)
	{
		lp = tw_getlp(i);
		kp = tw_getkp(i % g_tw_nkp);
		pe = tw_getpe(i % g_tw_npe);

		tw_lp_settype(lp, FTP_MODEL);
		tw_lp_onkp(lp, kp);
		tw_lp_onpe(lp, pe);
		tw_kp_onpe(kp, pe);
	}

	bzero(&g_ftp_stats, sizeof(ftp_statistics));

	tw_run();

	ftp_statistics_print();

	return 0;
}
#endif
