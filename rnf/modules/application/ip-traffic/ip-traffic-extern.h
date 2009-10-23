#ifndef INC_ip_traffic_extern_h
#define INC_ip_traffic_extern_h

/*
 * ftp.c
 */
extern void     ftp_statistics_print();

/*
 * ftp.c
 */
extern void     ftp_init(ftp_lp_state * state, tw_lp * lp);
extern void	ftp_event_handler(ftp_lp_state * state, tw_bf * bf,
					rn_message * msg, tw_lp * lp);
extern void     ftp_rc_event_handler(ftp_lp_state * state, tw_bf * bf,
					rn_message * msg, tw_lp * lp);
extern void     ftp_final(ftp_lp_state * state, tw_lp * lp);


/*
 * ftp-global.c
 */
extern char    *g_ftp_buffer;
extern unsigned int g_ftp_buffer_size;
extern unsigned int g_ftp_chunk_size;

extern ftp_statistics g_ftp_stats;

#if MODULE
extern rn_lptype ftp_lp;
#endif

#endif
