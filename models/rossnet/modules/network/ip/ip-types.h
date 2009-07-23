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

	unsigned long int capacity;
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
	unsigned long int s_ncomplete;
	unsigned long int s_nforward;
	unsigned long int s_ndropped;
	unsigned long int s_ndropped_source;
	unsigned long int s_nnet_failures;
	unsigned long int s_ndropped_ttl;
	unsigned long int s_avg_ttl;
	unsigned long int s_max_ttl;
};

DEF(struct, ip_message)
{
	rn_link		*link;
	tw_stime         last_sent;
};

#endif
