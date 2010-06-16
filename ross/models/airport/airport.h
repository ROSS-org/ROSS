#ifndef INC_airport_h
#define INC_airport_h

#include <ross.h>

#define MEAN_DEPARTURE 30.0
#define MEAN_LAND 10.0

typedef enum airport_event_t airport_event_t;
typedef struct airport_state airport_state;
typedef struct airport_message airport_message;

enum airport_event_t
{
	ARRIVAL = 1, 
	DEPARTURE,
	LAND
};

struct airport_state
{
	int		landings;
	int		planes_in_the_sky;
	int		planes_on_the_ground;

	tw_stime	waiting_time;
	tw_stime	furthest_flight_landing;
};

struct airport_message
{
	airport_event_t	 type;

	tw_stime	 waiting_time;
	tw_stime	 saved_furthest_flight_landing;
};

static tw_lpid	 nlp_per_pe = 1024;
static tw_stime	 mean_flight_time = 1;
static int	 opt_mem = 1000;
static int	 planes_per_airport = 1;

static tw_stime	 wait_time_avg = 0.0;

#endif
