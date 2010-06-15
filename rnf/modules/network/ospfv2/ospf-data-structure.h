#ifndef INC_ospf_data_structure_h
#define INC_ospf_data_structure_h

/*
 * Global data structure crap, to be removed.
 */
enum ConnectionType_tag;
typedef enum ConnectionType_tag ConnectionType;
enum ConnectionType_tag
{
	ospf_c_router = 1
};

struct toConnections_tag;
typedef struct toConnections_tag toConnections;
struct toConnections_tag
{
	ConnectionType connectionType; /* Whether it is stub network or a router, if router, 
                                         what type of router */
	unsigned int myIpAddress;
	unsigned int myIpMask;
	unsigned int bandWidth;
	float delay;
	unsigned int ipAdress;
	unsigned int ipMask;
	unsigned int routerID;
	unsigned cost;
	unsigned char routerAreaID;
};

struct ospf_graph_tag;
typedef struct ospf_graph_tag ospf_graph;
struct ospf_graph_tag
{
	unsigned int routerID;
	unsigned nconnects;
	toConnections *c_ptr;

	ospf_lsa	*lsa;
};

#endif
