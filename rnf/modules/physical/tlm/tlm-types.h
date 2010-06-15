#ifndef INC_tlm_types_h
#define INC_tlm_types_h

struct tlm_state_t;
typedef tlm_state_t tlm_state;
struct tlm_particle_t;
typedef tlm_particle_t tlm_particle;
struct tlm_message_t;
typedef tlm_message_t tlm_message;
struct tlm_statistics_t;
typedef tlm_statistics_t tlm_statistics;
struct tlm_pe_t;
typedef tlm_pe_t tlm_pe;

enum tlm_event_t_t;
typedef tlm_event_t_t tlm_event_t;
enum tlm_event_t_t
{
	RM_PROXIMITY_LP = 100,
	RM_PROXIMITY_ENV = 101,

	// Now can add my own
	RM_SCATTER = 102,
	RM_PARTICLE = 103,
	RM_GATHER = 104,
	RM_WAVE_INIT = 105
};

struct tlm_statistics_t
{
	tw_stat	s_nparticles;
	tw_stat	s_ncell_scatter;
	tw_stat	s_ncell_gather;
	tw_stat	s_ncell_initiate;

};

struct tlm_pe_t
{
	FILE	*wave_log;
	FILE	*move_log;
};

struct tlm_particle_t
{
	double	 range;
	double	 freq;

	double	*position;
	double	*velocity;

	tw_lp	*user_lp;
};

struct tlm_state_t
{
	/*
	 * particles	-- queue of range particles
	 * nbrs		-- neighboring CELL LPs
	 */
	tw_memoryq	*particles;
	tw_lpid		*nbrs;

	/* next_time	-- next scatter time */
	tw_stime	 next_time;

	/*
	 * displacement		-- overall displacement
	 * displacements	-- displacement from a given direction
	 */
	double		 displacement;
	double		*displacements;

	/* per LP statistics */
	tlm_statistics	*stats;
};

struct tlm_message_t
{
	tlm_event_t	 type;
	tw_lpid		 id;
	double		 displacement;
	int		 direction;

	// RC-only variables
	double		 prev_time;
	double		 displ;
	double		 disp[6];
};

#endif
