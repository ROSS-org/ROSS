#ifndef INC_tcp_layer_extern_h
#define INC_tcp_layer_extern_h

/*
 * tcp-layer-xml.c
 */
extern void tcp_layer_xml(tw_lp * lp, xmlNodePtr node);

/*
 * tcp-layer.c
 */
extern void     tcp_layer_init(tcp_layer_state * SV, xmlNodePtr node, tw_lp * lp);
extern void     tcp_layer_event_handler(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);
extern void     tcp_layer_final(tcp_layer_state * SV, tw_lp * lp);
extern void     tcp_layer_rc_event_handler(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);


/*
 * tcp-layer-process.c
 */
extern void     tcp_layer_process_ack(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);
extern void     tcp_layer_process_data(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);
extern void     tcp_layer_update_cwnd(tcp_layer_state * SV, tw_bf * CV,
					tcp_layer_message * M, tw_lp * lp);
extern void     tcp_layer_update_rtt(tcp_layer_state * SV, tw_bf * CV,
					tcp_layer_message * M, tw_lp * lp);
extern void     tcp_layer_timeout(tcp_layer_state * SV, tw_bf * CV, tcp_layer_message * M,
					tw_lp * lp);
/*
 * tcp-layer-process-rc.c
 */
extern void     tcp_layer_process_ack_rc(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);
extern void     tcp_layer_process_data_rc(tcp_layer_state * SV, tw_bf * CV,
					rn_message * M, tw_lp * lp);
extern void     tcp_layer_timeout_rc(tcp_layer_state * SV, tw_bf * CV,
					tcp_layer_message * M, tw_lp * lp);

/*
 * tcp-layer-util.c
 */
extern void     tcp_layer_util_event(tw_lp * lp, int type, int source, int dest,
					int seq_num, int ack, int size);

/*
 * tcp-layer-global.c
 */
extern tcp_layer_statistics	g_tcp_layer_stats;

extern int      g_tcp_layer_iss;
extern int      g_tcp_layer_mss;
extern int      g_tcp_layer_recv_wnd;

#ifdef LOGGING
extern FILE   **host_received_log;
extern FILE   **host_sent_log;
extern FILE   **router_log;
extern FILE   **serv_tcpdump;
extern FILE   **serv_cwnd;
#endif

#endif
