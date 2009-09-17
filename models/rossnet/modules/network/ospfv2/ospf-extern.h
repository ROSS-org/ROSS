#ifndef INC_ospf_extern_h
#define INC_ospf_extern_h

extern int g_ospf_link_weight;

extern int g_route[];
extern int g_route1[];
extern int gr1;
extern int gr2;

extern int g_ospf_rt_write;

/* 
 * ospf-experiments.c
 */
extern void ospf_random_weights(ospf_state * state, tw_lp * lp);
extern void ospf_experiment_weights(ospf_state * state, long int i, tw_lp * lp);

/* 
 * ospf-ip.c
 */
extern void ospf_ip_route(ospf_state * state, tw_bf * bf, rn_message * rn_msg, tw_lp * lp);

/*
 * ospf-router-debug.c
 */
extern void ospf_router_debug_routing(ospf_state * state, int d, tw_lp * lp);
extern void ospf_router_debug_rate(tw_lp * lp, int * next);

/*
 * ospf-xml.c
 */
extern void ospf_xml(ospf_state * state, const xmlNodePtr my_node, tw_lp * lp);

extern ospf_lsa * getlsa(rn_machine *, int index, int id);
extern int shouldRecalculateTable(ospf_lsa *newLsa, ospf_lsa *oldLsa);

/*
 * ospf-timer.c
 */
extern tw_event *ospf_timer_cancel(tw_event * tmr, tw_lp *lp);
extern tw_event *ospf_timer_start(ospf_nbr * nbr, tw_event * tmr, 
					tw_stime ts, int type, tw_lp * lp);

/*
 * ospf-interface.c
 */
extern void ospf_interface_event_handler(ospf_nbr * nbr, 
					ospf_int_event event, tw_lp * lp);

/*
 * ospf-neighbor-list.c
 */
extern void ospf_list_free(char * list, unsigned int * size);
extern int  ospf_list_contains(char * list, int index);
extern void ospf_list_link(char * list, int index, unsigned int * size);
extern void ospf_list_unlink(char * list, int index, unsigned int * size);

/*
 * ospf-timer-aging.c
 */
extern void ospf_aging_timer_set(ospf_state * state, ospf_db_entry *, int, tw_lp *);
extern void ospf_aging_timer(ospf_state *state, tw_bf *bf, tw_lp *lp);

/*
 * ospf-hello.c
 */
extern void ospf_hello_send(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, tw_lp * lp);
extern void ospf_hello_packet(ospf_state *, ospf_nbr *, ospf_hello *, tw_bf *, tw_lp *);

/*
 * ospf-neighbor-state.c
 */
extern void ospf_nbr_event_handler(ospf_state * state, ospf_nbr * nbr, ospf_nbr_event event, tw_lp * lp);

/*
 * ospf-interface.c
 */
//extern tw_lp	*ospf_int_getneighbor(ospf_state * state, int id);
//extern int ospf_int_getinterface(ospf_state * state, int id);
extern void ospf_interface_event_handler(ospf_nbr *, ospf_int_event, tw_lp *);

/*
 * ospf-dd.c
 */
extern tw_memory *ospf_dd_copy(tw_memory *b2, tw_lp * lp);
extern void ospf_dd_fill_hdrs(ospf_nbr * nbr);
extern void ospf_dd_exchange(ospf_state *state, tw_lp *lp, ospf_message *m, int);
extern void ospf_dd_fill_header(ospf_nbr * nbr, int index);
extern int ospf_dd_compare(ospf_dd_pkt *h1, ospf_dd_pkt *h2);
extern void ospf_dd_event_handler(ospf_state *, ospf_nbr *, tw_bf *, tw_lp *lp);
extern void ospf_dd_retransmit(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, tw_lp * lp);

/*
 * ospf-lsa.c
 */
extern void ospf_lsa_flush(ospf_state *, ospf_nbr *, ospf_db_entry *, tw_lp *);
extern int ospf_lsa_find(ospf_state * state, ospf_nbr *, int id, tw_lp * lp);
extern void ospf_lsa_print(FILE * log, ospf_lsa * lsa);
extern char lsa_create(ospf_state * state, int lsa_index, tw_lp * lp);
extern void ospf_lsa_link_allocate(ospf_state * state, ospf_lsa * lsa, tw_lp * lp);
extern tw_memory *ospf_lsa_allocate(tw_lp * lp);
extern void ospf_lsa_premature_age(ospf_state * state, ospf_db_entry *, tw_lp * lp);
extern unsigned int ospf_lsa_age(ospf_state *, ospf_db_entry * dbe, tw_lp * lp);
extern void ospf_lsa_refresh(ospf_state * state, ospf_db_entry * dbe, tw_lp * lp);
extern void ospf_lsa_create(ospf_state * state, ospf_nbr *, int lsa_index, tw_lp * lp);
extern void ospf_lsa_start_flood(ospf_nbr * nbr, ospf_db_entry * dbe, tw_lp * lp);
extern void ospf_lsa_flood(ospf_state * s, ospf_db_entry *d, int f, tw_lp * lp);
extern int ospf_lsa_isnewer(ospf_state *, ospf_db_entry *, ospf_db_entry *, tw_lp *);
extern void ospf_lsa_process(ospf_state *state, ospf_message *m, int interface);
extern ospf_lsa	*ospf_lsa_compare(ospf_nbr * n, ospf_lsa *lsa);

/*
 * ospf-ls.c
 */
/*
extern void ospf_ls_update_process(ospf_state *state, ospf_nbr *, tw_memory *, tw_lp *);
extern void ospf_ls_flood_process(ospf_state *state, ospf_nbr *, tw_memory *, tw_lp *);
extern void ospf_ls_clear_update(ospf_message *m);
*/
extern void ospf_ls_update_recv(ospf_state *, tw_bf *, ospf_nbr *, tw_lp *);

/*
 * ospf-ls-request.c
 */
extern void ospf_ls_request(ospf_state * state, ospf_nbr * nbr, tw_lp * lp);
extern void ospf_ls_request_recv(ospf_state *, tw_bf *, ospf_nbr *, tw_lp *);

/*
 * ospf-ack.c
 */
extern void ospf_ack_delayed(ospf_nbr * nbr, tw_memory * buf, int, tw_lp * lp);
extern void ospf_ack_timed_out(ospf_nbr *, tw_bf *, tw_lp * lp);
extern void ospf_ack_direct(ospf_nbr *, tw_memory *, int size, tw_lp *lp);
extern void ospf_ack_process(ospf_nbr * nbr, tw_bf *bf, tw_lp *lp);

/*
 * ospf-database.c
 */
extern void ospf_db_read(ospf_state * state, tw_lp * lp);
extern void ospf_db_print_raw(ospf_state * state, FILE * log, tw_lp * lp);
extern void ospf_db_print(ospf_state * state, FILE * log, tw_lp * lp);
extern void ospf_db_route(ospf_state *, ospf_db_entry * curr, tw_lp * lp);
extern void ospf_db_remove(ospf_state * state, int, tw_lp * lp);
extern unsigned int ospf_db_init(ospf_state * state, tw_lp * lp);
extern ospf_db_entry *ospf_db_insert(ospf_state * s, int index, ospf_db_entry *, tw_lp * lp);
extern void ospf_db_update(ospf_state *, ospf_nbr * nbr, ospf_db_entry *, tw_lp *);

/*
 * ospf-util.c
 */
extern tw_event	*ospf_event_new(ospf_state *, tw_lp *, tw_stime, tw_lp *);
extern tw_event *ospf_event_send(ospf_state * state, tw_event * event, int type,
				tw_lp *, int size, tw_memory * mem, int area);

/*
 * ospf-stats.c
 */
extern void ospf_statistics_collection(ospf_state *state, tw_lp *lp);

/*
 * ospf-neighbor.c
 */
extern ospf_nbr	*ospf_getnbr(ospf_state * state, int id);
extern void ospf_neighbor_init(ospf_state * state, tw_lp * lp);
extern ospf_nbr *ospf_get_neighbor(ospf_state * state, int id);

/*
 * ospf-routing.c
 */
extern void ospf_routing_init(ospf_state * state, tw_lp * lp);
extern void ospf_rt_build(ospf_state *state, int start_vertex);
extern void ospf_rt_timer(ospf_state * state, int start_vertex);
extern void ospf_rt_print(ospf_state * state, tw_lp *lp, FILE *);

/*
 * ospf-router.c
 */
extern ospf_nbr	*ospf_int_getinterface(ospf_state * state, int id);
extern void ospf_startup(ospf_state *state, tw_lp *lp);
extern void ospf_event_handler(ospf_state *, tw_bf *bf, rn_message *m, tw_lp *lp);

/*
 * ospf-router-rc.c 
 */
extern void ospf_rc_event_handler(ospf_state *, tw_bf *bf, rn_message *m, tw_lp *lp);

/*
 * ospf-global.c
 */
extern int			 db_sz;

extern tw_fd			 g_ospf_fd;
extern tw_fd			 g_ospf_lsa_fd;
extern tw_fd			 g_ospf_dd_fd;
extern tw_fd			 g_ospf_ll_fd;
extern ospf_global_state	*g_ospf_state;
extern char			*g_ospf_init_file;
extern FILE			*g_ospf_routing_file;
extern FILE			*g_ospf_lsa_file;
extern FILE			*g_ospf_lsa_input;

extern int *p;
extern int *front;
extern int *f;
extern int *s;
extern int *d;

extern ospf_statistics *g_ospf_stats;

extern tw_memory	**g_ospf_lsa;

extern int g_routers;
extern int g_max_updates;
extern unsigned int g_memory;

extern unsigned int OSPF_LSA_MAX_AGE;
extern unsigned int OSPF_LSA_REFRESH_AGE;

/*
 * ospf-main.c
 */
extern void ospf_md_opts();
extern void ospf_main(int argc, char ** argv, char ** env);
extern void ospf_md_final();

#endif
