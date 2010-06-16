#ifndef INC_ospf_data_structure_h
#define INC_ospf_data_structure_h

/*
 * Global data structure crap, to be removed.
 */
typedef enum ConnectionType ConnectionType;
enum ConnectionType
{
	ospf_c_router = 1
};

typedef struct toConnections toConnections;
struct toConnections
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

typedef struct ospf_graph ospf_graph;
struct ospf_graph
{
	unsigned int routerID;
	unsigned nconnects;
	toConnections *c_ptr;

	ospf_lsa	*lsa;
};

#endif
