#ifndef INC_epi_types_h
#define INC_epi_types_h

#define EPI_LP_TYPE	 6
#define NINE_OCLOCK		32400
#define TWENTY_FOUR_HOURS	86400

struct epi_state_tag;
typedef struct epi_state_tag epi_state;
struct epi_message_tag;
typedef struct epi_message_tag epi_message;
struct epi_statistics_tag;
typedef struct epi_statistics_tag epi_statistics;
struct epi_ic_stage_tag;
typedef struct epi_ic_stage_tag epi_ic_stage;
struct epi_agent_tag;
typedef struct epi_agent_tag epi_agent;
struct epi_agent_info_tag;
typedef struct epi_agent_info_tag epi_agent_info;

enum epi_event_t_tag;
typedef enum epi_event_t_tag epi_event_t;
enum epi_grid_t_tag;
typedef enum epi_grid_t_tag epi_grid_t;
enum epi_stage_t_tag;
typedef enum epi_stage_t_tag epi_stage_t;
enum epi_agent_t_tag;
typedef enum epi_agent_t_tag epi_agent_t;
enum epi_agent_profile_t_tag;
typedef enum epi_agent_profile_t_tag epi_agent_profile_t;
enum epi_agent_behavior_t_tag;
typedef enum epi_agent_behavior_t_tag epi_agent_behavior_t;

	/*
	 * epi_event_t: enumeration of the various types of events we handle
	 */
enum epi_event_t_tag
{
	EPI_ADD = 1,
	EPI_REMOVE,
	EPI_STAGE_TRANSITION
};

	/*
	 * epi_grid_t: enumeration of grid (location) types
	 */
enum epi_grid_t_tag
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
enum epi_agent_behavior_t_tag
{
	EPI_AGENT_NORMAL = 0,
	EPI_AGENT_WORRIED_WELL = 1,
	EPI_AGENT_WORK_WHILE_SICK = 2
};

	/*
	 * epi_statistics: Epi model statistics structure
	 *
	 * s_move_ev	-- how many times did we get a move event?
	 * s_nchecked	-- number of agents checked by seir
	 * s_ndraws	-- total draws from random uniform distribution
	 * s_ninfected	-- number of agents who have become infected
	 */
struct epi_statistics_tag
{
	unsigned long int	s_move_ev;
	unsigned long int	s_nchecked;
	unsigned long int	s_ndraws;
	unsigned long int	s_ninfected;
	unsigned long int	s_ndead;
};

	/*
	 * epi_state	-- the location LP state
	 *
	 * grid_type	-- type of this location: HOME, WORK, etc
	 * pq		-- priority queue pointer to global location PQ
	 * stats	-- per LP statistics collection structure
	 * ncontagious	-- number of symptomatic agents at this location
	 * last_event	-- last time that an seir check was performed
	 */

	/*
	 * NOTE: The issue is that we can use different types of PQ's, and since we
	 * do not have iterators defined, we cannot walk the unknown data structure impl.
	 */
struct epi_state_tag
{
	epi_grid_t	 grid_type;
	void		*pq;
	tw_event	*tmr_remove;
	tw_stime	 last_event;
	unsigned int	 max_queue_size;
	unsigned int	 ncontagious;
	epi_statistics	*stats;
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
struct epi_ic_stage_tag
{
	unsigned int	 stage_index;
	char		*stage_name;
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
enum epi_stage_t_tag
{
	EPI_SUSCEPTIBLE = 0,
	EPI_INCUBATING,
	EPI_SYMPTOMATIC,
	EPI_IMMUNE,
	EPI_DECEASED
};

	/*
	 * epi_agent_t: enumeration of the various type of agents we represent
	 */
enum epi_agent_t_tag
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
enum epi_agent_profile_t_tag
{
	EPI_PROF_FINANCIAL_1 = 1,
	EPI_PROF_FINANCIAL_2
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
	 */
struct epi_agent_tag
{
	/* these 4 vars are need for PQ */
	epi_agent	*volatile next;
	epi_agent	*volatile prev;
	epi_agent	*volatile up;
	int		 heap_index;

	tw_stime	 ts_remove;
	tw_stime	 ts_next;

	epi_agent_t	 a_type;
	epi_stage_t	 stage;

	tw_stime	 ts_stage_tran;
	tw_stime	 ts_last_tran;

	tw_stime	 ts_infected;
	unsigned int	 n_infected;

	epi_agent_behavior_t	 behavior_flags;
	int		 days_remaining;

	unsigned int	 id;
	int		 curr;
	unsigned int	 nloc;
	int		*loc;
	tw_stime	*dur;

	//	epi_agent_state_t agent_behavior; // replaced by behavior_flags

	// the NUM we are connected to
	tw_lp		*num;

	// This is the type for this event
	epi_event_t	 e_type;

	// Census Tract we are in
	unsigned int	 ct;
};

#endif
