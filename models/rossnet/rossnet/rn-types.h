#ifndef INC_rn_types_h
#define INC_rn_types_h

FWD(struct, rn_hash);
FWD(struct, rn_link);
//FWD(enum, rn_link_status);
FWD(struct, rn_machine);
FWD(struct, rn_subnet);
FWD(struct, rn_area);
FWD(struct, rn_as);

FWD(struct, rn_layer);
FWD(struct, rn_stream);
FWD(struct, rn_lp_state);
FWD(struct, rn_statistics);
FWD(struct, rn_lp);
FWD(struct, rn_message);
FWD(struct, rn_lptype);

typedef void	(*rn_xml_init_f) (void *sv, xmlNodePtr node, tw_lp *me);
typedef void	(*md_init_f) (int argc, char ** argv, char **env);
typedef void	(*md_final_f) ();

DEF(struct, rn_lptype)
{
	init_f		init;
	event_f		event;
	revent_f	revent;
	final_f		final;
	map_f		map;

	size_t	state_sz;

	/*
	 * Add new elements only below this line!
	 */
	char	sname[255];
	rn_xml_init_f	xml_init;
	md_init_f	md_init;
	md_final_f	md_final;
};

enum rn_machine_type_tag
{
	c_host = 1,
	c_router,
	c_ct_router,
	c_og_router,
	c_co_router,
	c_ha_router
};

enum rn_message_type_tag
{
	DOWNSTREAM = -1,
	TIMER,
	UPSTREAM,
	DIRECT,
	MOBILITY,
	LINK_STATUS,
	LINK_STATUS_UP,
	LINK_STATUS_DOWN,
	LINK_STATUS_WEIGHT
};

enum rn_link_status_tag
{
	rn_link_up = 1,
	rn_link_down,
	rn_link_mode_once
};

typedef enum rn_link_status_tag rn_link_status;
typedef enum rn_machine_type_tag rn_machine_type;
typedef enum rn_message_type_tag rn_message_type;

	/*
	 * type -- one of UPSTREAM or DOWNSTREAM, so that a layer may know
	 *         how to handle this message.
	 * size -- the hypothetically size of the packet, for queueing stats
	 * dsize -- the size of the data portion of this message.. semantically
	 * 	   equivalent to passing the size of the buffer in a write call
	 * data -- the message passed to a layer.. this is the actual message
	 */
DEF(struct, rn_message)
{
	rn_message_type		 type;

	tw_lp			*timer;

	unsigned int		 port;
	tw_lpid			 src;
	tw_lpid			 dst;

	unsigned int		 size;
	unsigned char		 ttl;

	rn_link			*link;
};

DEF(struct, rn_statistics)
{
	tw_stat		 s_nevents_processed;
	tw_stat		 s_nevents_rollback;
};

DEF(struct, rn_layer)
{
	tw_lp	lp;
};

DEF(struct, rn_stream)
{
	unsigned char		 nlayers;
	unsigned short int	 port;

	unsigned char		 cur_layer;
	rn_layer		*layers;
};

DEF(struct, rn_lp_state)
{
	unsigned int	 nstreams;

	int		 type;
	int		 direction;

	int		 nrng;

	int		 cur_stream;
	tw_lp		*cur_lp;
	rn_stream	*streams;

	unsigned int	 nmobility;
	rn_stream	*mobility;

	rn_statistics	 l_stats;

	tw_event	*cev;
};

DEF(struct, rn_link)
{
#if DYNAMIC_LINKS
	rn_link_status	status;
	tw_stime	next_status;
	tw_stime	last_status;
#endif

	tw_lpid 	addr;
	double		delay;

	unsigned short 	cost;	
	tw_stime	bandwidth;
	tw_stime	last_sent;

	tw_stime	avg_delay;

	rn_machine	*wire;
};

DEF(struct, rn_machine)
{
	tw_lpid			 id;
	tw_lpid			 conn;

	unsigned int		 uid;
	//unsigned char		 root;
	//unsigned char		 level;

	rn_machine_type		 type;

	rn_link			*link;
	int			 nlinks;

	void			*hash_link;

	int			*ft;
	short			*nhop;

	xmlNodePtr	 	 xml;
	rn_subnet		*subnet;
	//rn_machine		*next;
};

DEF(struct, rn_subnet)
{
	int			 id;

	unsigned int		 nmachines;
	rn_machine		*machines;

	tw_lpid			 low;
	tw_lpid			 high;

	rn_area			*area;
	rn_subnet		*next;
};

DEF(struct, rn_area)
{
	unsigned int		 id;

	//unsigned char		 root_lvl;
	tw_lpid			 root;

	tw_lpid			 low;
	tw_lpid			 high;

// need to fix configure.in!
//#ifdef HAVE_OSPF_H
	tw_memory		**g_ospf_lsa;
	unsigned int		 g_ospf_nlsa;
//#endif

	unsigned int		 nsubnets;
	rn_subnet		*subnets;

	unsigned int		 nmachines;
	rn_as			*as;
	rn_area			*next;
};

DEF(struct, rn_as)
{
	unsigned int		 id;
	//unsigned int		 frequency;

	tw_lpid			 low;
	tw_lpid			 high;

#if HAVE_BGP_H || 1
	tw_stime        	 keepalive_interval;
	tw_stime        	 hold_interval;
	tw_stime        	 mrai_interval;
#endif

	//char			*name;

	unsigned int		 nareas;
	rn_area			*areas;

	//rn_as			*next;
};

#define START_EVENTS	2

#define COMPLETE	0
#define INCOMPLETE	1

#endif
