#ifndef INC_rp_types_h
#define INC_rp_types_h

#define TW_RP_LP	1

#define Q_SIZE		1000000
#define PI		M_PI

FWD(struct, rp_state);
FWD(struct, rp_message);
FWD(struct, rp_statistics);

FWD(struct, rp_connection);
FWD(enum, rp_event_t);
FWD(enum, rp_conn_t);

#define physReal double

// Physics System model include data types
/************************************************************************/
/* Control interface                                                    */
/************************************************************************/
struct _physControlInput
{
	// goal location
	physReal	goalX;
	physReal	goalY;	// goalZ should be inferred from the map

	physReal	currentX; // optional
	physReal	currentY; // 

	physReal	maxTime; //
};

struct _physControlOutput
{
	int			status; //

	physReal	endX;
	physReal	endY;
	physReal	endZ;
};

/************************************************************************/
/* Query interface                                                      */
/************************************************************************/
struct _physQueryInput
{
	int type;
};

struct _physQueryOutput
{
	physReal	posX;
	physReal	posY;
	physReal	posZ;
};

/************************************************************************/
/*                                                                      */
/************************************************************************/

struct _robotDesc 
{
	// Robot body
	physReal	cogX,cogY,cogZ; // center of gravity
	physReal	inertiaX,inertiaY,inertiaZ ; // principle moment of inertia
	physReal	mass;

	// Wheels
	physReal	w1x,w1y,w1z; // wheels location w.r.t to the cog
	physReal	w2x,w2y,w2z;
	physReal	w3x,w3y,w3z;
	physReal	w4x,w4y,w4z;

	physReal	w1r;		// radius
	physReal	w2r;
	physReal	w3r;
	physReal	w4r;
};

struct _robotState
{
	// Body
	physReal	x,y,z; // position of cog in simulation	
	physReal	rx,ry,rz; // rotation
	physReal	drive; // rotation of front wheels w.r.t to body
};

struct _robot
{
	struct _robotDesc	d;
	struct _robotState	s;
};


/************************************************************************/
/* Interface functions                                                 */
/************************************************************************/

extern int physSimControl(struct _physControlInput* inp, 
			  struct _physControlOutput* outp,
			  struct _robot* r);

int physSimQuery(struct _physQueryInput* inp, 
		 struct _physQueryOutput* outp,
		 struct _robot* r);


DEF(enum, rp_conn_t)
{
	RW_DIRECT = 1
};

DEF(struct, rp_connection)
{
	unsigned int	 id;
	unsigned int	 nid;

	tw_lpid		 to;

	tw_stime	 start;
	tw_stime	 end;

	double		 metric;
	double		 pl;

	rp_conn_t	 type;
};

DEF(enum, rp_event_t)
{
	RP_TIMESTEP = 1,
	RP_MOVE,
	RP_MULTIHOP_CALC
};

DEF(struct, rp_statistics)
{
	tw_stat s_move_ev;
	tw_stat s_prox_ev;
	tw_stat s_recv_ev;

	tw_stat s_nwaves;
	tw_stat s_nconnect;
	tw_stat s_ndisconnect;
};

DEF(struct, rp_state)
{
	double		*position;
	double		*velocity;

	rp_statistics	*stats;

	unsigned int	*rt;

	tw_stime	 prev_time;

	double		 transmit;
	double		 range;
	double		 frequency;

	int		 change_dir;
	int		 theta;

	struct _physControlInput pc_input;
	struct _physControlOutput pc_output;
	struct _robot      robot;
};

DEF(struct, rp_message)
{
	rp_event_t	 type;
	tw_lpid		 from;

	double		 x_pos;
	double		 y_pos;
	double		 theta;
	int		 change_dir;
};

#endif
