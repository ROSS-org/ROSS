#ifndef INC_ip_traffic_types_h
#define INC_ip_traffic_types_h

#define FTP_START_EVENTS 1	
#define FTP_SEND_TIME (double) 1.0

enum ftp_event_states_t;
typedef ftp_event_states_t ftp_event_states;
struct ftp_message_t;
typedef ftp_message_t ftp_message;
struct ftp_lp_state_t;
typedef ftp_lp_state_t ftp_lp_state;
struct ftp_statistics_t;
typedef ftp_statistics_t ftp_statistics;

enum ftp_event_states_t
{
	FTP_READ,
	FTP_SEND
};

struct ftp_message_t
{
	ftp_event_states	type;

	unsigned int		size;
};

struct ftp_lp_state_t
{
	unsigned long int	bytes_read;
	unsigned long int	bytes_sent;

	unsigned long int	files_sent;
	unsigned long int	files_read;
};

struct ftp_statistics_t
{
	unsigned long int	s_bytes_read;
	unsigned long int	s_bytes_sent;

	unsigned long int	s_files_sent;
	unsigned long int	s_files_read;
};

#endif
