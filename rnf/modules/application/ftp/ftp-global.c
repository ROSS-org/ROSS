#include <ftp.h>

/*
 * g_ftp_buffer		-- the application data buffer
 * g_ftp_buffer_size  	-- the size of the buffer
 */
char		*g_ftp_buffer = NULL;
unsigned int	 g_ftp_buffer_size = 0;
unsigned long int	 g_ftp_chunk_size = 0;

tw_fd	 g_ftp_fd = 0;

/*
 * g_ftp_stats		-- ftp statistics collection object
 */
ftp_statistics	 g_ftp_stats;
