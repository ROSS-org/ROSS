#include <rossnet.h>

rn_stream	*
rn_getstream_byport(rn_lp_state * state, tw_lpid port)
{
	unsigned int	i;

	for(i = 0; i < state->nstreams; i++)
	{
		if(state->streams[i].port == port)
		{
			state->cur_stream = i;
			return &state->streams[i];
		}
	}

	//tw_error(TW_LOC, "Unable to find stream port %d!", port);

	return state->streams;
}
	
rn_stream	*
rn_getstream(tw_lp * lp)
{
	rn_lp_state	*state;
	rn_stream	*s;

	state = (rn_lp_state *) lp->cur_state;
	s = &state->streams[state->cur_stream];

#if 0
	if(!s)
		tw_error(TW_LOC, "Could not get stream from lp %d!", lp->gid);
#endif

	return s;
}
