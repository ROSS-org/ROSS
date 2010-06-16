#ifndef INC_ip_traffic_types_h
#define INC_ip_traffic_types_h

#define FTP_START_EVENTS 1	
#define FTP_SEND_TIME (double) 1.0

typedef enum ftp_event_states ftp_event_states;
typedef struct ftp_message ftp_message;
typedef struct ftp_lp_state ftp_lp_state;
typedef struct ftp_statistics ftp_statistics;

enum ftp_event_states
{
	FTP_READ,
	FTP_SEND
};

struct ftp_message
{
	ftp_event_states	type;

	unsigned int		size;
};

struct ftp_lp_state
{
	unsigned long int	bytes_read;
	unsigned long int	bytes_sent;

	unsigned long int	files_sent;
	unsigned long int	files_read;
};

struct ftp_statistics
{
	unsigned long int	s_bytes_read;
	unsigned long int	s_bytes_sent;

	unsigned long int	s_files_sent;
	unsigned long int	s_files_read;
};

#endif
