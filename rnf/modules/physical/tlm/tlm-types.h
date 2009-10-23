#ifndef INC_tlm_types_h
#define INC_tlm_types_h

FWD(struct, tlm_state);
FWD(struct, tlm_particle);
FWD(struct, tlm_message);
FWD(struct, tlm_statistics);
FWD(struct, tlm_pe);

FWD(enum, tlm_event_t);
DEF(enum, tlm_event_t)
{
	RM_PROXIMITY_LP = 100,
	RM_PROXIMITY_ENV = 101,

	// Now can add my own
	RM_SCATTER = 102,
	RM_PARTICLE = 103,
	RM_GATHER = 104,
	RM_WAVE_INIT = 105
};

DEF(struct, tlm_statistics)
{
	tw_stat	s_nparticles;
	tw_stat	s_ncell_scatter;
	tw_stat	s_ncell_gather;
	tw_stat	s_ncell_initiate;

};

DEF(struct, tlm_pe)
{
	FILE	*wave_log;
	FILE	*move_log;
};

DEF(struct, tlm_particle)
{
	double	 range;
	double	 freq;

	double	*position;
	double	*velocity;

	tw_lp	*user_lp;
};

DEF(struct, tlm_state)
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

DEF(struct, tlm_message)
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
