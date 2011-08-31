#ifndef INC_pm_types_h
#define INC_pm_types_h

#define TW_PM_LP	1

#define Q_SIZE		1000000
#define PI		M_PI

typedef struct pm_state pm_state;
typedef struct pm_message pm_message;
typedef struct pm_statistics pm_statistics;

typedef struct pm_connection pm_connection;
typedef enum pm_event_t pm_event_t;
typedef enum pm_conn_t pm_conn_t;

enum pm_conn_t
{
	PM_DIRECT = 1
};

struct pm_connection
{
	unsigned int	 id;
	unsigned int	 nid;

	tw_lpid		 to;

	tw_stime	 start;
	tw_stime	 end;

	double		 metric;
	double		 pl;

	pm_conn_t	 type;
};

enum pm_event_t
{
	PM_TIMESTEP = 1,
	PM_MOVE,
	PM_MULTIHOP_CALC
};

struct pm_statistics
{
	tw_stat	s_move_ev;
	tw_stat s_nwaves;

	tw_stat s_recv_ev;
	tw_stat s_prox_ev;
	tw_stat s_nconnect;
	tw_stat s_ndisconnect;
};

struct pm_state
{
#if 0
	tw_memory	**direct;
	tw_memoryq	*direct_hist;
	int		 id;
#endif


	double		*position;
	double		*velocity;

	pm_statistics	*stats;

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

struct pm_message
{
	pm_event_t	 type;
	tw_lpid		 from;

	double		 x_pos;
	double		 y_pos;
	double		 theta;
	int		 change_dir;
};

#endif
