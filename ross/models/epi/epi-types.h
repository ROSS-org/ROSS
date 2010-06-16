#ifndef INC_epi_types_h
#define INC_epi_types_h

#define EPI_LP_TYPE	 6
#define NINE_OCLOCK		32400
#define TWENTY_FOUR_HOURS	86400

typedef struct epi_state epi_state;
typedef struct epi_statistics epi_statistics;
typedef struct epi_message epi_message;
typedef struct epi_disease epi_disease;
typedef struct epi_ic_stage epi_ic_stage;
typedef struct epi_pathogen epi_pathogen;
typedef struct epi_agent epi_agent;
typedef struct epi_agent_info epi_agent_info;

typedef enum epi_event_t epi_event_t;
typedef enum epi_grid_t epi_grid_t;
typedef enum epi_stage_t epi_stage_t;
typedef enum epi_agent_t epi_agent_t;
typedef enum epi_agent_profile_t epi_agent_profile_t;
typedef enum epi_agent_behavior_t epi_agent_behavior_t;

	/*
	 * epi_event_t: enumeration of the various types of events we handle
	 * (purposely set EPI_REMOVE to zero, since this is a timer event)
	 */
enum epi_event_t
{
	EPI_REMOVE,
	EPI_ADD,
	EPI_STAGE
};

struct epi_message
{
	//epi_event_t	 type;
};

	/*
	 * epi_grid_t: enumeration of grid (location) types
	 */
enum epi_grid_t
{
	EPI_GRID_HOME = 1,
	EPI_GRID_OFFICE_1,
	EPI_GRID_OFFICE_2,
	EPI_GRID_SCHOOL,
	EPI_GRID_BUS_STOP,
	EPI_GRID_BUS,
	EPI_GRID_AUTO,
	EPI_GRID_TAXI,
	EPI_GRID_HOSPITAL,
	EPI_GRID_STORE
};

	/*
	 *  epi_agent_behavior_t: Flags for behavior states
	 *
	 */
enum epi_agent_behavior_t
{
	EPI_AGENT_NORMAL = 0,
	EPI_AGENT_WORRIED_WELL,
	EPI_AGENT_WORK_WHILE_SICK,
	EPI_AGENT_DEAD
};

	/*
	 * epi_statistics: Epi model statistics structure
	 *
	 * s_move_ev	-- how many times did we get a move event?
	 * s_nchecked	-- number of agents checked by seir
	 * s_ndraws	-- total draws from random uniform distribution
	 * s_ninfected	-- number of agents who have become infected
	 */
struct epi_statistics
{
	tw_stat		s_move_ev;
	tw_stat		s_nchecked;
	tw_stat		s_ndraws;
	tw_stat		s_ww_pop;
	tw_stat		s_wws_pop;

	tw_stat		s_ninfected;
	tw_stat		s_ndead;
};

	/*
	 * epi_state	-- the location LP state
	 *
	 * grid_type	-- type of this location: HOME, WORK, etc
	 * stats	-- per LP statistics collection structure
	 * ncontagious	-- number of symptomatic agents at this location, by disease
	 * ncontagious_tot -- total number of infectious agents at location
	 * last_time	-- last time that an seir check was performed
	 *
	 * hospital	-- the hospital this agent visits if sick
	 */
struct epi_state
{
	//epi_grid_t	 grid_type;

	tw_event	*tmr_remove;
	tw_stime	*ts_seir;

	unsigned int	 max_queue_size;
	unsigned int	*ncontagious;
	unsigned int	 ncontagious_tot;
	unsigned int	 hospital;

	epi_statistics	*stats;
};

	/*
	 * epi_disease: describes a disease and it's stages
	 */
struct epi_disease
{
	char		 name[255];
	unsigned int	 nstages;
	epi_ic_stage	*stages;
};

	/*
	 * epi_ic_stage describes a stage in the illness.  Normal
	 * progression goes from Susceptible (non-infected) to
	 * infected (incubating), to infected (symptomatic), to
	 * recovered (immune).
	 *
	 * ln_multiplier is used for the random draw from geometric
	 * distribution.
	 */
struct epi_ic_stage
{
	unsigned int	 index;
	char		 name[255];
	double		 start_multiplier,
			 stop_multiplier;
	double		 ln_multiplier;
	double		 min_duration,
			 max_duration;
	double		 mortality_rate;
	int		 eligible_for_inoculation,
			 progress_trumps_inoculation,
			 hospital_treatment,
			 is_symptomatic;
	double		 days_in_hospital;
};

	/*
	 * epi_stage_t:  enumeration of the stages of disease 
	 * not used by code - stages are input
	 */
enum epi_stage_t
{
	EPI_SUSCEPTIBLE = 0,
	EPI_EXPOSED,
	EPI_INFECTIOUS,
	EPI_RECOVERED,
	EPI_DECEASED
};

	/*
	 * epi_agent_t: enumeration of the various type of agents we represent
	 */
enum epi_agent_t
{
/*
	EPI_AGT_ADULT_1 = 1,
	EPI_AGT_ADULT_2,
	EPI_AGT_PRESCHOOL,
	EPI_AGT_ELEM_SCHOOL,
	EPI_AGT_HIGH_SCHOOL,
	EPI_AGT_COLLEGE,
	EPI_AGT_HEALTH_WORKER,
	EPI_AGT_TEACHER,
	EPI_AGT_WORKER
*/
	EPI_AGT_NOT_A_WORKER = 0,
	EPI_AGT_WORKER,
	EPI_AGT_FINANCIAL_WORKER,
	EPI_AGT_CHILD,
	EPI_AGT_FIRM_A_WORKER
};

	/*
	 * epi_agent_profile_t: enumeration of network usage profile
	 */
enum epi_agent_profile_t
{
	EPI_PROF_FINANCIAL_1 = 1,
	EPI_PROF_FINANCIAL_2
};

struct epi_pathogen
{
	unsigned int	 index;
	unsigned int	 stage;

	tw_stime	 ts_stage_tran;
};

	/*
	 * epi_egent: representation of a person in the model
	 *
	 * next		-- pointer used by LPs to build priority queue container of agents
	 * prev		-- pointer used by LPs to build priority queue container of agents
	 * up		-- pointer used by LPs to build priority queue container of agents
	 * ts_remove	-- time the agent will remove itself from location LP
	 * ts_next	-- minimum of ts_remove and ts_stage_tran, used for queuing
	 * a_type	-- the type of person represented
	 * stage	-- disease stage
	 * ts_stage_tran - transition time to next stage 
	 * ts_last_tran	-- time of last transition (i.e. to current stage)
	 * ts_infected	-- time agent caught disease (became infected)
	 * n_infected	-- number of other agents infected by this agent
	 * behavior_flags  boolean bit flags for behaviors
	 *	1 - worried well
	 *	2 - goes to work (school) when sick
	 *	4 - 
	 * days_remaining  number of days before going back to work
	 * curr		-- the current position in the location vector
	 * nloc		-- the size of the location vector
	 * loc		-- the location vector
	 *
	 * region	-- "home" region for reporting statistics
	 */
struct epi_agent
{
	// now that agents are stored in memory buffers, we no longer need these ptrs
	/* these 4 vars are need for PQ */
	//epi_agent	*volatile next;
	//epi_agent	*volatile prev;
	//epi_agent	*volatile up;
	//int		 heap_index;

	tw_lpid		 id;

	tw_stime	 ts_remove;
	tw_stime	 ts_next;

	tw_memory	*pathogens;

	//tw_stime	 ts_infected;
	//unsigned int	 n_infected;

	epi_agent_behavior_t	 behavior_flags;
	int		 days_remaining;

	int		 curr;
	int		 nloc;
	tw_lpid		 loc[10];
	tw_stime	 dur[10];

	int		 region;
};

#endif
