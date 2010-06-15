#ifndef INC_ip_traffic_types_h
#define INC_ip_traffic_types_h

#define FTP_START_EVENTS 1	
#define FTP_SEND_TIME (double) 1.0

enum ftp_event_states_tag;
typedef enum ftp_event_states_tag ftp_event_states;
struct ftp_message_tag;
typedef struct ftp_message_tag ftp_message;
struct ftp_lp_state_tag;
typedef struct ftp_lp_state_tag ftp_lp_state;
struct ftp_statistics_tag;
typedef struct ftp_statistics_tag ftp_statistics;

enum ftp_event_states_tag
{
	FTP_READ,
	FTP_SEND
};

struct ftp_message_tag
{
	ftp_event_states	type;

	unsigned int		size;
};

struct ftp_lp_state_tag
{
	unsigned long int	bytes_read;
	unsigned long int	bytes_sent;

	unsigned long int	files_sent;
	unsigned long int	files_read;
};

struct ftp_statistics_tag
{
	unsigned long int	s_bytes_read;
	unsigned long int	s_bytes_sent;

	unsigned long int	s_files_sent;
	unsigned long int	s_files_read;
};

#endif
