#ifndef INC_ospf_data_structure_h
#define INC_ospf_data_structure_h

/*
 * Global data structure crap, to be removed.
 */
FWD(enum,ConnectionType);
DEF(enum,ConnectionType)
{
	ospf_c_router = 1
};

FWD(struct, toConnections);
DEF(struct, toConnections)
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

FWD(struct, ospf_graph);
DEF(struct, ospf_graph)
{
	unsigned int routerID;
	unsigned nconnects;
	toConnections *c_ptr;

	ospf_lsa	*lsa;
};

#endif
