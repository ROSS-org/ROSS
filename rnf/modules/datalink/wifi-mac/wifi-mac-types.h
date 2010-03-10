#ifndef INC_wifi_mac_types_h
#define INC_wifi_mac_types_h

/***************************************************/
/* FWD Declaration of Wifi-Mac Types ***************/
/***************************************************/

FWD(struct, wifi_mac_state);
FWD(struct, wifi_mac_particle);
FWD(struct, wifi_mac_message);
FWD(struct, wifi_mac_statistics);
FWD(struct, wifi_mac_pe);
FWD(enum, wifi_mac_event_t);

/***************************************************/
/* Actual Declaration of Wifi-Mac Types ************/
/***************************************************/

enum State {
  /**
   * The PHY layer is synchronized upon a packet.
   */
  SYNC,
  /**
   * The PHY layer is sending a packet.
   */
  TX,
  /**
   * The PHY layer has sense the medium busy through
   * the CCA mechanism
   */
  CCA_BUSY,
  /**
   * The PHY layer is IDLE.
   */
  IDLE,
  /**
   * The PHY layer is switching to other channel.
   */
  SWITCHING
};


DEF(enum, wifi_mac_event_t)
{
	RM_PROXIMITY_LP = 100,
	RM_PROXIMITY_ENV = 101,

	// Now can add my own
	RM_SCATTER = 102,
	RM_PARTICLE = 103,
	RM_GATHER = 104,
	RM_WAVE_INIT = 105
};

DEF(struct, wifi_mac_statistics)
{
	tw_stat	s_nparticles;
	tw_stat	s_ncell_scatter;
	tw_stat	s_ncell_gather;
	tw_stat	s_ncell_initiate;

};

DEF(struct, wifi_mac_pe)
{
	FILE	*wave_log;
	FILE	*move_log;
};

DEF(struct, wifi_mac_particle)
{
	double	 range;
	double	 freq;

	double	*position;
	double	*velocity;

	tw_lp	*user_lp;
};

DEF(struct, wifi_mac_state)
{
	bool rxing;
	tw_stime endTx;
	tw_stime endRx;
	tw_stime endCcaBusy;
	tw_stime endSwitching; 
	tw_stime startTx;
	tw_stime startRx;
	tw_stime startCcaBusy;
	tw_stime startSwitching; 
	tw_stime previousStateChangeTime;
	
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
	wifi_mac_statistics	*stats;
};

DEF(struct, wifi_mac_message)
{
	wifi_mac_event_t	 type;
	tw_lpid		 id;
	double		 displacement;
	int		 direction;

	// RC-only variables
	double		 prev_time;
	double		 displ;
	double		 disp[6];
};

#endif
