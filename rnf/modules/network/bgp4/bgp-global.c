#include <bgp.h>

//@{
/**
 * @name Memory queue file descriptors
 */
tw_fd           g_bgp_fd;      /**< membufs for header messages */
tw_fd           g_bgp_fd_rtes; /**< membufs for routes */
tw_fd           g_bgp_fd_asp;  /**< membufs for AS paths within routes */
//@}

//@{
/**
 * @name Global BGP model statistics
 */
/*
 * The following comments are ambiguous...
 * g_bgp_meds	-- MED values for each AS
 * g_bgp_prefs	-- Local Pref values for each AS
 */
bgp_stats        g_bgp_stats;            /**< global LP statistics */
bgp_as		*g_bgp_as = NULL;
bgp_state	*g_bgp_state = NULL;     /**< global LP state */
char		*g_bgp_init_file = NULL; /**< the bgp.conf config file */
//@}

/**
 * To make XML for iBGP connections unnecessary
 */
xmlXPathObjectPtr	*g_bgp_ases;
FILE			*g_bgp_rt_fd;
