#ifndef INC_ip_types_h
#define INC_ip_types_h

struct ip_state_t;
typedef ip_state_t ip_state;
struct ip_message_t;
typedef ip_message_t ip_message;
struct ip_stats_t;
typedef ip_stats_t ip_stats;
struct ip_link_t;
typedef ip_link_t ip_link;

struct ip_state_t
{
	ip_stats	*stats;

	int		 major;
	int		 minor;

	float		 max_delay;

	tw_stat		 capacity;
	tw_memoryq	*link_q;
};

struct ip_stats_t
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

struct ip_message_t
{
	rn_link		*link;
	tw_stime         last_sent;
};

#endif
