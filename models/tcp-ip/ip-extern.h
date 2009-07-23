#ifndef INC_ip_extern_h
#define INC_ip_extern_h

/*
 * ip.c
 */
extern void	 ip_lp_init(ip_state *, tw_lp *);
extern void	 ip_event_handler(ip_state *, tw_bf *, tcp_message *, tw_lp *);
extern void	 ip_rc_event_handler(ip_state *, tw_bf *, tcp_message *, tw_lp *);
extern void	 ip_lp_final(ip_state *state, tw_lp * lp);

/*
 * ip-upstream.c
 */
extern void	 ip_upstream(ip_state *, tcp_message *, tw_lp * lp);
extern void	 ip_rc_upstream(ip_state * state, tcp_message * msg, tw_lp * lp);

/*
 * ip-downstream.c
 */
extern ip_link	*ip_getlink(ip_state * state, tw_lpid dst);
extern void	 ip_downstream_source(ip_state *, tw_bf *, tcp_message *, tw_lp *);
extern void	 ip_downstream_forward(ip_state *, tw_bf *, tcp_message *, tw_lp *);

/*
 * ip-rc-downstream.c
 */
extern void	 ip_rc_downstream(ip_state *, tcp_message *, tw_lp *);

/*
 * ip-packet.c
 */
extern void ip_packet_drop(ip_state * state, tcp_message * msg, tw_lp * lp);
extern void ip_rc_packet_drop(ip_state * state, tcp_message * msg, tw_lp * lp);

/*
 * ip-routing.c
 */
extern void ip_routing_init();
extern void ip_routing_init_ft(ip_state * state, tw_lp * lp);
extern void ip_rt_print(ip_state * state, tw_lp *lp, FILE * log);

/*
 * ip-routing.c
 */
extern tw_lpid	 ip_getroute(ip_state * state, tw_lpid dst);

/*
 * ip-init.c
 */
extern void	 ip_init(tw_lp * lp, FILE * f);

/*
 * ip-main.c
 */
extern void	 ip_main(int argc, char ** argv, char ** env);
extern void	 ip_md_final();

/*
 * ip-global.c
 */
extern ip_stats	*g_ip_stats;
extern int	 g_ip_header;
extern tw_fd	 g_ip_fd;
extern int	 g_ip_routing_simple;
extern int	 g_ip_write;
extern FILE	*g_ip_routing_file;

extern short int	*g_ip_front;
extern short int	*g_ip_p;
extern short int	*g_ip_f;
extern short int	*g_ip_s;
extern short int	*g_ip_d;

#endif
