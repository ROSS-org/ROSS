/** @brief Generate and Process Hello Messages
 * 
 *
 *
 */

#include <ross.h>
#include "olsr-constants.h"
#include "olsr-types.h"
#include "olsr-node.h"

void olsr_gen_hello_packet(olsr_node_state* node, olsr_packet* pkt, tw_bf * bf, tw_lp * lp) {	
	pkt->Message_Type = HELLO_MESSAGE;
	pkt->Vtime = NEIGHB_HOLD_TIME;
	pkt->message.hello.Htime = HELLO_INTERVAL;
	pkt->message.hello.Willingness = WILL_DEFAULT;
	pkt->TTL = 1;
	pkt->Originator_Address = node->address;
	pkt->Hop_Count = 0;
	pkt->Message_Sequence_Number = 1;
	pkt->message.hello->Links = olsr_node_links(node);
	
}


void olsr_recv_hello_packet(olsr_node_state* node, olsr_packet* pkt, tw_bf * bf, tw_lp * lp) {
	uint32_t address = pkt->Originator_Address;
	HELLO_entry* h = olsr_check_list_addresses(pkt->Originator_Address, pkt->message.hello->Links);
	if(h == NULL) {
		olsr_add_link(node, pkt, h, bf, lp);
	}
	else {
		olsr_mod_link(node, pkt, h, bf, lp);
	}
}

void olsr_add_link(olsr_node_state* node, olsr_packet* pkt, HELLO_entry* h, tw_bf * bf, tw_lp * lp) {
	link_tuple link;
	link.local_iface_addr = node->address;
	link.neighbor_iface_addr = pkt->Originator_Address; 
	link.SYM_time = node->now - 1; 
	link.time = node->now + pkt->Vtime;
}

void olsr_mod_link(olsr_node_state* node, olsr_packet* pkt, HELLO_entry* h, tw_bf * bf, tw_lp * lp) {
	link_tuple link;
	
	link.local_iface_addr = node->address;
	link.neighbor_iface_addr = pkt->Originator_Address; 
	link.ASYM_time = node->now + pkt->Vtime;
	
	switch(h->Link_Type) {
		case LOST_LINK:
			link.SYM_time = node->now - 1;
		break;
		
		case SYM_LINK:
		case ASYM_LINK:
			link.SYM_time = node->now + pkt->Vtime;
			link.time = link.SYM_time + NEIGHB_HOLD_TIME;
		break;
	}
	
	link.time = max(link.time, link.ASYM_time);
}

