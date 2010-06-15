#ifndef INC_ospf_types_h
#define INC_ospf_types_h

#define OSPF_LP_TYPE 3
DEF(struct, ospf_statistics)
{
	unsigned long int    s_drop_dd;
	unsigned long int    s_proc_dd;

	unsigned long int    s_drop_hello_int;
	unsigned long int    s_drop_poll_int;

	unsigned long int    s_e_hello_in;
	unsigned long int    s_e_hello_out;
	unsigned long int    s_e_hello_timeouts;
	unsigned long int    s_e_dd_msgs;
	unsigned long int    s_e_ls_requests;
	unsigned long int    s_e_ls_updates;
	unsigned long int    s_e_ls_acks;
	unsigned long int    s_e_ack_timeouts;
	unsigned long int    s_e_aging_timeouts;
	unsigned long int    s_e_unknown;

	unsigned long int    s_sent_hellos;
	unsigned long int    s_sent_dds;
	unsigned long int    s_sent_ls_requests;
	unsigned long int    s_sent_ls_updates;
	unsigned long int    s_sent_ls_acks;
	unsigned long int    s_sent_unknown;
	unsigned long int	s_sent_lost;

	unsigned long int	dropped_packets;

	unsigned long int	s_cause_bgp;
	unsigned long int	s_cause_ospf;
};

DEF(struct, ospf_global_state)
{
	unsigned int	mtu;

	tw_stime	hello_timer;
	tw_stime	hello_sendnext;

	tw_stime	rt_interval;
	tw_stime	poll_interval;
	tw_stime	ack_interval;
	tw_stime	flood_timer;
};

DEF(struct, ospf_state)
{
	ospf_global_state	*gstate;

	rn_machine	*m;
	rn_area		*ar;

	tw_stime	 sn;
	unsigned int	 from;
	unsigned int	 from1;

	tw_stime	 c_time;

	unsigned short	 n_interfaces;
	int		*interface_ids;

	ospf_nbr	*nbr;

	unsigned char    priority;

	unsigned char    designated_r;
	unsigned char    b_designated_r;
	ospf_int_state   istate;

	/*
	 * The ids of the designated and backup designated routers
	 */
	unsigned int     dr;
	unsigned int     bdr;

	ospf_db_entry	*db;
	tw_stime	 db_last_ts;
	tw_event	*rt_timer;

	tw_event	*aging_timer;

	unsigned char	 lsa_wrapping;

	signed int	 lsa_seqnum;

	ospf_lsa	*lsa;

	ospf_statistics *stats;

#if OSPF_LOG
	FILE		*log;
#endif
};

#endif
