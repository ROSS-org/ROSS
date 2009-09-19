#ifndef INC_ftp_types_h
#define INC_ftp_types_h

#define FTP_START_EVENTS 1	
#define FTP_SEND_TIME (double) 1.0

FWD(enum, ftp_event_states);
FWD(struct, ftp_message);
FWD(struct, ftp_lp_state);
FWD(struct, ftp_statistics);

DEF(enum, ftp_event_states)
{
	FTP_READ,
	FTP_SEND,
	FTP_COMPLETE
};

DEF(struct, ftp_message)
{
	ftp_event_states	type;

	unsigned int		size;
};

DEF(struct, ftp_lp_state)
{
	unsigned long int	bytes_read;
	unsigned long int	bytes_sent;

	unsigned long int	files_sent;
	unsigned long int	files_read;
};

DEF(struct, ftp_statistics)
{
	unsigned long int	s_bytes_read;
	unsigned long int	s_bytes_sent;

	unsigned long int	s_files_sent;
	unsigned long int	s_files_read;
};

#endif
