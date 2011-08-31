/**
 *  @file olsr-types.h
 *  @brief File containing the types used in OLSR routing.
 *
 */
#include <stdint.h>

#ifndef _OLSR_TYPES_H_
#define _OLSR_TYPES_H_


#define MAX_RADIOS 30
#define MAX_DUPLICATE_SET 1000

typedef struct {
	uint32_t*	OLSR_Interface_Address;
} MID_message;

typedef struct {
	uint16_t	ANSN; /**< Advertised Neighbor Sequence Number */
	uint32_t*	ANMA; /**< Advertised Neighbor Main Address */
} TC_message;

typedef struct {
	uint32_t	Address;
	uint32_t	Netmask;
} HNA_entry;

typedef struct {
	HNA_entry*	HNA_table;
} HNA_message;

struct {
	uint8_t		Link_Type;
	uint8_t		Neighbor_Type;
	uint16_t	Link_Message_Size;
	uint32_t*	Neighbor_Interface_Address;
} HELLO_entry;

typedef struct {
	double			Htime;
	uint8_t			Willingness;
	HELLO_entry* 	Links;
} HELLO_message;

typedef struct {
	uint16_t	Packet_Length;
	uint16_t	Packet_Sequence_Number;
	uint8_t		Message_Type;
	double		Vtime;
	uint16_t	Message_Size;
	uint32_t	Originator_Address;
	uint8_t		TTL;
	uint8_t		Hop_Count;
	uint16_t	Message_Sequence_Number;

	union {
		HELLO_message	hello;
		MID_message		mid;
		TC_message		tc;
		HNA_message		hna;
		_Bool			data_only;
	} message;
	
} olsr_packet;

typedef struct {
	uint32_t		addr;
	uint16_t		seq_num;
	_Bool			retransmitted;
	uint32_t*		iface_list;
	double			time;
} duplicate_tuple;

typedef struct {
	uint32_t		iface_addr;
	uint32_t		main_addr; 
	double			time;
} interface_association_tuple;

typedef struct {
	uint32_t	local_iface_addr;
	uint32_t	neighbor_iface_addr; 
	double		SYM_time; 
	double		ASYM_time; 
	double		time;
} link_tuple; 

typedef struct {
	uint32_t	neighbor_main_addr;
	uint32_t	status; 
	uint8_t		willingness;
} neighbor_tuple;

typedef struct {
	uint32_t	neighbor_main_addr;
	uint32_t	two_hop_addr;
	double		time;
} two_hop_tuple;

typedef struct {
	uint32_t	main_addr;
	double		time;
} mpr_selector_tuple; 

typedef struct {
	uint32_t	dest_addr;
	uint32_t	last_addr;
	uint16_t	seq;
	double		time;
} topology_tuple; 

typedef struct {
	uint32_t	gateway_addr;
	uint32_t	network_addr;
	uint32_t	netmask;
	double		time;
} association_tuple;

/**
 * \brief Convert packet integer time to time in seconds
 * 
 * From RFC-3626, the conversion formula is:
 * \f$  C(1+\frac{a}{16})2^b \f$
 * Where a is the upper nibble, and b is the lower nibble.
 */
double uint8_time_to_double(uint8_t htime);

/**
 * \brief Convert time in seconds to header time block
 * 
 * From RFC-3626 Section 18.3 (p 65): [W]ay of computing the mantissa/exponent 
 * representation of a number T (of seconds) is the following:
 *  	
 *    - Find the largest integer ’b’ such that: \f$ T/C >= 2^b \f$
 *    - Compute the expression \f$ 16*(T/(C*(2^b))-1) \f$, which may not be a 
 *      integer, and round it up. This results in the value for ’a’
 *    - If ’a’ is equal to 16: increment ’b’ by one, and set ’a’ to 0
 *    - Now, ’a’ and ’b’ should be integers between 0 and 15, and the field 
 *      will be a byte holding the value \f$ a*16+b \f$
 * 
 *
 */
uint8_t double_time_to_uint8(double time);


#endif