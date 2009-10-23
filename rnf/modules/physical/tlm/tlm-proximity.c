#include <tlm.h>

	/*
	 * HAVE TO COPY THE PARTICLE!!  Already placed into local particle Q
	 */
void
tlm_proximity_send(tlm_state * state, tw_memory * p, tw_memory * b, tw_lp * lp)
{
	tw_event	*e;

	tlm_message	*m;
	tlm_particle	*p0;
	tlm_particle	*p1;

	p0 = tw_memory_data(p);
	p1 = tw_memory_data(b);

/*
	printf("%ld: sending PROXIMITY_LP at %lf to LPs: %ld -- %ld \n",
		lp->id, tw_now(lp), p0->user_lp->id, p1->user_lp->id);
*/

	// send RM_PROXIMITY_LP event to particle p0
	e = tw_event_new(p0->user_lp->gid, 0.0, lp);
	m = tw_event_data(e);
	m->type = RM_PROXIMITY_LP;

	//tw_event_memory_set(e, b, g_tlm_fd);
	tw_event_send(e);

	// now send the other direction
	// send RM_PROXIMITY_LP event to particle p1
	e = tw_event_new(p1->user_lp->gid, 0.0, lp);
	m = tw_event_data(e);

	m->type = RM_PROXIMITY_LP;

	//tw_event_memory_set(e, p, g_tlm_fd);
	tw_event_send(e);
}
