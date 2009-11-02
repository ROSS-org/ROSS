#ifndef INC_rm_types_h
#define INC_rm_types_h

FWD(struct, rm_state);
FWD(struct, rm_particle);
FWD(struct, rm_message);
FWD(struct, rm_statistics);
FWD(struct, rm_pe);

FWD(enum, rm_event_t);
DEF(enum, rm_event_t)
{
	RM_PROXIMITY_LP = 100,
	RM_PROXIMITY_ENV = 101,

	// Now can add my own
	RM_SCATTER = 102,
	RM_PARTICLE = 103,
	RM_GATHER = 104,
	RM_WAVE_INIT = 105
};

DEF(struct, rm_statistics)
{
	tw_stat	s_nparticles;
	tw_stat	s_ncell_scatter;
	tw_stat	s_ncell_gather;
	tw_stat	s_ncell_initiate;

};

DEF(struct, rm_pe)
{
	FILE	*wave_log;
	FILE	*move_log;
};

DEF(struct, rm_particle)
{
	double	 range;
	double	 freq;

	double	*position;
	double	*velocity;

	tw_lp	*user_lp;
};

DEF(struct, rm_state)
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
	rm_statistics	*stats;
};

DEF(struct, rm_message)
{
	rm_event_t	 type;
	tw_lpid		 id;
	double		 displacement;
	int		 direction;

	// RC-only variables
	double		 prev_time;
	double		 displ;
	double		 disp[6];
};

#endif
