#ifndef INC_rw_types_h
#define INC_rw_types_h

#define TW_RW_LP	1

#define Q_SIZE		1000000
#define PI		M_PI

FWD(struct, rw_state);
FWD(struct, rw_message);
FWD(struct, rw_statistics);

FWD(struct, rw_connection);
FWD(enum, rw_event_t);
FWD(enum, rw_conn_t);

DEF(enum, rw_conn_t)
{
	RW_DIRECT = 1
};

DEF(struct, rw_connection)
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

DEF(enum, rw_event_t)
{
	RW_TIMESTEP = 1,
	RW_MOVE,
	RW_MULTIHOP_CALC
};

DEF(struct, rw_statistics)
{
	tw_stat s_move_ev;
	tw_stat s_prox_ev;
	tw_stat s_recv_ev;

	tw_stat s_nwaves;
	tw_stat s_nconnect;
	tw_stat s_ndisconnect;
};

DEF(struct, rw_state)
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

DEF(struct, rw_message)
{
	rw_event_t	 type;
	tw_lpid		 from;

	double		 x_pos;
	double		 y_pos;
	double		 theta;
	int		 change_dir;
};

#endif
