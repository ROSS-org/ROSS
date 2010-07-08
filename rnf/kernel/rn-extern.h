#ifndef INC_rn_extern_h
#define INC_rn_extern_h

extern int	g_rn_link_prob;
extern int	g_rn_converge_ospf;
extern int	g_rn_converge_bgp;

extern int	g_rn_src;
extern int	g_rn_dst;

extern tw_peid rn_map(tw_lpid gid);

/*
 * hash-quadratic.c
 */
extern void	*rn_hash_create(unsigned int);
extern void	 rn_hash_insert(void *h, rn_link *);
extern rn_link	*rn_hash_remove(void *h, rn_link *);
extern rn_link	*rn_hash_fetch(void *h, int id);

/*
 * rn-routing.c
 */
extern int rn_route(rn_machine * m, tw_lpid dst);
extern int rn_route_change(rn_machine * m, int loc, int new_val);

/**
 * This file contains all of the externable objects in rossnet.
 */

/*
 * rn-forward.c
 */
extern void rn_ft_route_add(tw_lp * lp, int id);
extern void rn_ft_route_del(tw_lp * lp, int id);

/*
 * rn-timer.c
 */
extern tw_event	*rn_timer_simple(tw_lp * lp, tw_stime ts);
extern tw_event	*rn_timer_init(tw_lp * lp, tw_stime ts);
extern tw_event	*rn_timer_reset(tw_lp * lp, tw_event ** e, tw_stime ts);
extern void	 rn_timer_cancel(tw_lp * lp, tw_event ** e);

/** \brief Functions relating to an Autonomous System
 * \author David Bauer
 * This file provides functionality for the AS data type within rossnet.
 * rn-as.c.
 */
extern rn_as	*rn_nextas(rn_as * as);

/*
 * xml-libxml.c
 */
extern char *xml_getprop(xmlNode * node, char * prop);
extern void rn_xml_init();
extern void rn_xml_end();
extern void rn_xml_topology();
extern void rn_xml_link_topology();
extern void rn_xml_model();

extern xmlXPathObjectPtr xpath(char * expr, xmlXPathContextPtr ctxt);

/*
 * rn-area.c
 */
extern rn_area	*rn_nextarea(rn_area * as);
extern rn_area	*rn_nextarea_onas(rn_area * ar, rn_as * as);

/*
 * rn-subnet.c
 */
extern rn_subnet	*rn_nextsn_onarea(rn_subnet * sn, rn_area * ar);
extern rn_machine	*rn_nextmachine_onsn(rn_machine * m, rn_subnet * sn);

/*
 * rn-machine.c
 */
extern rn_as		*rn_getas(rn_machine * m);
extern rn_area		*rn_getarea(rn_machine * m);
extern rn_subnet	*rn_getsubnet(rn_machine * m);

extern rn_machine      * rn_getmachine(tw_lpid id);
extern void rn_setmachine(rn_machine * m);
extern void rn_machine_print(rn_machine * m);

/*
 * rn-link.c
 */
extern rn_link_status	 rn_link_getstatus(rn_link * l, tw_stime ts);
extern void		 rn_link_random_weight_changes(tw_lp * lp);
extern void		 rn_link_random_changes(tw_lp *);
extern void		 rn_link_setwire(rn_link * l);
extern rn_machine	*rn_link_getwire(rn_link * l);
extern rn_link		*rn_getlink(rn_machine * m, tw_lpid id);

/*
 * rn-message.c
 */
extern	void	 rn_message_setsize(rn_message * e, unsigned long sz);
extern	unsigned long rn_message_getsize(rn_message * e);
extern	void	 rn_message_setdirection(rn_message * m, rn_message_type direction);
extern	int	 rn_message_getdirection(rn_message * m);

/*
 * rn-event.c
 */
extern tw_event	*rn_event_direct(tw_lpid dst, tw_stime ts, tw_lp * src);
extern tw_event	*rn_event_new(tw_lpid dst, tw_stime ts, tw_lp * src, 
			rn_message_type, unsigned long int);
extern void	 rn_event_send(tw_event * e);

/*
 * rn-reverse.c
 */
extern void rn_reverse_event_send(tw_lpid, tw_lp * lp, rn_message_type, unsigned long int);

/*
 * rn-setup.c
 */
extern void	 rn_setup_streams(rn_lp_state * state, rn_lptype * types, tw_lp *);
extern void	 rn_setup_mobility(rn_lp_state * state, rn_lptype * types, tw_lp *);
extern void	 rn_setup_links(rn_lp_state * state, tw_lp * lp);
extern void	 rn_setup_options(int argc, char **argv);
extern void	 rn_setup();

/*
 * rn-init.c
 */
extern void	 rn_init_environment(void);
extern void	 rn_init_streams(rn_lp_state * state, tw_lp * lp);

/*
 * rn-models.c
 */
extern void	 rn_models_init(rn_lptype *, int, char **, char **);
extern void	 rn_models_final(rn_lptype * types);

/*
 * rn-lp.c
 */
extern void	*rn_getstate(tw_lp * lp);
extern tw_lp	*rn_getlp(rn_lp_state * state, int id);
extern tw_lp 	*rn_next_lp(rn_lp_state * state, tw_lp * last, int type);
extern rn_lptype	*rn_lp_gettype(tw_lp * lp);
extern void		 rn_lp_settype(tw_lp * lp, char * name);

/*
 * rn-stream.c
 */
extern rn_stream	*rn_getstream(tw_lp * lp);
extern rn_stream	*rn_getstream_byport(rn_lp_state * state, tw_lpid port);

/*
 * rn-global.c
 */
extern tw_lpid		g_rn_env_nlps;

extern xmlDocPtr document_network;
extern xmlDocPtr document_links;
extern xmlDocPtr document_model;

extern xmlXPathContextPtr ctxt;
extern xmlXPathContextPtr ctxt_links;
extern xmlXPathContextPtr ctxt_model;

extern xmlNodePtr g_rn_environment;

extern char		 g_rn_run[1024];
extern unsigned char	 g_rn_ttl;
extern unsigned int	 g_rn_msg_sz;

extern char	 g_rn_tools_dir[];
extern char	 g_rn_rt_table[];
extern char	 g_rn_xml_topology[];
extern char	 g_rn_xml_link_topology[];
extern char	 g_rn_xml_model[];
extern char	 g_rn_logs_dir[];

/* For NetDMF only */
extern char      g_rn_netdmf_config[];

extern rn_statistics	*g_rn_stats;
extern rn_lptype	*g_rn_lptypes;

extern unsigned int	 g_rn_nas;
extern rn_as		*g_rn_as;

extern unsigned int	 g_rn_nareas;
extern rn_area		*g_rn_areas;

extern unsigned int	 g_rn_nsubnets;
extern rn_subnet	*g_rn_subnets;

extern unsigned int 	 g_rn_nlinks;
extern unsigned int 	 g_rn_nlink_status;
extern unsigned int	 g_rn_nmachines;
extern unsigned int	 g_rn_nrouters;

extern rn_machine	*g_rn_machines;

#endif
