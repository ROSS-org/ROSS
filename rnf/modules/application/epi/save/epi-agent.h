#ifndef INC_epi_agent_h
#define INC_epi_agent_h

typedef struct epi_agent epi_agent;
typedef struct epi_agent_info epi_agent_info;

typedef enum num_agent_t num_agent_t;
typedef enum epi_stage_t epi_stage_t;
typedef enum epi_agent_t epi_agent_t;
typedef enum epi_agent_profile_t epi_agent_profile_t;

	/*
	 * epi_stage_t:  enumeration of the stages of disease 
	 * not used by code - stages are input
	 */
enum epi_stage_t
{
	EPI_SUSCEPTIBLE = 0,
	EPI_INCUBATING,
	EPI_SYMPTOMATIC,
	EPI_IMMUNE,
	EPI_DECEASED
};

	/*
	 * num_agent_t: enumeration of the various type of agents we represent in the NUM
	 */
enum num_agent_t
{
	NUM_AGT_FIN_TRADER = 1,
	NUM_AGT_FIN_OFFICE
};
	/*
	 * epi_agent_t: enumeration of the various type of agents we represent
	 */
enum epi_agent_t
{
	EPI_AGT_ADULT_1 = 1,
	EPI_AGT_ADULT_2,
	EPI_AGT_PRESCHOOL,
	EPI_AGT_ELEM_SCHOOL,
	EPI_AGT_HIGH_SCHOOL,
	EPI_AGT_COLLEGE,
	EPI_AGT_HEALTH_WORKER,
	EPI_AGT_TEACHER,
	EPI_AGT_WORKER
};

	/*
	 * epi_agent_profile_t: enumeration of network usage profile
	 */
enum epi_agent_profile_t
{
	EPI_PROF_FINANCIAL_1 = 1,
	EPI_PROF_FINANCIAL_2,
};

	/*
	 * epi_egent: representation of a person in the model
	 *
	 * next		-- pointer used by LPs to build priority queue container of agents
	 * prev		-- pointer used by LPs to build priority queue container of agents
	 * up		-- pointer used by LPs to build priority queue container of agents
	 * ts_remove	-- time the agent will remove itself from location LP
	 * ts_next	-- minimum of ts_remove and ts_stage_tran, used for queuing
	 * type		-- the type of person represented
	 * stage	-- disease stage
	 * ts_stage_tran - transition time to next stage 
	 * ts_last_tran	-- time of last transition (i.e. to current stage)
	 * ts_infected	-- time agent caught disease (became infected)
	 * n_infected	-- number of other agents infected by this agent
	 * curr		-- the current position in the location vector
	 * nloc		-- the size of the location vector
	 * loc		-- the location vector
	 */
struct epi_agent
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

	unsigned int	 id;
	int		 curr;
	signed int	 dir;

	unsigned int	 nloc;
	int		*loc;
	tw_stime	*dur;

	// Network User Model Add-Ons
	num_state	*num;
	num_agent_t	 n_type;

	// This is the type for this event
	int		 e_type;
};

#endif
