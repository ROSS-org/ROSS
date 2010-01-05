#include <ospf.h>

/*
 * Reset the interface timers and clear/reset interface variables.
 */
void
ospf_interface_reset(ospf_nbr * nbr, tw_lp * lp)
{
	nbr->inactivity_timer = ospf_timer_cancel(nbr->inactivity_timer, lp);
}

/*
 * Calculate the designated router
 */
void
ospf_interface_calc_dr(ospf_nbr * nbr, tw_lp *lp)
{
	ospf_state 	*r;
	ospf_nbr	*n;

	int		 i;
	int		 dr;
	int		 bdr;

	int		 dr_priority;
	int		 bdr_priority;

	recalculate:

	r = nbr->router;
	bdr = dr = -1;
	bdr_priority = dr_priority = -1;

	for(i = 0; i < r->n_interfaces; i++)
	{
		n = &r->nbr[i];

		//routers with priority 0 ineligible to become DR
		if(n->priority == 0)
			continue;

		// only consider rtrs which have established connections
		if(n->state < ospf_nbr_two_way_st)
			continue;

		// only consider rtrs listing thenselves as designated
		// rtrs to become the new designated rtr
		if(n->designated_r)
		{
			if(-1 == dr)
			{
				dr = n->id;
				dr_priority = n->priority;
			}
			else
			{
				if(n->priority > dr_priority)
				{
					dr = n->id;
					dr_priority = n->priority;
				}
				else if(n->priority == dr_priority &&
					n->id > dr)
				{
					dr = n->id;
					dr_priority = n->priority;
				}
			}
		}

		// only consider rtrs listing themselves as backups
		// to become the new backup designated rtr
		if(n->b_designated_r)
		{
			if(-1 == bdr)
			{
				bdr = n->id;
				bdr_priority = n->priority;
			}
			else
			{
				if(n->priority > bdr_priority)
				{
					bdr = n->id;
					bdr_priority = n->priority;
				}
				else if(n->priority == bdr_priority && n->id > bdr)
				{
					bdr = n->id;
					bdr_priority = n->priority;
				}
			}
		}
	}

	/*
	 * Also must consider ourselves in the computation
	 */
	if(r->designated_r)
	{
		if(r->priority > dr_priority)
		{
			dr = lp->gid;
			dr_priority = r->priority;
		}
		else if(r->priority == dr_priority && lp->gid > dr)
		{
			dr = lp->gid;
			dr_priority = r->priority;
		}
	}

	if(r->b_designated_r)
	{
		if(r->priority > bdr_priority)
		{
			bdr = lp->gid;
			bdr_priority = r->priority;
		}
		else if(r->priority == bdr_priority && lp->gid > bdr)
		{
			bdr = lp->gid;
			bdr_priority = r->priority;
		}
	}

	if(-1 == dr)
	{
		dr = bdr;
		dr_priority = bdr_priority;
	}

	if(-1 == dr)
	{
		printf("%lld: No dr or rdr elected! \n", lp->gid);
		r->bdr = -1;
		r->dr = -1;

		return;
	}

	printf("dr : %d \n", dr);
	printf("bdr: %d \n", bdr);

	// if I am currently elected new dr or bdr, recalc
	if((dr == lp->gid && r->dr != lp->gid) ||
	   (bdr == lp->gid && r->bdr != lp->gid))
		goto recalculate;

	// if I am de-elected as the dr or bdr, recalc
	if((r->dr == lp->gid && dr != lp->gid) ||
	   (r->bdr == lp->gid && bdr != lp->gid))
		goto recalculate;

	if(dr == lp->gid)
	{
		r->istate = ospf_int_dr_other_st;
	}

	if(bdr == lp->gid)
	{
		r->istate = ospf_int_backup_st;
	}

	if(r->dr != dr || r->bdr != bdr)
	{
		for(i = 0; i < r->n_interfaces; i++)
		{
			n = &r->nbr[i];

			if(n->state >= ospf_nbr_two_way_st)
				ospf_nbr_event_handler(lp->cur_state, n, ospf_nbr_adj_ok_ev, lp);
		}
	}
	
	r->dr = dr;
	r->bdr = bdr;

	tw_error(TW_LOC, "New DR: %d, new BDR: %d", r->dr, r->bdr);

	if(r->dr == lp->gid && r->bdr == lp->gid)
		tw_error(TW_LOC, "%d: Set myself to be dr and bdr!", lp->gid);
}

/*
State(s):  Down

            Event:  InterfaceUp

        New state:  Depends upon action routine

           Action:  Start the interval Hello Timer, enabling the
                    periodic sending of Hello packets out the interface.
                    If the attached network is a physical point-to-point
                    network, Point-to-MultiPoint network or virtual
                    link, the interface state transitions to Point-to-
                    Point.  Else, if the router is not eligible to
                    become Designated Router the interface state
                    transitions to DR Other.

                    Otherwise, the attached network is a broadcast or
                    NBMA network and the router is eligible to become
                    Designated Router.  In this case, in an attempt to
                    discover the attached network's Designated Router
                    the interface state is set to Waiting and the single
                    shot Wait Timer is started.  Additionally, if the
                    network is an NBMA network examine the configured
                    list of neighbors for this interface and generate
                    the neighbor event Start for each neighbor that is
                    also eligible to become Designated Router.
*/

void
ospf_interface_up(ospf_nbr * nbr, tw_lp * lp)
{
	nbr->inactivity_timer =
		ospf_timer_start(nbr, nbr->inactivity_timer, 
				 nbr->router_dead_interval,
				OSPF_HELLO_TIMEOUT, lp);

	// assuming broadcast only right now, but future work is to simply 
	// check the link type for this interface from the datastructure.
	nbr->istate = ospf_int_point_to_point_st;

	printf("%lld: Interface up: %d \n", lp->gid, nbr->id);
}

/*
State(s):  Waiting

            Event:  WaitTimer

        New state:  Depends upon action routine.

           Action:  Calculate the attached network's Backup Designated
                    Router and Designated Router, as shown in Section
                    9.4.  As a result of this calculation, the new state
                    of the interface will be either DR Other, Backup or
                    DR.
*/

void
ospf_interface_waittimer(ospf_nbr * nbr, tw_lp * lp)
{
	ospf_interface_calc_dr(nbr, lp);
}

/*
State(s):  Waiting

            Event:  BackupSeen

        New state:  Depends upon action routine.

           Action:  Calculate the attached network's Backup Designated
                    Router and Designated Router, as shown in Section
                    9.4.  As a result of this calculation, the new state
                    of the interface will be either DR Other, Backup or
                    DR.
*/

void
ospf_interface_backupseen(ospf_nbr * nbr, tw_lp * lp)
{
	ospf_interface_calc_dr(nbr, lp);
}

/*
State(s):  DR Other, Backup or DR

            Event:  NeighborChange

        New state:  Depends upon action routine.

           Action:  Recalculate the attached network's Backup Designated
                    Router and Designated Router, as shown in Section
                    9.4.  As a result of this calculation, the new state
                    of the interface will be either DR Other, Backup or
                    DR.
*/

void
ospf_interface_nbrchange(ospf_nbr * nbr, tw_lp * lp)
{
	ospf_interface_calc_dr(nbr, lp);
}

/*

         State(s):  Any State

            Event:  LoopInd

        New state:  Loopback

           Action:  Since this interface is no longer connected to the
                    attached network the actions associated with the
                    above InterfaceDown event are executed.
*/

/*

         State(s):  Loopback

            Event:  UnloopInd

        New state:  Down

           Action:  No actions are necessary.  For example, the
                    interface variables have already been reset upon
                    entering the Loopback state.  Note that reception of
                    an InterfaceUp event is necessary before the
                    interface again becomes fully functional.
*/

void
ospf_interface_unloopind(ospf_nbr * nbr, tw_lp * lp)
{
	nbr->istate = ospf_int_down_st;
}

/*

         State(s):  Any State

            Event:  InterfaceDown

        New state:  Down

           Action:  All interface variables are reset, and interface
                    timers disabled.  Also, all neighbor connections
                    associated with the interface are destroyed.  This
                    is done by generating the event KillNbr on all
                    associated neighbors (see Section 10.2).
*/

void
ospf_interface_down(ospf_nbr * nbr, tw_lp * lp)
{
	ospf_interface_reset(nbr, lp);
	ospf_nbr_event_handler(lp->cur_state, nbr, ospf_nbr_kill_nbr_ev, lp); 
	nbr->istate = ospf_int_down_st;

	printf("%lld: Interface down: %d \n", lp->gid, nbr->id);
}

void
ospf_interface_event_handler(ospf_nbr * nbr, ospf_int_event event, tw_lp * lp)
{
	switch(event)
	{
		case ospf_int_up_ev:
			if(nbr->istate == ospf_int_down_st)
				ospf_interface_up(nbr, lp);
			break;
		case ospf_int_waittimer_ev:
			if(nbr->istate == ospf_int_waiting_st)
				ospf_interface_waittimer(nbr, lp);
			break;
		case ospf_int_backupseen_ev:
			if(nbr->istate == ospf_int_waiting_st)
				ospf_interface_backupseen(nbr, lp);
			break;
		case ospf_int_nbrchange_ev:
			if(nbr->istate == ospf_int_dr_other_st ||
			   nbr->istate == ospf_int_backup_st ||
			   nbr->istate == ospf_int_dr_st)
				ospf_interface_nbrchange(nbr, lp);
			break;
		case ospf_int_loopind_ev:
			ospf_interface_down(nbr, lp);
			nbr->istate = ospf_int_loopback_st;
			break;
		case ospf_int_unloopind_ev:
			if(nbr->istate == ospf_int_loopback_st)
				ospf_interface_unloopind(nbr, lp);
			break;
		case ospf_int_down_ev:
			ospf_interface_down(nbr, lp);
			break;

		default:
			tw_error(TW_LOC, "Unknown interface event: %d", event);
	}
}
