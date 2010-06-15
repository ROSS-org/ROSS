#ifndef INC_tlm_types_h
#define INC_tlm_types_h

struct tlm_state_tag;
typedef struct tlm_state_tag tlm_state;
struct tlm_particle_tag;
typedef struct tlm_particle_tag tlm_particle;
struct tlm_message_tag;
typedef struct tlm_message_tag tlm_message;
struct tlm_statistics_tag;
typedef struct tlm_statistics_tag tlm_statistics;
struct tlm_pe_tag;
typedef struct tlm_pe_tag tlm_pe;

enum tlm_event_t_tag;
typedef enum tlm_event_t_tag tlm_event_t;
enum tlm_event_t_tag
{
	RM_PROXIMITY_LP = 100,
	RM_PROXIMITY_ENV = 101,

	// Now can add my own
	RM_SCATTER = 102,
	RM_PARTICLE = 103,
	RM_GATHER = 104,
	RM_WAVE_INIT = 105
};

struct tlm_statistics_tag
{
	tw_stat	s_nparticles;
	tw_stat	s_ncell_scatter;
	tw_stat	s_ncell_gather;
	tw_stat	s_ncell_initiate;

};

struct tlm_pe_tag
{
	FILE	*wave_log;
	FILE	*move_log;
};

struct tlm_particle_tag
{
	double	 range;
	double	 freq;

	double	*position;
	double	*velocity;

	tw_lp	*user_lp;
};

struct tlm_state_tag
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

struct tlm_message_tag
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
