#ifndef INC_ip_types_h
#define INC_ip_types_h

FWD(struct, ip_state);
FWD(struct, ip_message);
FWD(struct, ip_stats);
FWD(struct, ip_link);

DEF(struct, ip_state)
{
	ip_stats	*stats;

	int		 major;
	int		 minor;

	float		 max_delay;

	tw_stat		 capacity;
	tw_memoryq	*link_q;
};

DEF(struct, ip_stats)
{
	/*
	 * IP layer LP statistics
	 *
	 * s_ncomplete -- # of pkts which terminated at this LP
	 * s_nforward  -- # of pkts forwarded on from this node
	 * s_ndropped  -- # of pkts which could not be forwarded
	 */
	tw_stat s_ncomplete;
	tw_stat s_nforward;
	tw_stat s_ndropped;
	tw_stat s_ndropped_source;
	tw_stat s_nnet_failures;
	tw_stat s_ndropped_ttl;
	tw_stat s_avg_ttl;
	tw_stat s_max_ttl;
};

DEF(struct, ip_message)
{
	rn_link		*link;
	tw_stime         last_sent;
};

#endif
