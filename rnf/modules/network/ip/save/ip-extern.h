#ifndef INC_ip_extern_h
#define INC_ip_extern_h

/*
 * ip.c
 */
extern void	 ip_init(ip_state *, tw_lp *);
extern void	 ip_event_handler(ip_state *, tw_bf *, rn_message *, tw_lp *);
extern void	 ip_rc_event_handler(ip_state *, tw_bf *, rn_message *, tw_lp *);
extern void	 ip_final(ip_state *state, tw_lp * lp);

/*
 * ip-upstream.c
 */
extern void	 ip_upstream(ip_state *, rn_message *, tw_lp * lp);
extern void	 ip_rc_upstream(ip_state * state, rn_message * msg, tw_lp * lp);

/*
 * ip-downstream.c
 */
extern void	 ip_downstream_source(ip_state *, tw_bf *, rn_message *, tw_lp *);
extern void	 ip_downstream_forward(ip_state *, tw_bf *, rn_message *, tw_lp *);

/*
 * ip-rc-downstream.c
 */
extern void	 ip_rc_downstream(ip_state *, rn_message *, tw_lp *);

/*
 * ip-packet.c
 */
extern void ip_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp);
extern void ip_rc_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp);

/*
 * ip-routing.c
 */
extern void ip_routing_init();
//extern void ip_routing_init_ft(ip_state * state, tw_lp * lp);
extern void ip_rt_print(ip_state * state, tw_lp *lp, FILE * log);
extern rn_link	*ip_route(rn_machine * me, rn_message * msg);

/*
 * ip-routing.c
 */
extern tw_lpid	 ip_getroute(ip_state * state, tw_lpid dst);

/*
 * ip-xml.c
 */
extern void	 ip_xml(tw_lp * lp, xmlNodePtr node);

/*
 * ip-model.c
 */
extern void	 ip_md_init(int argc, char ** argv, char ** env);
extern void	 ip_md_final();

/*
 * ip-global.c
 */
extern tw_stime g_ip_major_interval;
extern tw_stime g_ip_minor_interval;

extern ip_stats	*g_ip_stats;
extern int	 g_ip_header;
extern tw_fd	 g_ip_fd;
extern int	 g_ip_routing_simple;
extern FILE	*g_ip_log;
extern FILE	*g_ip_routing_file;
extern int     **g_ip_rt;
extern unsigned int	 g_ip_log_on;

extern short int	*g_ip_front;
extern short int	*g_ip_p;
extern short int	*g_ip_f;
extern short int	*g_ip_s;
extern short int	*g_ip_d;

#endif
