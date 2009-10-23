#include <ftp.h>

	/*
	 * Initialize the ftp model global variables
	 */
int
ftp_mod_init(ftp_lp_state * state, tw_lp * lp)
{
	bzero(&g_ftp_stats, sizeof(ftp_statistics));

	/*
	 * Trying to send 5 Mb files
	 */
	g_ftp_buffer_size = 5 * 1024;
	g_ftp_buffer = calloc(g_ftp_buffer_size, 1);

	return 0;
}
