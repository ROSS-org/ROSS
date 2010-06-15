#ifndef INC_rw_types_h
#define INC_rw_types_h

#define TW_RW_LP	1

#define Q_SIZE		1000000
#define PI		M_PI

struct rw_state_t;
typedef rw_state_t rw_state;
struct rw_message_t;
typedef rw_message_t rw_message;
struct rw_statistics_t;
typedef rw_statistics_t rw_statistics;

struct rw_connection_t;
typedef rw_connection_t rw_connection;
enum rw_event_t_t;
typedef rw_event_t_t rw_event_t;
enum rw_conn_t_t;
typedef rw_conn_t_t rw_conn_t;

enum rw_conn_t_t
{
	RW_DIRECT = 1
};

struct rw_connection_t
{
	unsigned int	 id;
	unsigned int	 nid;

	tw_lpid		 to;

	tw_stime	 start;
	tw_stime	 end;

	double		 metric;
	double		 pl;

	rw_conn_t	 type;
};

enum rw_event_t_t
{
	RW_TIMESTEP = 1,
	RW_MOVE,
	RW_MULTIHOP_CALC
};

struct rw_statistics_t
{
	tw_stat s_move_ev;
	tw_stat s_prox_ev;
	tw_stat s_recv_ev;

	tw_stat s_nwaves;
	tw_stat s_nconnect;
	tw_stat s_ndisconnect;
};

struct rw_state_t
{
#if 0
	tw_memory	**direct;
	tw_memoryq	*direct_hist;
#endif

	double		*position;
	double		*velocity;

	rw_statistics	*stats;

	unsigned int	*rt;

	tw_stime	 prev_time;

	double		 transmit;
	double		 range;
	double		 frequency;

	int		 change_dir;
	int		 theta;

#if 0
	double		 pathloss1;
	double		 slope1;
	double		 slope2;
	double		 breakpoint;
	double		 log_bp;
	double		 threshold;
	double		 speed;
	double		 rate;
#endif
};

struct rw_message_t
{
	rw_event_t	 type;
	tw_lpid		 from;

	double		 x_pos;
	double		 y_pos;
	double		 theta;
	int		 change_dir;
};

#endif
