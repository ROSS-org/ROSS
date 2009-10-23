#include <bgp.h>

/**
 * \var Memory queue file descriptors
 *
 * g_bgp_fd		-- membufs for header messages
 * g_bgp_fd_rtes	-- membufs for routes
 * g_bgp_fd_asp		-- membufs for AS paths within routes
 */
tw_fd           g_bgp_fd;
tw_fd           g_bgp_fd_rtes;
tw_fd           g_bgp_fd_asp;

/**
 * \var Global BGP model statistics
 *
 * g_bgp_stats	-- global LP statistics
 * g_bgp_meds	-- MED values for each AS
 * g_bgp_prefs	-- Local Pref values for each AS
 * g_bgp_state	-- global LP state
 * g_bgp_init_file	-- the bgp.conf config file
 */
bgp_stats        g_bgp_stats;
bgp_as		*g_bgp_as = NULL;
bgp_state	*g_bgp_state = NULL;
char		*g_bgp_init_file = NULL;

/**
 * \var To make XML for iBGP connections unnecessary
 */
xmlXPathObjectPtr	*g_bgp_ases;
FILE			*g_bgp_rt_fd;
