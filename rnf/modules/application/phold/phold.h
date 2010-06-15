#ifndef INC_phold_h
#define INC_phold_h

#include <rossnet.h>

	/*
	 * PHOLD Types
	 */

struct phold_state_tag;
typedef struct phold_state_tag phold_state;
struct phold_message_tag;
typedef struct phold_message_tag phold_message;

struct phold_state_tag
{
	long int	 dummy_state;
};

struct phold_message_tag
{
	long int	 dummy_data;
};

extern void phold_init(phold_state * s, tw_lp * lp);
extern void phold_event_handler(phold_state *, tw_bf *, 
				phold_message *, tw_lp *);
extern void phold_event_handler_rc(phold_state * s, tw_bf * bf, 
				   phold_message * m, tw_lp * lp);
extern void phold_finish(phold_state * s, tw_lp * lp);
extern void phold_xml(phold_state * state, 
			const xmlNodePtr node, tw_lp * lp);

#endif
