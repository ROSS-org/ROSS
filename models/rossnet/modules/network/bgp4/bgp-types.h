#ifndef INC_bgp_types_h
#define INC_bgp_types_h

#define INITIAL_OPEN_STORM_DELAY 10
#define INITIAL_UPDATE_STORM_DELAY 15
#define OPEN_ACK_DELAY 15
#define UPDATE_DELAY 20

/*
 * Minimum Message Sizes
 */
#define BGP_CONNECT	21
#define BGP_OPEN	29
#define BGP_KEEPALIVE	19
#define BGP_UPDATE	23
#define BGP_NOTIFY	21

FWD(struct, bgp_direct_ospf);
FWD(struct, bgp_route);
FWD(struct, bgp_message);
FWD(struct, bgp_nbr);
FWD(struct, bgp_state);
FWD(struct, bgp_stats);
FWD(enum, bgp_message_type);
FWD(enum, bgp_update_type);
FWD(struct, bgp_as);

DEF(struct, bgp_as)
{
	unsigned int med;
	unsigned int local_pref;
	unsigned int path_padding;

	int *degree;
};

DEF(enum, bgp_update_type)
{
	REMOVE = 0,
	ADD = 1
};

DEF(struct, bgp_route)
{
	unsigned int		 	 src;
	unsigned short int		 dst;
	unsigned short int		 next_hop;

	tw_memoryq			 as_path;

	// rc_nbr 	-- used for nbr down changes
	//bgp_nbr	*rc_nbr;

	unsigned short int		 origin;
	unsigned short int		 med;
	unsigned short int		 bit;
};

DEF(enum, bgp_message_type)
{
	CONNECT = 1,
	OPEN,
	KEEPALIVETIMER,
	KEEPALIVE,
	UPDATE,
	NOTIFICATION = 6,
	MRAITIMER = 7
};

DEF(struct, bgp_nbr)
{
	unsigned short	 id;

	unsigned char	 up;
	unsigned int	 last_update;
	unsigned char	 local_pref;
	unsigned short	 hop_count;
};

DEF(struct, bgp_state)
{
	rn_machine     *m;
	rn_as          *as;

	tw_event       *tmr_keep_alive;
	tw_event       *tmr_mrai;

	tw_stime        keepalive_interval;
	tw_stime        hold_interval;
	tw_stime        mrai_interval;

	/*
	 * Local LP Statistics
	 */
	bgp_stats	*stats;

	bgp_nbr		*nbr;
	unsigned int	 n_interfaces;

	// Pointers to routes installed in our RIB from the
	// AS global route list
	tw_memory	**rib;

	FILE		*log;

	/*
	 * These flags turn on/off parts of the decision algorithm.
	 * By default, these are all turned on.
	 */
	struct
	{
		unsigned char local_pref:1;
		unsigned char as_path:1;
		unsigned char origin:1;
		unsigned char med:1;

		unsigned char hot_potato:1;
		unsigned char next_hop:1;
		unsigned char existing:1;
		unsigned char _pad:1;
	} b;
};

DEF(struct, bgp_message)
{
	// If iBGP message, then we will not have
	// correct BGP source from rn_message
	//unsigned short int	src;

#if 0
	struct
	{
		bgp_message_type	type;

		// If add == true, we're adding, else we're deleting
		unsigned char	update;

		/*
		 * Used to determine cause of BGP UPDATES
		 */
		unsigned char cause_bgp;
		unsigned char cause_ospf;
		unsigned char unreachable;
	} b;
#endif

#if 1
	struct
	{
		unsigned char	type;

		// If add == true, we're adding, else we're deleting
		unsigned char	update:1;

		/*
		 * Used to determine cause of BGP UPDATES
		 */
		unsigned char cause_bgp:1;
		unsigned char cause_ospf:1;
		unsigned char unreachable:1;
	} b;
#endif

	// RC for keepalive messages:
	//tw_stime        old_last_update;
	//int             was_up;

	// RC for notification messages:
	//int             error;

	// RC for update messages:
	//int             rc_asp;
	//int             rev_num_neighbors;
};

DEF(struct, bgp_stats)
{
	unsigned int s_nupdates_sent;
	unsigned int s_nupdates_recv;

	unsigned int s_nroute_adds;
	unsigned int s_nroute_removes;

	unsigned int s_nnotify_sent;
	unsigned int s_nnotify_recv;

	unsigned int s_nopens;
	unsigned int d_opens;

	unsigned int s_nkeepalives;
	unsigned int s_nkeepalivetimers;
	unsigned int s_nunreachable;

	unsigned int s_nconnects;
	unsigned int s_nconnects_dropped;

	unsigned int s_cause_bgp;
	unsigned int s_cause_ospf;

	unsigned int s_nibgp_nbrs_up;
	unsigned int s_nebgp_nbrs_up;

	unsigned int s_nibgp_nbrs;
	unsigned int s_nebgp_nbrs;

	unsigned int s_nmrai_timers;

	unsigned int s_ndec_local_pref;
	unsigned int s_ndec_aspath;
	unsigned int s_ndec_origin;
	unsigned int s_ndec_med;
	unsigned int s_ndec_hot_potato;
	unsigned int s_ndec_next_hop;
	unsigned int s_ndec_existing;
	unsigned int s_ndec_default;

};

#endif
