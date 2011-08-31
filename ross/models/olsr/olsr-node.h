/** @brief 
 *
 */

#include "olsr-types.h"
#include "mobilty.h"

typedef struct {
/* Node	Characteristics */
	uint32_t			address;
	tw_grid_pt			location;
	double				tx_power;
	tw_stime			now;
/* Node	Sets */	
	duplicate_tuple*     duplicate_set;
	link_tuple*          link_set;
	neighbor_tuple*      neighbor_set;
	two_hop_tuple*	     two_hop_neighbor_set;
	uint32_t*			 mpr_set;
	mpr_selector_tuple*  mpr_selector_set;
	topology_tuple* 	 topology_set;
} olsr_node_state;

