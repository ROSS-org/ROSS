#ifndef INC_ip_types_h
#define INC_ip_types_h

#define	IP_LP_TYPE 5

FWD(struct, ip_state);
FWD(struct, ip_message);
FWD(struct, ip_stats);
FWD(struct, ip_link);

DEF(struct, ip_state)
{
	ip_link		*links;
	unsigned int	 nlinks;

	ip_stats	*stats;
};

DEF(struct, ip_link)
{
	tw_lpid		 addr;
	float		 delay;
	tw_stime	 bandwidth;

	tw_stime	 last_sent;
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
	unsigned int s_ncomplete;
	unsigned int s_nforward;
	unsigned int s_ndropped;
	unsigned int s_ndropped_source;
	unsigned int s_nnet_failures;
	unsigned int s_ndropped_ttl;
	unsigned long int s_avg_ttl;
	unsigned int s_max_ttl;
};

DEF(struct, ip_message)
{
	tw_lpid		 src;
	tw_lpid		 dst;

	unsigned short int size;

	ip_link		*rc_link;
	tw_stime         rc_lastsent;
};

#endif
