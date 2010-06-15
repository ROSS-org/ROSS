#ifndef INC_num_types_h
#define INC_num_types_h

#define NUM_LP_TYPE	 7

FWD(struct, num_state);
FWD(struct, num_message);
FWD(struct, num_statistics);
FWD(struct, num_profile);

// FWD(enum, num_event_t); // cannot overload message types.  Using rn_message
FWD(enum, num_agent_t);
FWD(enum, num_level_t);

// Defines for time in seconds
// define quitting time for internet use at 10:00 AM
//  10:00 - 36000
//   9:15 - 33300
//   9:10 - 33000
//#define QUIT_TIME 36000
#define QUIT_TIME 33000

// file size is in bits
// MIN_FILE_SIZE is 8KBytes
// MAX_FILE_SIZE is 524288 =  64 KBytes
//                 4194304 = 512 KBytes
#define MIN_FILE_SIZE 		65536
#define MAX_FILE_SIZE 		524288
//#define MAX_FILE_SIZE 		4194304

#define EIGHT_OCLOCK		28800
#define NINE_OCLOCK		32400
#define TEN_OCLOCK		36000
#define ELEVEN_OCLOCK		39600
#define FIVE_OCLOCK		61200
#define TWENTY_FOUR_HOURS	86400
#define HALF_HOUR		1800
#define ONE_MINUTE		60
#define TEN_MINUTES		600
#define WEEK_END		432000
#define	WEEK			604800
#define CHECK_TIME		86340

#define NUM_LEVELS		4

DEF(enum, num_agent_t)
{
	NUM_HOME_USER = 0,
	NUM_WORKER,
	NUM_FIN_WORKER,
	NUM_CHILD,
	NUM_OTHER_WORKER
};

// num_level_t enumerates connection speed.
// Should replace with more meaningful names.
DEF(enum, num_level_t)
{
	NUM_LEVEL_0 = 0,
	NUM_LEVEL_1,
	NUM_LEVEL_2,
	NUM_LEVEL_3
};

	/*
	 * num_profile: Network user profiles
	 */
DEF(struct, num_profile)
{
	unsigned int	bitrate;
};

	/*
	 * num_statistics: num model statistics structure
	 *
	 * s_nfiles	number of files received
	 * s_nstart	number of files asked for
	 * s_nstop	number of stops due to agent moving
	 * s_total_time	time waiting for file completion
	 * s_total_kb	total kilobytes received
	 * s_failures	number of times file not received
	 */
DEF(struct, num_statistics)
{
	unsigned int		s_nfiles;
	unsigned int		s_nstart;
	unsigned int		s_nstop;
	unsigned int		s_nfailures;

	unsigned int		s_total_kb;
	tw_stime		s_total_time;
};

	/*
	 * num_state	-- the location LP state
	 *
	 * stats	-- per LP statistics collection structure
	 */
DEF(struct, num_state)
{
	num_agent_t	 type;

	tw_stime	 start;

	unsigned int	 file_size;

	num_statistics	*stats;
};

#endif
