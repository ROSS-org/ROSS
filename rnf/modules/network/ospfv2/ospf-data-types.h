#ifndef INC_ospf_data_types_h
#define INC_ospf_data_type_h

#define OSPF_ROUTER 1

#define OSPF_LSA_MAX_AGE_DIFF	 	9000

#define OSPF_HEADER		24
#define OSPF_DD_HEADER		8
#define OSPF_HELLO_HEADER	32
#define OSPF_ACK_HEADER		20
#define OSPF_LSA_UPDATE_HEADER	20
#define OSPF_LSA_LINK_LENGTH	20

/* The packet MTU minus the header */
#define OSPF_MIN_LSA_SEQNUM	INT_MIN
#define OSPF_MAX_LSA_SEQNUM	INT_MAX

struct ospf_db_entry_tag
{
	struct
	{
		unsigned int free:1;
		unsigned int age:15;
		unsigned int entry:16;
	} b;
 
	//unsigned int lsa;
	unsigned int lsa;

	signed int	seqnum;

	/*
	 * These are used to determine the cause of the UPDATE
	 */
	unsigned char	cause_bgp;
	unsigned char	cause_ospf;
};

enum ospf_link_type_tag
{
	ospf_link_point_to_point = 1,
	ospf_link_transit_net,
	ospf_link_stub,
	ospf_link_virtual	
};

struct ospf_lsa_link_tag
{
	//unsigned int			dst;
	unsigned int			dst;

	//unsigned int			data;
	//unsigned short		tos;
	//ospf_link_type		type;
	unsigned int		metric;
};

enum ospf_lsa_type_tag
{
	ospf_lsa_router = 1,
	ospf_lsa_network,
	ospf_lsa_summary3,
	ospf_lsa_summary4,
	ospf_lsa_as_ext
};

struct ospf_lsa_tag
{
	ospf_lsa_type	type;

	unsigned int	id;
	//unsigned  adv_r;
	unsigned int adv_r;
	//unsigned short	length;
	unsigned int length;

	tw_memoryq	 links;
	tw_memory	*next;

	/*
	 * These are used to determine the convergence time.
	 */
#if VERIFY_OSPF_CONVERGENCE
	tw_stime	 start;
	int		 count;
#endif
};

#endif
