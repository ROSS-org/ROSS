#include <rossnet.h>

/*
 * Environment related variables
 */
tw_lpid		g_rn_env_nlps = 0;

int	g_rn_link_prob = 1;
int	g_rn_converge_ospf = 0;
int	g_rn_converge_bgp = 0;

int	g_rn_src = -1;
int	g_rn_dst = -1;

/**
 * rossnet global data items
 *
 * g_rn_stats	-- rossnet statistics struct
 */
char			 g_rn_run[1024] = "undefined";
unsigned char		 g_rn_ttl = 32;
unsigned int	         g_rn_msg_sz;

rn_statistics		*g_rn_stats = NULL;

/**
 * These are the data type lists that make up a topology.
 *
 * g_rn_nas	-- number of ASes in the topology
 * g_rn_as	-- list of ASes in the top
 *
 * g_rn_nareas	-- number of areas in the topology
 * g_rn_areas	-- list of Areas in the topology
 *
 * g_rn_nsubnets -- number of subnets in the topology
 * g_rn_subnets	 -- list of Subnets in the topology
 *
 * g_rn_nmachines -- number of machines in the topology
 * g_rn_machines  -- list of machines in the topology
 *
 * g_rn_nextlp	  -- the next layer LP to allocate
 */
unsigned int		 g_rn_nas = 0;
rn_as			*g_rn_as = NULL;

unsigned int		 g_rn_nareas = 0;
rn_area			*g_rn_areas = NULL;

unsigned int		 g_rn_nsubnets = 0;
rn_subnet		*g_rn_subnets = NULL;

unsigned int		 g_rn_nrouters = 0;

unsigned int 		 g_rn_nlinks = 0;
unsigned int 		 g_rn_nlink_status = 0;

unsigned int 		 g_rn_nmachines = 0;
rn_machine		*g_rn_machines = NULL;

/**
 * g_rn_lptypes	-- list of module lp types
 */
rn_lptype		*g_rn_lptypes = NULL;

/**
 * These are the files that hold the topology and the model configurations.
 *
 * g_rn_xml_topology	-- the node topology descriptor
 * g_rn_link_topology	-- the link topology descriptor
 * g_rn_xml_model	-- the model descriptor
 */
char	 g_rn_xml_topology[1024];
char	 g_rn_xml_link_topology[1024];
char	 g_rn_xml_model[1024];
char	 g_rn_rt_table[1024];
char	 g_rn_tools_dir[1024] = "tcp-ip-square";
char	 g_rn_logs_dir[1024] = "logs";

xmlDocPtr document_network = NULL;
xmlDocPtr document_links = NULL;
xmlDocPtr document_model = NULL;

xmlXPathContextPtr ctxt = NULL;
xmlXPathContextPtr ctxt_links = NULL;
xmlXPathContextPtr ctxt_model = NULL;

xmlNodePtr g_rn_environment = NULL;

/**
 * g_rn_barrier	-- barrier used during lp initialization, gives RN a chance to 
 *		   init sequential, in multi-threaded enviroment of ROSS
 * g_rn_lp_lck	-- lock used in rn_lps, during tw_init_lps, to init local lps
 */
tw_barrier	g_rn_barrier;
tw_mutex	g_rn_layer_lck;
tw_mutex	g_rn_stream_lck;
