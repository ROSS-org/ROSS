#ifndef OSPF_NEIGHBOR_TYPES_H
#define OSPF_NEIGHBOR_TYPES_H

#define OSPF_AGING_INTERVAL		4
#define OSPF_AGING_INCREMENT		4
#define OSPF_AGING_FACTOR 		(OSPF_AGING_INCREMENT / OSPF_AGING_INTERVAL)

#define OSPF_RETRANS_INTERVAL		5

/* nbr interface state machine states */
enum ospf_int_state_t;
typedef ospf_int_state_t ospf_int_state;
enum ospf_int_state_t
{
	ospf_int_down_st = 1,
	ospf_int_waiting_st,
	ospf_int_dr_other_st,
	ospf_int_backup_st,
	ospf_int_dr_st,
	ospf_int_loopback_st,
	ospf_int_point_to_point_st
};

/* nbr interface state machine events */
enum ospf_int_event_t;
typedef ospf_int_event_t ospf_int_event;
enum ospf_int_event_t
{
	ospf_int_up_ev,
	ospf_int_waittimer_ev,
	ospf_int_backupseen_ev,
	ospf_int_nbrchange_ev,
	ospf_int_loopind_ev,
	ospf_int_unloopind_ev,
	ospf_int_down_ev
};

/* nbr state machine states */
enum ospf_nbr_state_t;
typedef ospf_nbr_state_t ospf_nbr_state;
enum ospf_nbr_state_t
{
	ospf_nbr_down_st, 
	ospf_nbr_attempt_st,
	ospf_nbr_init_st,
	ospf_nbr_two_way_st,
	ospf_nbr_exstart_st,
	ospf_nbr_exchange_st,
	ospf_nbr_loading_st,
	ospf_nbr_full_st
};

/* nbr state machine events */
enum ospf_nbr_event_t;
typedef ospf_nbr_event_t ospf_nbr_event;
enum ospf_nbr_event_t
{
	ospf_nbr_hello_recv_ev,
	ospf_nbr_start_ev,
	ospf_nbr_two_way_recv_ev,
	ospf_nbr_neg_done_ev,
	ospf_nbr_exchange_done_ev,
	ospf_nbr_bad_ls_request_ev,
	ospf_nbr_load_done_ev,
	ospf_nbr_adj_ok_ev,
	ospf_nbr_seqnum_mismatch_ev,
	ospf_nbr_one_way_ev,
	ospf_nbr_kill_nbr_ev,
	ospf_nbr_inactivity_timer_ev,
	ospf_nbr_ll_down_ev,

	rc_ospf_nbr_inactivity_timer_ev
};

/*
 * ospf_nbr:
 * 
 * the neighbor data structure described in the OSPF RFC
 */
struct ospf_nbr_t
{
	rn_machine		*m;
	rn_area			*ar;

	/*
	 * Neighbor variables
	 */
	ospf_link_type		 ltype;
	ospf_state		*router;
	ospf_nbr_state 		 state;
	ospf_int_state		 istate; 
	
	tw_stime		 last_sent;
	
	/* These do not need to be so large! */
	tw_stime	hello_interval;
	tw_stime	router_dead_interval;
	tw_stime	bandwidth;

	unsigned char	 master;
	unsigned int dd_seqnum;

	unsigned int	id;

	unsigned int	priority;
	unsigned int	designated_r;
	unsigned int	b_designated_r;

	/*
	 * Neighbor timer variables
	 *
	 * hello_timer		-- timer to wake me up so I can send my nbr HELLOs
	 * inactivity_timer	-- HELLO inactive timer, real tw_timer
	 * flood_accum		-- the length of the LSAs in the flood pkt
	 * next_flood		-- next flood time
	 * flood		-- flood timer, next flood msg
	 * retrans_accum	-- size of next retrans msg
	 * next_retransmit	-- next retransmit time
	 * retrans		-- retransmit timer, next retrans msg
	 * delay_ack_timer	-- ACK timer, real tw_timer
	 */
	tw_event	*hello_timer;
	tw_event	*inactivity_timer;

	unsigned int	 flood_accum;
	tw_stime	 next_flood;
	tw_event	*flood;

	unsigned int	 retrans_accum;
	tw_stime	 next_retransmit;
	tw_event	*retrans_timer;

	tw_event	*delay_ack_timer;

	/*
	 * Neighbor messages
	 *
	 * last_recv_dd		-- the last DD pkt I recv'd, might need to
	 *			   resend this pkt at some point, so keep
	 *			   a pointer to it.
	 * retrans_dd		-- the currently sent DD pkt, which may
	 *			   need to be retransmitted, so keep a ptr
	 *			   to this also.
	 * next_db_entry	-- the next DB entry to send in the next
	 *			   DD pkt sequence.
	 * hello		-- HELLO message to neighbor
	 */
	tw_memory	*last_recv_dd;
	tw_memory	*retrans_dd;

	unsigned int	 next_db_entry;

	ospf_hello	*hello;

	/*
	 * Neighbor lists
	 *
	 * nrequest	-- size of my request list
	 * requests	-- array of all neighbors.  When an entry is set
	 *		   to zero, no request needed, otherwise request it.
	 */
	unsigned int	 nrequests;
	char			*requests;

	tw_memory	*delay;
	unsigned int	 delay_ack_size;
};

#endif
