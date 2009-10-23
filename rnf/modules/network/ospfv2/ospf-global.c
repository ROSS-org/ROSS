#include <ospf.h>

int g_ospf_link_weight = 10;

int	db_sz = 0;

tw_fd	g_ospf_fd = 0;
tw_fd	g_ospf_lsa_fd = 0;
tw_fd	g_ospf_dd_fd = 0;
tw_fd	g_ospf_ll_fd = 0;

ospf_global_state *g_ospf_state = NULL;

int g_route[400];
int g_route1[400];
int gr1;
int gr2;

int g_ospf_rt_write = 0;

unsigned int OSPF_LSA_MAX_AGE = 3600000;
unsigned int OSPF_LSA_REFRESH_AGE = 1800000;

int g_max_updates = 0;
unsigned int g_memory = 0;

int *p;
int *front; 
int *f; 
int *s; 
int *d; 

ospf_dd_pkt *g_last_dd;
ospf_dd_pkt *g_rex_dd;

ospf_statistics *g_ospf_stats;
char		*g_ospf_init_file = NULL;
FILE		*g_ospf_routing_file = NULL;
FILE		*g_ospf_lsa_file = NULL;
FILE		*g_ospf_lsa_input = NULL;
