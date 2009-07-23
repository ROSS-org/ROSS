#ifndef INC_tcp_extern_h
#define INC_tcp_extern_h

/*
 * tcp-timer.c
 */
extern tw_event *tcp_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, 
		tw_stime ts, int sn, tw_lp * lp);
extern tw_event *tcp_rc_timer_reset(tw_event *, tw_bf * bf, tcp_message * old, tw_lp * lp);
extern void tcp_timer_cancel(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp);
extern tw_event	*tcp_rc_timer_cancel(tcp_message * old, tw_lp * lp);

/*
 * tcp-init.c
 */
extern void tcp_init(tcp_state *, FILE * f, tw_lp *);

/*
 * tcp-main.c
 */
extern void     tcp_main(int argc, char ** argv, char ** env);
extern void     tcp_md_final();

/*
 * tcp.c
 */
extern void     tcp_lp_init(tcp_state *, tw_lp *);
extern void     tcp_event_handler(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_rc_event_handler(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_lp_final(tcp_state *, tw_lp *);

/*
 * tcp-process.c
 */
extern void     tcp_process(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_process_ack(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_process_data(tcp_state *, tw_bf *, tcp_message *, tw_lp *);

/*
 * tcp-update.c
 */
extern void     tcp_update_cwnd(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_update_rtt(tcp_state *, tw_bf *, tcp_message *, tw_lp *);

/*
 * tcp-timeout.c
 */
extern void     tcp_timeout(tcp_state *, tw_bf *, tcp_message *, tw_lp *);

/*
 * tcp-rc-process.c
 */
extern void     tcp_rc_process_ack(tcp_state *, tw_bf *, tcp_message *, tw_lp *);
extern void     tcp_rc_process_data(tcp_state *, tw_bf*, tcp_message *, tw_lp *);

/*
 * tcp-rc-timeout.c
 */
extern void     tcp_rc_timeout(tcp_state *, tw_bf *, tcp_message *, tw_lp *);

/*
 * tcp-util.c
 */
extern void tcp_event_send(tcp_state *, int, tw_stime, int, int, int, int, tw_lp *);

/*
 * tcp-global.c
 */
extern tw_fd		 g_tcp_fd;
extern tcp_statistics	*g_tcp_stats;

#endif
