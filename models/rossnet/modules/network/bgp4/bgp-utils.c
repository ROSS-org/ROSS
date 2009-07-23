#include <bgp.h>

tw_event	*
bgp_timer_start(tw_lp * lp, tw_event * tmr, tw_stime ts, bgp_message_type type)
{
	tw_memory	*b = tw_memory_alloc(lp, g_bgp_fd);
	bgp_message	*m = tw_memory_data(b);

	tmr = rn_timer_simple(lp, ts);

	m->b.type = type;
	
        tw_event_memory_set(tmr, b, g_bgp_fd);

	return tmr;
}

void
bgp_r_timer_start()
{
}
