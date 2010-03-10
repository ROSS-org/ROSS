#ifndef INC_wifi_mac_extern_h
#define INC_wifi_mac_extern_h

/*
 * wifi-mac-xml.c
 */
extern void wifi_mac_xml(wifi_mac_state *, const xmlNodePtr node, tw_lp *);

/*
 * wifi-mac-model.c
 */
extern void wifi_mac_md_opts();
extern void wifi_mac_md_init(int argc, char ** argv, char ** env);
extern void wifi_mac_md_final();

/*
 * wifi-mac-pe.c - model per PE functions
 */
extern void wifi_mac_pe_init(tw_pe * me);
extern void wifi_mac_pe_post_init(tw_pe * me);
extern void wifi_mac_pe_gvt(tw_pe * me);
extern void wifi_mac_pe_final(tw_pe * me);

/*
 * wifi-mac-timer.c - RM Wrapper for ROSS timer library
 */
extern tw_event	*wifi_mac_timer_cancel(tw_event * t, tw_lp * lp);
extern tw_event	*wifi_mac_timer_start(tw_event * t, int dim, tw_stime ts, tw_lp * lp);

/*
 * wifi-mac.c - Cell LP file with simulation engine function definitions
 */
extern void _wifi_mac_init(wifi_mac_state *, tw_lp * lp);
extern void _wifi_mac_event_handler(wifi_mac_state *, tw_bf * , wifi_mac_message *, tw_lp *lp);
extern void _wifi_mac_rc_event_handler(wifi_mac_state *, tw_bf *, wifi_mac_message *, tw_lp * lp);
extern void _wifi_mac_final(wifi_mac_state *, tw_lp * lp);
extern int wifi_mac_main(int argc, char ** argv, char ** env);

/*
 * wifi-mac-global.c: global variables
 */
extern unsigned int	 g_wifi_mac_optmem;
extern wifi_mac_statistics	*g_wifi_mac_stats;

extern tw_lpid		 nlp_per_pe;
extern tw_lpid		 nwifi_mac_lp_per_pe;

extern double   	g_wifi_mac_edThresholdW;
extern double   	g_wifi_mac_ccaMode1ThresholdW;
extern double   	g_wifi_mac_txGainDb;
extern double   	g_wifi_mac_rxGainDb;
extern double   	g_wifi_mac_txPowerBaseDbm;
extern double   	g_wifi_mac_txPowerEndDbm;
extern uint32_t 	g_wifi_mac_nTxPower;

extern tw_fd		 g_wifi_mac_fd;

#endif
