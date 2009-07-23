#ifndef INC_bgp_extern_h
#define INC_bgp_extern_h

/*
 * bgp-open.c
 */
extern void bgp_open_send(bgp_state *, bgp_nbr * n, tw_lp * lp);
extern void bgp_open(bgp_state *, tw_bf *, bgp_message *, int, tw_lp *);

/* 
 * bgp-decision.c
 */
extern int bgp_decision(bgp_state *, bgp_route * r1, bgp_route * r2, tw_lp *);

/*
 * bgp-route.c
 */
extern int	  bgp_route_inpath(bgp_state * state, bgp_route * r_in, tw_lp * lp);
extern void	  bgp_route_aspath_print(bgp_state * state, bgp_route * r);
extern int	  bgp_route_free(bgp_state * state, tw_memory * route, tw_lp * lp);
extern tw_memory *bgp_getroute(bgp_state * state, bgp_route * find);
extern void	  bgp_route_add(bgp_state * state, tw_memory * b);
extern void	  bgp_route_withdraw(bgp_state * state, bgp_route * r);
extern tw_memory *bgp_route_new(bgp_state *, tw_lp *, tw_lp *, tw_lp *);
extern int        bgp_route_equals(bgp_route *, bgp_route *);
extern tw_memory *bgp_route_copy(bgp_state *, bgp_route *, bgp_nbr *, bgp_update_type, tw_lp *);
extern void 	  bgp_rib_print(bgp_state * state, tw_lp * lp);
extern void	  bgp_route_print(bgp_state * state, bgp_route * r);

/*
 * bgp-main.c
 */
extern void	bgp_rt_file(int * read);
extern void     bgp_main(int argc, char **argv, char **env);
extern void	bgp_md_final();

/*
 * bgp-utils.c
 */
extern tw_event *bgp_timer_start(tw_lp *, tw_event *, tw_stime, bgp_message_type);

/*
 * bgp-global.c
 */
extern tw_fd    g_bgp_fd;
extern tw_fd    g_bgp_fd_rtes;
extern tw_fd    g_bgp_fd_asp;
extern FILE	*g_bgp_rt_fd;

extern bgp_stats	 g_bgp_stats;
extern bgp_state	*g_bgp_state;
extern bgp_as		*g_bgp_as;
extern char		*g_bgp_init_file;
extern xmlXPathObjectPtr	*g_bgp_ases;

/*
 * bgp-router.c
 */
extern void     bgp_init(bgp_state *, tw_lp *);
extern void     bgp_event_handler(bgp_state *, tw_bf *, rn_message *, tw_lp *);
extern void     bgp_r_event_handler(bgp_state *, tw_bf *, rn_message *, tw_lp *);
extern void     bgp_final(bgp_state *, tw_lp *);

/* 
 * bgp-xml.c
 */
extern void     bgp_xml(bgp_state * state, const xmlNodePtr node, tw_lp *);

/*
 * bgp-notify.c
 */
extern void bgp_notify_send(bgp_state *, bgp_nbr * n, tw_lp * lp);
extern void bgp_notify(bgp_state * state, tw_bf * bf, int src, tw_lp * lp);

/*
 * bgp-nbr.c
 */
extern bgp_nbr	 *bgp_getnbr(bgp_state * state, int id);

/*
 * bgp-connect.c
 */
extern void bgp_connect(bgp_state * state, bgp_nbr * n, tw_lp * lp);

/*
 * bgp-ospf.c
 */
extern void	bgp_ospf_init(bgp_state * bs, bgp_route * r, tw_lp * lp);
extern void	bgp_ospf_flush(bgp_state *, rn_as *, bgp_route *, int, tw_lp *);
extern void	bgp_ospf_update(bgp_state *, rn_as *, bgp_route *, int, tw_lp *);


/*
 *  bgp-mrai.c
 */
extern void	bgp_mrai_tmr(bgp_state * state, tw_lp * lp);
extern void	bgp_mrai_timer(bgp_state *, tw_bf *, bgp_message *, tw_lp *);

/*
 * bgp-update.c
 */
extern void 	bgp_update_all(bgp_state *, bgp_message *, int, bgp_route *, tw_lp *);
extern void	bgp_update_send(bgp_state *, bgp_nbr *, bgp_route *, bgp_message *, 
				int, tw_lp *);
extern void     bgp_update(bgp_state *, tw_bf *, bgp_message *, int, tw_lp *);

/* 
 * bgp-keepalive.c
 */
extern void	bgp_keepalive_tmr(bgp_state * state, tw_lp * lp);
extern void	bgp_keepalive_send(bgp_nbr * n, tw_lp * lp);
extern void	bgp_keepalive(bgp_state * state, int, int, tw_lp *);
extern void     bgp_keepalive_timer(bgp_state *, tw_bf *, bgp_message *, tw_lp *);
extern void	deal_with_timed_out_routers(bgp_state * state, tw_bf * bf, 
				bgp_message * msg, tw_lp * lp);

#endif
