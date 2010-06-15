#ifndef INC_ip_types_h
#define INC_ip_types_h

#define	IP_LP_TYPE 5

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
	ip_link		*links;
	unsigned int	 nlinks;

	ip_stats	*stats;
};

struct ip_link_t
{
	tw_lpid		 addr;
	float		 delay;
	tw_stime	 bandwidth;

	tw_stime	 last_sent;
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
	unsigned int s_ncomplete;
	unsigned int s_nforward;
	unsigned int s_ndropped;
	unsigned int s_ndropped_source;
	unsigned int s_nnet_failures;
	unsigned int s_ndropped_ttl;
	unsigned long int s_avg_ttl;
	unsigned int s_max_ttl;
};

struct ip_message_t
{
	tw_lpid		 src;
	tw_lpid		 dst;

	unsigned short int size;

	ip_link		*rc_link;
	tw_stime         rc_lastsent;
};

#endif
