/*********************************************************************
		              tcp-init.c 
*********************************************************************/

#include "tcp.h"

Routing_Table * dijkstra(int start_vertex);
Routing_Table * dijkstra_dml(int start_vertex);

int               number_of_nodes;
int               *router_lookup;
int               *host_lookup;
int               *p, *front, *f, *s;
double            *d;

void tcp_init_simple(char *cfg){
  int               i,j;
  int               num_links;
  int               speed;
  int               connected_router;
  int               buffer;
  int               file_sz;
  char              type[20];
  float             delay;
  FILE              *config;
  Routing_Table    *parent;

  if((config = fopen(cfg,"r")) == 0 ){
    printf("tcp error: Unable to open config file\n");
    exit(0);
  }

  fscanf(config,"%d",&g_npe);
  if(g_npe < 1 || g_npe > 4){
    printf("tcp-init error: invalid number of processors %d\n",g_npe);
    exit(0);
  }
  
  g_nkp = g_npe;

  fscanf(config,"%d",&g_type);

  fscanf(config,"%d",&g_frequency);
  if(g_frequency > 1000000 || g_frequency < 1000){
    printf("tcp-init error: frequency is outside valid range %d\n",g_frequency);
    exit(0);
  }
  fscanf(config,"%d",&g_iss);
  if(g_iss <  0 ){
    printf("tcp-init error: iss is outside valid range %d\n",g_iss);
    exit(0);
  }
  
  fscanf(config,"%d",&g_mss);
  if(g_mss < 400 || g_mss > 2000){
    printf("tcp-init error: mss is outside valid range %d\n",g_mss);
    exit(0);
  }
  
  fscanf(config,"%d",&g_recv_wnd);
  if(g_recv_wnd < 0 || g_recv_wnd > 200){
    printf("tcp-init error: recv_wnd is outside valid range %d\n",g_recv_wnd);
    exit(0);
  }

  fscanf(config,"%d",&g_routers);
  if(g_routers < 0){
    printf("tcp-init error: invaild number of routers %d\n",g_routers);
    exit(0);
  }

  fscanf(config,"%d",&g_hosts);
  if(g_hosts < 2){
    printf("tcp-init error: not enough hosts %d\n",g_hosts);
    exit(0);
  }
  
  g_routers_links = (Router_Link **) malloc(sizeof(Router_Link *) * g_routers);
  g_routers_info = (int *) malloc(sizeof(int) * g_routers);

  g_hosts_links = (Host_Link *) malloc(sizeof(Host_Link) * g_hosts);
  g_hosts_info = (Host_Info *) malloc(sizeof(Host_Info) * g_hosts);

  for(i=0;i<g_routers;i++){
    fscanf(config,"%s", &type );
    if(strcmp("router",type) != 0){
      printf("tcp-init error: router with invalid type\n");
      exit(0);
    }

    fscanf(config,"%d", &num_links );
    if(num_links < 0 || num_links > 1025){
      printf("tcp-init error: router with invalid number of links %d\n", num_links);
      exit(0);
    }
    // printf("Router %d Has %d links\n", i, num_links);

    g_routers_info[i] = num_links;
    g_routers_links[i] = (Router_Link *) malloc(sizeof(Router_Link) * num_links);

    for(j=0;j<num_links;j++){
      fscanf(config,"%d %f %d %d", &speed, &delay, &connected_router, &buffer);
      //printf("\tConnected to Router %d with a link of speed %d and delay of %f",
      //	     connected_router, speed, delay);
      if(speed < 100 || speed > 2000000000){
	printf("tcp-init error: router with invalid link speed %d\n", speed);
	exit(0);
      }

      if(delay*g_frequency < 5 || delay > 200){
	printf("tcp-init error: router with invalid link delay %d %d\n", delay,i);
	exit(0);
      }

      if(buffer < 0){
	printf("tcp-init error: router with invalid link buffer %d\n", buffer);
	exit(0);
      }

      if(connected_router < 0){
	printf("tcp-init error: router with invalid connected router %d\n", connected_router);
	exit(0);
      }

      g_routers_links[i][j].link_speed = ((double) speed / g_frequency);
      g_routers_links[i][j].delay = delay * g_frequency;
      g_routers_links[i][j].buffer_sz = buffer;
      g_routers_links[i][j].connected = connected_router;
    }
  }

  for(i=0; i<g_hosts; i++){
    fscanf(config,"%s", &type );
    if(strcmp("host",type) != 0){
      printf("tcp-init error: host with invalid type\n");
      exit(0);
    }

    fscanf(config,"%d %f %d %s", &speed, &delay, &connected_router, &type);
    
    if(speed < 100 || speed > 2000000000){
      printf("tcp-init error: host with invalid link speed %d\n", speed);
      exit(0);
    }

    if(delay*g_frequency < 5 || delay > 200){
      printf("tcp-init error: host with invalid link delay %d\n", delay);
      exit(0);
    }

    if(connected_router < 0){
      printf("tcp-init error: host with invalid connected router %d\n", connected_router);
      exit(0);
    }

    if(strcmp("random",type) != 0 && strcmp("server",type) != 0 
       && strcmp("direct",type) != 0 
#ifdef CLIENT_DROP 
       && strcmp("client-drop",type) != 0 
#endif
       ){
      printf("tcp-init error: host with invalid type\n");
      exit(0);
    }
    
    g_hosts_links[i].connected = connected_router;
    g_hosts_links[i].link_speed = ( (double) speed / g_frequency ) / (double) 8;
    g_hosts_links[i].delay = delay * g_frequency;

    
    // printf("%d connected %d \n",i, g_hosts_links[i].connected);
    switch (type[0]) 
      {
      case 'r':
	g_hosts_info[i].type = 0;
	break;

      case 'd':
	g_hosts_info[i].type = 1;

	fscanf(config,"%d", &connected_router );
	if(connected_router < 0){
	  printf("tcp-init error: host with invalid connected router %d\n",
		 connected_router);
	  exit(0);
	}
	g_hosts_info[i].connected = connected_router;

	fscanf(config,"%d", &file_sz );
	if(file_sz < 1 || file_sz > 2000000000){
	  printf("tcp-init error: host with invalid number file size %d\n", file_sz);
	  exit(0);
	}
	g_hosts_info[i].size = pow(2,31) - 1; 
	//g_hosts_info[i].size = file_sz;

	break;
      case 's':
	g_hosts_info[i].type = 2;
	break;

#ifdef CLIENT_DROP
      case 'c':
	g_hosts_info[i].type = 3;
	
	fscanf(config,"%d", &connected_router );
	if(connected_router < 0){
	  printf("tcp-init error: host with invalid connected router %d\n", 
		 connected_router);
	  exit(0);
	}
	
	g_hosts_info[i].connected = connected_router;

	fscanf(config,"%d", &file_sz );
	if(file_sz < 1 || file_sz > 2000000000){
	  printf("tcp-init error: host with invalid number file size %d\n", file_sz);
	  exit(0);
	}
	// g_hosts_info[i].size = file_sz;
	g_hosts_info[i].size = pow(2,31) - 1; 

	fscanf(config,"%d", &num_links );
	if(num_links < 0){
	  printf("tcp-init error: host with invalid number of drops %d\n", num_links);
	  exit(0);
	}

	g_hosts_info[i].packets = (int *) malloc(sizeof(int) * num_links);
	g_hosts_info[i].drop_index = 0;
	g_hosts_info[i].max = num_links;

	for(j=0;j<num_links;j++){	  
	  fscanf(config,"%d", &g_hosts_info[i].packets[j]);
	  if(j > 0 && g_hosts_info[i].packets[j] <= g_hosts_info[i].packets[j-1])
	    {
	      printf("tcp-init error: host with invalid packet drop %d %d\n",
		     g_hosts_info[i].packets[j], g_hosts_info[i].packets[j-1]);
	      exit(0);
	    }	  
	}
	break;
#endif
      }
  }
  
  // update to all shorts paths later. switch to network number.  
  // Init routering table.
  if(g_type == -1){
    int routing_table;
    
    routing_table = open("routing-table.data",  O_RDONLY );


    if(g_hosts > 100000)
      g_num_links = 30;
    else 
      g_num_links = 10;

    g_new_routing_table = (unsigned char **) malloc(sizeof(unsigned char *) * g_routers);
      
    for(i = 0 ; i < g_routers; i++) {
      g_new_routing_table[i] = (unsigned char *) malloc(g_routers * sizeof(unsigned char));
      read(routing_table, g_new_routing_table[i], sizeof(unsigned char) * g_routers);
    }
    close(routing_table);
  }
    
  fclose(config);
}


void tcp_init(char *cfg){
  int               i,j;
  int               num_links;
  int               speed;
  int               connected_router;
  int               buffer;
  int               file_sz;
  char              type[20];
  float             delay;
  FILE              *config;
  Routing_Table    *parent;
  if((config = fopen(cfg,"r")) == 0 ){
    printf("tcp error: Unable to open config file\n");
    exit(0);
  }

  fscanf(config,"%d",&g_npe);
  if(g_npe < 1 || g_npe > 4){
    printf("tcp-init error: invalid number of processors %d\n",g_npe);
    exit(0);
  }
  
  g_nkp = g_npe;

  fscanf(config,"%d",&g_frequency);
  if(g_frequency > 1000000 || g_frequency < 1000){
    printf("tcp-init error: frequency is outside valid range %d\n",g_frequency);
    exit(0);
  }
  fscanf(config,"%d",&g_iss);
  if(g_iss <  0 ){
    printf("tcp-init error: iss is outside valid range %d\n",g_iss);
    exit(0);
  }
  
  fscanf(config,"%d",&g_mss);
  if(g_mss < 400 || g_mss > 2000){
    printf("tcp-init error: mss is outside valid range %d\n",g_mss);
    exit(0);
  }
  
  fscanf(config,"%d",&g_recv_wnd);
  if(g_recv_wnd < 0 || g_recv_wnd > 200){
    printf("tcp-init error: recv_wnd is outside valid range %d\n",g_recv_wnd);
    exit(0);
  }

  fscanf(config,"%d",&g_routers);
  if(g_routers < 0){
    printf("tcp-init error: invaild number of routers %d\n",g_routers);
    exit(0);
  }

  fscanf(config,"%d",&g_hosts);
  if(g_hosts < 2){
    printf("tcp-init error: not enough hosts %d\n",g_hosts);
    exit(0);
  }
  
  g_routers_links = (Router_Link **) malloc(sizeof(Router_Link *) * g_routers);
  g_routers_info = (int *) malloc(sizeof(int) * g_routers);

  g_hosts_links = (Host_Link *) malloc(sizeof(Host_Link) * g_hosts);
  g_hosts_info = (Host_Info *) malloc(sizeof(Host_Info) * g_hosts);

  for(i=0;i<g_routers;i++){
    fscanf(config,"%s", &type );
    if(strcmp("router",type) != 0){
      printf("tcp-init error: router with invalid type\n");
      exit(0);
    }

    fscanf(config,"%d", &num_links );
    if(num_links < 0 || num_links > 1025){
      printf("tcp-init error: router with invalid number of links %d\n", num_links);
      exit(0);
    }
    //printf("Router %d Has %d links\n", i, num_links);

    g_routers_info[i] = num_links;
    g_routers_links[i] = (Router_Link *) malloc(sizeof(Router_Link) * num_links);

    for(j=0;j<num_links;j++){
      fscanf(config,"%d %f %d %d", &speed, &delay, &buffer, &connected_router);
      //printf("\tConnected to Router %d with a link of speed %d and delay of %f",
      //	     connected_router, speed, delay);
      if(speed < 100 || speed > 2000000000){
	printf("tcp-init error: router with invalid link speed %d\n", speed);
	exit(0);
      }

      if(delay*g_frequency < 5 || delay > 200){
	printf("tcp-init error: router with invalid link delay %d\n", delay);
	exit(0);
      }

      if(buffer < 0){
	printf("tcp-init error: router with invalid link buffer %d\n", buffer);
	exit(0);
      }

      if(connected_router < 0){
	printf("tcp-init error: router with invalid connected router %d\n", connected_router);
	exit(0);
      }

      g_routers_links[i][j].link_speed = ((double) speed / g_frequency) / (double) 8;
      g_routers_links[i][j].delay = delay * g_frequency;
      g_routers_links[i][j].buffer_sz = buffer;
      g_routers_links[i][j].connected = connected_router;
    }
  }

  for(i=0; i<g_hosts; i++){
    fscanf(config,"%s", &type );
    if(strcmp("host",type) != 0){
      printf("tcp-init error: host with invalid type\n");
      exit(0);
    }

    fscanf(config,"%d %f %d %s", &speed, &delay, &connected_router, &type);
    
    if(speed < 100 || speed > 2000000000){
      printf("tcp-init error: host with invalid link speed %d\n", speed);
      exit(0);
    }

    if(delay*g_frequency < 5 || delay > 200){
      printf("tcp-init error: host with invalid link delay %d\n", delay);
      exit(0);
    }

    if(connected_router < 0){
      printf("tcp-init error: host with invalid connected router %d\n", connected_router);
      exit(0);
    }

    if(strcmp("random",type) != 0 && strcmp("server",type) != 0 
       && strcmp("direct",type) != 0 
#ifdef CLIENT_DROP 
       && strcmp("client-drop",type) != 0 
#endif
       ){
      printf("tcp-init error: host with invalid type\n");
      exit(0);
    }
    
    g_hosts_links[i].connected = connected_router;
    g_hosts_links[i].link_speed = ( (double) speed / g_frequency ) / (double) 8;
    g_hosts_links[i].delay = delay * g_frequency;

    
    // printf("%d connected %d \n",i, g_hosts_links[i].connected);
    switch (type[0]) 
      {
      case 'r':
	g_hosts_info[i].type = 0;
	break;

      case 'd':
	g_hosts_info[i].type = 1;

	fscanf(config,"%d", &connected_router );
	if(connected_router < 0){
	  printf("tcp-init error: host with invalid connected router %d\n",
		 connected_router);
	  exit(0);
	}
	
	g_hosts_info[i].connected = connected_router;

	fscanf(config,"%d", &file_sz );
	if(file_sz < 1 || file_sz > 2000000000){
	  printf("tcp-init error: host with invalid number file size %d\n", file_sz);
	  exit(0);
	}
	g_hosts_info[i].size = file_sz;
	break;
      case 's':
	g_hosts_info[i].type = 2;
	break;

#ifdef CLIENT_DROP
      case 'c':
	g_hosts_info[i].type = 3;
	
	fscanf(config,"%d", &connected_router );
	if(connected_router < 0){
	  printf("tcp-init error: host with invalid connected router %d\n", 
		 connected_router);
	  exit(0);
	}
	
	g_hosts_info[i].connected = connected_router;

	fscanf(config,"%d", &file_sz );
	if(file_sz < 1 || file_sz > 2000000000){
	  printf("tcp-init error: host with invalid number file size %d\n", file_sz);
	  exit(0);
	}
	g_hosts_info[i].size = file_sz;
	
	fscanf(config,"%d", &num_links );
	if(num_links < 0){
	  printf("tcp-init error: host with invalid number of drops %d\n", num_links);
	  exit(0);
	}

	g_hosts_info[i].packets = (int *) malloc(sizeof(int) * num_links);
	g_hosts_info[i].drop_index = 0;
	g_hosts_info[i].max = num_links;

	for(j=0;j<num_links;j++){	  
	  fscanf(config,"%d", &g_hosts_info[i].packets[j]);
	  if(j > 0 && g_hosts_info[i].packets[j] <= g_hosts_info[i].packets[j-1])
	    {
	      printf("tcp-init error: host with invalid packet drop %d %d\n",
		     g_hosts_info[i].packets[j], g_hosts_info[i].packets[j-1]);
	      exit(0);
	    }	  
	}
	break;
#endif
      }
  }
  
  // update to all shorts paths later. switch to network number.  
  // Init routering table.
  
  g_routing_table = (Routing_Table **) malloc(sizeof(Routing_Table *) * g_routers);
  
  d = calloc(g_routers + g_hosts, sizeof(double));
  front = calloc(g_routers + g_hosts, sizeof(int));
  p = calloc(g_routers + g_hosts, sizeof(int));
  f = calloc(g_routers + g_hosts, sizeof(int));
  s = calloc(g_routers + g_hosts, sizeof(int));

  for(i = 0 ; i < g_routers; i++) {
     g_routing_table[i] = dijkstra_dml(i);
     // printf("routering table at %d\n",i);
     // for(j = 0; j < g_routers + g_hosts; j++)
     //  printf("router %d link %d\n",g_routing_table[i][j].connected,
     //	       g_routing_table[i][j].link);
     // printf("\n\n");
   }
    
  fclose(config);
  free(d);
  free(front);
  free(p);
  free(f);
  free(s);
}


void tcp_init_rocketfuel_build(char *config,char *routing_file){
  int               i,j,k,l;
  int               num_links;
  int               speed;
  int               connected_router;
  int               buffer;
  int               file_sz;
  int               level;
  int               z;
  int               g_clients_2 = 0;
  char              type[20];
  float             delay;
  int               routing_table;
  Routing_Table     *parent;

  router_lookup = malloc(sizeof(int) * 25000);
  host_lookup = malloc(sizeof(int) * 25000);

  init_node_list();
  number_of_nodes = read_rocketfuel_file( config );
  color_topology(8);
  map_nodes_to_ross();
  
  if((routing_table = open(routing_file, O_CREAT|O_WRONLY| O_TRUNC )) <=  0){
    printf("Error opening new routing data file\n");
    exit(0);
  }
 
   g_hosts = 0;
  g_routers = 0;
  z = -1;
  for(i = 0; i < number_of_nodes; i++)
    {
      router_lookup[i] = g_routers;
      host_lookup[i] = g_hosts;
     
      if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0 )
	g_hosts++;
      else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
	g_routers++;
    }
  
  printf("number of hosts %d routers %d \n", g_hosts, g_routers);
  
  g_routers_links = (Router_Link **) malloc(sizeof(Router_Link *) * g_routers);
  g_routers_info = (int *) malloc(sizeof(int) * g_routers);
  
  g_hosts_links = (Host_Link *) malloc(sizeof(Host_Link) * g_hosts);
  g_hosts_info = (Host_Info *) malloc(sizeof(Host_Info) * g_hosts);
 
  g_frequency = 100000;
  g_iss = 0;
  g_mss = 960;
  g_recv_wnd = 64;
  
  k = 0;
  l = 0;
  z = -1;
  
  for(i=0; i< number_of_nodes; i++){
    if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
      {
	g_hosts_links[k].link_speed = ((double) 70000  / g_frequency) / (double) 8;
	g_hosts_links[k].delay = .1 * g_frequency;
	g_hosts_links[k].connected =  router_lookup[g_node_list[i].link_list[0].node_id];
	g_clients_2++;

	if(host_lookup[i] % 2 == 0)
	  g_hosts_info[k].type = 2;
	else
	  g_hosts_info[k].type = 0;
	k++;
      }
    else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0 )
      {
	g_routers_info[l] = g_node_list[i].num_links;
	g_routers_links[l] = (Router_Link *) malloc(sizeof(Router_Link) *  g_routers_info[l] );
	
	for(j=0;j< g_routers_info[l] ;j++){
	  if(g_node_list[g_node_list[i].link_list[j].node_id].num_links == 1)
	    {
	      g_routers_links[l][j].link_speed = ((double) 70000 / g_frequency) / (double) 8;
	      g_routers_links[l][j].delay = .005 * g_frequency;
	      g_routers_links[l][j].buffer_sz = 5000;  
	      g_clients_2++;
	    }
	  else {
	    if(g_node_list[i].level < 
	       g_node_list[g_node_list[i].link_list[j].node_id].level)
	      level =  g_node_list[g_node_list[i].link_list[j].node_id].level;
	    else
	      level = g_node_list[i].level;
	    switch (level) 
	      {
	      case 0:
		  g_routers_links[l][j].link_speed = ((double) 9920000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 12400000;
		break;
	      case 1:
		  g_routers_links[l][j].link_speed = ((double) 2480000000.0 / (double) g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 3000000;
		break;
	      case 2:
		  g_routers_links[l][j].link_speed = ((double) 620000000 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 775000;
		break;
	      case 3:
		  g_routers_links[l][j].link_speed = ((double) 155000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 200000;
		break;
	      case 4:
		  g_routers_links[l][j].link_speed = ((double) 45000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 60000;
		break;
	      case 5:
		  g_routers_links[l][j].link_speed = ((double) 1500000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 20000;
		break;
	      case 6:
		  g_routers_links[l][j].link_speed = ((double) 375000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 10000;
		break;
	      case 7:
		  g_routers_links[l][j].link_speed = ((double) 70000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 5000;
		break;
	      default:
		printf("there has been an error %d\n",level);
		exit(0);
	      }
	  }
	  if( g_node_list[g_node_list[i].link_list[j].node_id].num_links == 1)
	    g_routers_links[l][j].connected =  host_lookup[g_node_list[i].link_list[j].node_id] + g_routers;
	  else
	    g_routers_links[l][j].connected =  router_lookup[g_node_list[i].link_list[j].node_id];
	}
	l++;
      }
  }

  // update to all shorts paths later. switch to network number.  
  // Init routering table. 
  
  g_routing_table = (Routing_Table **) malloc(sizeof(Routing_Table *) * g_routers);
  d = calloc(g_routers + g_hosts, sizeof(double));
  front = calloc(g_routers + g_hosts, sizeof(int));
  p = calloc(g_routers + g_hosts, sizeof(int));
  f = calloc(g_routers + g_hosts, sizeof(int));
  s = calloc(g_routers + g_hosts, sizeof(int));

  for(i = 0 ; i < g_routers; i++) {
    g_routing_table[i] = dijkstra(i); 
    write(routing_table,g_routing_table[i],(g_routers+g_hosts)*sizeof(Routing_Table));
    //    fprintf(routing_table,"%d\n",i);
        printf("routing table %d\n", i);
    //    for(j = 0; j < g_routers + g_hosts; j++){
    //    fprintf(routing_table,"%d %d ",g_routing_table[i][j].connected, g_routing_table[i][j].link);
    //    }
    //    fprintf(routing_table,"\n");
  }
  // write(routing_table,g_routing_table,(g_routers+g_hosts)*g_routers*sizeof(Routing_Table));
  close(routing_table);
  
  free(d);
  free(front);
  free(p);
  free(f);
  free(s);
}



void tcp_init_rocketfuel_load(char *config, char *routing_file){
  int               i,j,k,l;
  int               num_links;
  int               speed;
  int               connected_router;
  int               buffer;
  int               file_sz;
  int               level;
  int               z;
  int               g_clients_2 = 0;
  char              type[20];
  float             delay;
  //FILE              *routing_table;
  int               routing_table;
  Routing_Table    *parent;

  router_lookup = malloc(sizeof(int) * 25000);
  host_lookup = malloc(sizeof(int) * 25000);

  init_node_list();
  number_of_nodes = read_rocketfuel_file( config );
  color_topology(8);
  map_nodes_to_ross();

  /*
    if((routing_table = fopen(routing_file, "r")) == NULL ){
    printf("Error opening new routing data file\n");
    exit(0);
    }
  */
  if((routing_table = open(routing_file, O_RDONLY)) <=  0){
    printf("Error opening new routing data file\n");
    exit(0);
    }

  g_hosts = 0;
  g_routers = 0;
  z = -1;
  for(i = 0; i < number_of_nodes; i++)
    {
      router_lookup[i] = g_routers;
      host_lookup[i] = g_hosts;
     
      if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0 )
	g_hosts++;
      else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
	g_routers++;
    }
  
  printf("number of hosts %d routers %d \n", g_hosts, g_routers);

  g_routers_links = (Router_Link **) malloc(sizeof(Router_Link *) * g_routers);
  g_routers_info = (int *) malloc(sizeof(int) * g_routers);
  
  g_hosts_links = (Host_Link *) malloc(sizeof(Host_Link) * g_hosts);
  g_hosts_info = (Host_Info *) malloc(sizeof(Host_Info) * g_hosts);
  
  g_frequency = 100000;
  g_iss = 0;
  g_mss = 960;
  g_recv_wnd = 64;
  
  k = 0;
  l = 0;
  z = -1;
  
  for(i=0; i< number_of_nodes; i++){
    if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
      {
	g_hosts_links[k].link_speed = ((double) 70000  / g_frequency) / (double) 8;
	g_hosts_links[k].delay = .005 * g_frequency;
	g_hosts_links[k].connected =  router_lookup[g_node_list[i].link_list[0].node_id];
	g_clients_2++;

	if(host_lookup[i] % 2 == 0)
	  g_hosts_info[k].type = 2;
	else
	  g_hosts_info[k].type = 0;
	k++;
      }
    else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0 )
      {
	g_routers_info[l] = g_node_list[i].num_links;
	g_routers_links[l] = (Router_Link *) malloc(sizeof(Router_Link) *  g_routers_info[l] );
	
	for(j=0;j< g_routers_info[l] ;j++){
	  if(g_node_list[g_node_list[i].link_list[j].node_id].num_links == 1)
	    {
	      g_routers_links[l][j].link_speed = ((double) 70000 / g_frequency) / (double) 8;
	      g_routers_links[l][j].delay = .005 * g_frequency;
	      g_routers_links[l][j].buffer_sz = 5000;  
	      g_clients_2++;
	    }
	  else {
	    if(g_node_list[i].level < 
	       g_node_list[g_node_list[i].link_list[j].node_id].level)
	      level =  g_node_list[g_node_list[i].link_list[j].node_id].level;
	    else
	      level = g_node_list[i].level;
	    switch (level) 
	      {
	      case 0:
		  g_routers_links[l][j].link_speed = ((double) 9920000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 12400000;
		break;
	      case 1:
		  g_routers_links[l][j].link_speed = ((double) 2480000000.0 / (double) g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 3000000;
		break;
	      case 2:
		  g_routers_links[l][j].link_speed = ((double) 620000000 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 775000;
		break;
	      case 3:
		  g_routers_links[l][j].link_speed = ((double) 155000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 200000;
		break;
	      case 4:
		  g_routers_links[l][j].link_speed = ((double) 45000000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 60000;
		break;
	      case 5:
		  g_routers_links[l][j].link_speed = ((double) 1500000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 20000;
		break;
	      case 6:
		  g_routers_links[l][j].link_speed = ((double) 375000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 10000;
		break;
	      case 7:
		  g_routers_links[l][j].link_speed = ((double) 70000.0 / g_frequency) / (double) 8;
		  g_routers_links[l][j].delay = .005 * g_frequency;
		  g_routers_links[l][j].buffer_sz = 5000;
		break;
	      default:
		printf("there has been an error %d\n",level);
		exit(0);
	      }
	  }
	  if( g_node_list[g_node_list[i].link_list[j].node_id].num_links == 1)
	    g_routers_links[l][j].connected =  host_lookup[g_node_list[i].link_list[j].node_id] + g_routers;
	  else
	    g_routers_links[l][j].connected =  router_lookup[g_node_list[i].link_list[j].node_id];
	}
	l++;
      }
  }

  // update to all shorts paths later. switch to network number.  
  // Init routering table. 
  
  //g_routing_table = (Routing_Table **) malloc(sizeof(Routing_Table *) * g_routers);
  //for(i = 0 ; i < g_routers; i++) {
  //  g_routing_table[i] = calloc(g_routers + g_hosts, sizeof(Routing_Table));
  //  fscanf(routing_table,"%d",&l);

  //  for(j = 0; j < g_routers + g_hosts; j++){
  //   fscanf(routing_table,"%d %d",&l,&k);
  //   g_routing_table[i][j].connected = l;
  //   g_routing_table[i][j].link = k;
  // }
  // }
  g_routing_table = (Routing_Table **) malloc(g_routers*sizeof(Routing_Table *));
  
  for(j = 0; j < g_routers; j++){
     g_routing_table[j] = (Routing_Table *) malloc((g_routers+g_hosts) * sizeof(Routing_Table));
     read(routing_table, g_routing_table[j], (g_routers+g_hosts)*sizeof(Routing_Table));
     
     /*fscanf(routing_table,"%d",&l);
       for(i = 0; i < g_routers + g_hosts; i++){
       
       //}
       fscanf(routing_table,"%d %d",&l,&k);
       g_routing_table[j][i].connected = l;
       g_routing_table[j][i].link = k;
       // printf("router %d link %d\n",g_routing_table[i][j].connected,g_routing_table[i][j].link);
       }
     */
     
   }
  
  close(routing_table);
}

void 
tcp_init_mapped(){
  int g_hosts;
  int g_routers;
  int z,i,j,k;

  g_hosts = 0;
  g_routers = 0;
  z = -1;
  
  for (i = 0; i < number_of_nodes; i++)
    {
      router_lookup[i] = g_routers;
      host_lookup[i] = g_hosts;
      
      if (g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
	g_hosts++;
      else if (g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
	g_routers++;
    }
  
  router_lookup = malloc(sizeof(int) * 25000);
  host_lookup = malloc(sizeof(int) * 25000);
  
  init_node_list();
  number_of_nodes = read_rocketfuel_file("7018.cch");
  color_topology(8);
  map_nodes_to_ross();

  for (i = 0; i < number_of_nodes; i++)
    {
      if (g_node_list[i].num_links == 1 && g_node_list[i].level >= 0 && k < g_hosts)
	{
	} else if (g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
	  {
		  int count =0 ;
		  for (j = 0; j < g_node_list[i].num_links; j++)
		    {
		      int q,found = 0;
		      for(q = 0 ; q< g_node_list[g_node_list[i].link_list[j].node_id].num_links; q++)
			      if(g_node_list[g_node_list[i].link_list[j].node_id].link_list[q].node_id == i)
				found = 1;
		      if(!found)
			count++;
		    }
		  if(g_node_list[i].num_links - count < 2){
		    g_node_list[i].num_links = 0;
		    printf("Router to remove %d\n\n",router_lookup[i]);
		  }
	  }
    }

  for (i = 0; i < number_of_nodes; i++)
    {
      if (g_node_list[i].num_links == 1 && g_node_list[i].level >= 0 && k < g_hosts)
	{
	} else if (g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
	  {
	    int count =0 ;
	    for (j = 0; j < g_node_list[i].num_links; j++)
	      {
		int q,found = 0;
		for(q = 0 ; q< g_node_list[g_node_list[i].link_list[j].node_id].num_links; q++)
		  if(g_node_list[g_node_list[i].link_list[j].node_id].link_list[q].node_id == i)
		    found = 1;
		if(!found){
		  //printf("%d to %d\n",router_lookup[g_node_list[i].link_list[j].node_id],router_lookup[i]);
			      count++;
		}
	      }
	    if(g_node_list[i].num_links - count < 2){
	      g_node_list[i].num_links = 0;
	      printf("Router to remove %d\n\n",router_lookup[i]);
	      exit(0);
	    }
	  }
    }
  
	
  g_routers = 0;
  g_hosts = 0;
  for (i = 0; i < number_of_nodes; i++)
    {
      router_lookup[i] = g_routers;
      host_lookup[i] = g_hosts;
      
      if (g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
	g_hosts++;
      else if (g_node_list[i].num_links > 1 && g_node_list[i].level >= 0)
			g_routers++;
    }
}
    
void
tcp_init_lps(char *type){
  int i,j,k,l,m,n,o,p;
  tw_pe *pe;
  tw_kp *kp;
  tw_lp *lp;
  unsigned long lps_on_pe[4] = {0,0,0,0};
  
  if (strcmp(type,"dml") == 0){

    for(i = 0; i < g_npe; i++) {
      pe = tw_getpe(i);
      kp = tw_getkp(i);
      tw_kp_onpe(kp,pe);
      for(j=0;j<g_routers/g_npe;j++) 
	{ 
	  lp = tw_getlp( j + ((g_routers / g_npe) * i));
	  tw_lp_settype(lp, TW_TCP_ROUTER);
	  tw_lp_onkp(lp, kp);
	  tw_lp_onpe(lp, pe); 
	  lps_on_pe[pe->id]++;
	}
    }
    for(i = 0; i < g_routers % g_npe; i++)
      {
	pe = tw_getpe(i);
	kp = tw_getkp(i);
	lp = tw_getlp(i + ((g_routers / g_npe) * g_npe));
	tw_lp_settype(lp, TW_TCP_ROUTER);
	tw_lp_onkp(lp, kp);
	tw_lp_onpe(lp, pe); 
	lps_on_pe[pe->id]++;
      } 
    for(i = 0; i < g_npe; i++) {
      pe = tw_getpe(i);
      kp = tw_getkp(i);
      for(j=0;j<g_hosts/g_npe;j++)
	{
	  lp = tw_getlp((j + ((g_hosts / g_npe) * i)) + g_routers );
	  tw_lp_settype(lp, TW_TCP_HOST);
	  tw_lp_onkp(lp, kp);
	  tw_lp_onpe(lp, pe); 
	  lps_on_pe[pe->id]++;
	}
    }
    for(i=0 ; i < g_hosts% g_npe; i++)
      {
	pe = tw_getpe(i);
	kp = tw_getkp(i);
	lp = tw_getlp((i + ((g_hosts / g_npe) * g_npe)) + g_routers );
	tw_lp_settype(lp, TW_TCP_HOST);
	tw_lp_onkp(lp, kp);
	tw_lp_onpe(lp, pe); 
	lps_on_pe[pe->id]++;
      }
  }
  else if(strcmp(type,"simple") == 0  && g_type > 0 ) {
    for(i = 0; i < g_npe ; i++) {
       pe = tw_getpe(i);
       kp = tw_getkp(i);
       tw_kp_onpe(kp,pe);
    }

    j = g_type;
    k = g_type + g_type * g_type;
    n = g_routers;
    l = 0;
    m = 0;
    for(i = 0; i < g_type ; i ++) {
      // printf("%d\n",i);
      pe = tw_getpe(i%g_npe);
      kp = tw_getkp(i%g_npe);
      lp = tw_getlp(i);
      tw_lp_settype(lp, TW_TCP_ROUTER);
      tw_lp_onkp(lp, kp);
      tw_lp_onpe(lp, pe);
      lps_on_pe[pe->id]++;

      for(j = j; j < g_type + (i + 1) * g_type; j++){
	//printf("\t%d\n",j);
	l++;
	lp = tw_getlp(j);
	tw_lp_settype(lp, TW_TCP_ROUTER);
	tw_lp_onkp(lp, kp);
	tw_lp_onpe(lp, pe); 
        lps_on_pe[pe->id]++;

	for(k = k; k < g_type + pow(g_type, 2) + l * g_type; k++){
	  //printf("\t\t%d\n\t",k);
	  m++;
	  lp = tw_getlp(k);
	  tw_lp_settype(lp, TW_TCP_ROUTER);
	  tw_lp_onkp(lp, kp);
	  tw_lp_onpe(lp, pe);
          lps_on_pe[pe->id]++;

	  for(n = n; n < g_routers + m * g_type; n++){
	    //printf("%d ",n);
	    lp = tw_getlp(n);
	    tw_lp_settype(lp, TW_TCP_HOST);
	    tw_lp_onkp(lp, kp);
	    tw_lp_onpe(lp, pe); 
            lps_on_pe[pe->id]++;
	  }
	  //printf("\n");
	}
      }
    }
  } 
  else if(strcmp(type,"simple") == 0  && g_type == -1) {
    for(i = 0; i < g_npe ; i++) {
       pe = tw_getpe(i);
       kp = tw_getkp(i);
       tw_kp_onpe(kp,pe);
    }

    j = g_type;
    k = g_type + g_type * g_type;
    n = g_routers;
    l = 0;
    m = 0;
    for(i = 0; i < g_routers ; i ++) {
      // printf("%d\n",i);
      pe = tw_getpe(i%g_npe);
      kp = tw_getkp(i%g_npe);
      lp = tw_getlp(i);
      tw_lp_settype(lp, TW_TCP_ROUTER);
      tw_lp_onkp(lp, kp);
      tw_lp_onpe(lp, pe);
      lps_on_pe[pe->id]++;
    }
    for(i = g_routers ; i < g_routers + g_hosts; i++) {
      pe = tw_getpe(i%g_npe);
      kp = tw_getkp(i%g_npe);
      lp = tw_getlp(i);
      tw_lp_settype(lp, TW_TCP_HOST);
      tw_lp_onkp(lp, kp);
      tw_lp_onpe(lp, pe);
      lps_on_pe[pe->id]++;
    }
  }
  else if(strcmp(type,"mapped") == 0 ) {
    for(i=0; i< number_of_nodes; i++){
      if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
	{
	  pe = tw_getpe( g_node_list[i].pe);
	  kp = tw_getkp( g_node_list[i].kp);
	  tw_kp_onpe(kp,pe);
	  lp = tw_getlp(CORE_ROUTERS+host_lookup[i]);
	  tw_lp_settype(lp, TW_TCP_ROUTER);
	  tw_lp_onkp(lp, kp);
	  tw_lp_onpe(lp, pe); 
          lps_on_pe[pe->id]++;
	}
      else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0 ){
	pe = tw_getpe( g_node_list[i].pe);
	kp = tw_getkp( g_node_list[i].kp);
	tw_kp_onpe(kp,pe);
	lp = tw_getlp(router_lookup[i]);
	tw_lp_settype(lp, TW_TCP_ROUTER);
	tw_lp_onkp(lp, kp);
	tw_lp_onpe(lp, pe); 
        lps_on_pe[pe->id]++;
      }
    }
 
    for(i = g_routers ; i < g_routers + g_hosts; i++) {
      pe = tw_getpe(i%g_npe);
      kp = tw_getkp(i%g_npe);
      lp = tw_getlp(i);
      tw_lp_settype(lp, TW_TCP_HOST);
      tw_lp_onkp(lp, kp);
      tw_lp_onpe(lp, pe);
      lps_on_pe[pe->id]++;
    }
         
    free(g_node_list); 
    free(host_lookup);
    free(router_lookup);
  }
  else{
    for(i=0; i< number_of_nodes; i++){
      if( g_node_list[i].num_links == 1 && g_node_list[i].level >= 0)
	{
	  pe = tw_getpe( g_node_list[i].pe);
	  kp = tw_getkp( g_node_list[i].kp);
	  tw_kp_onpe(kp,pe);
	  lp = tw_getlp(g_routers+host_lookup[i]);
	  tw_lp_settype(lp, TW_TCP_HOST);
	  tw_lp_onkp(lp, kp);
	  tw_lp_onpe(lp, pe); 
          lps_on_pe[pe->id]++;
	}
      else if( g_node_list[i].num_links > 1 && g_node_list[i].level >= 0 ){
	pe = tw_getpe( g_node_list[i].pe);
	kp = tw_getkp( g_node_list[i].kp);
	tw_kp_onpe(kp,pe);
	lp = tw_getlp(router_lookup[i]);
	tw_lp_settype(lp, TW_TCP_ROUTER);
	tw_lp_onkp(lp, kp);
	tw_lp_onpe(lp, pe); 
        lps_on_pe[pe->id]++;
      }
    }
        
    free(g_node_list); 
    free(host_lookup);
    free(router_lookup);
  }

  for( i = 0; i < 4; i++ )
     {
        printf("PE %d: Has %d LPs \n", i, lps_on_pe[i] );
     }
}


Routing_Table *
dijkstra(int start_vertex)
{
    /* Index variables. */
    int i, j, q, v, w;

    /* d[] - distance to vertices.
     * front[] - stores the index to vertices that are in the frontier.
     * p[] - position of vertices in front[] array.
     * f[] - boolean frontier set array.
     * s[] - boolean solution set array.
     * 
     * timing - timing results structure for returning the time taken.
     * dist - used in distance computations.
     * vertices, n - Graph details: vertices array, size.
     * out_n - Current vertex's OUT set size.
     */
    Routing_Table *parent;
    double dist;
    size_t int_size;
    
    parent = calloc(g_routers + g_hosts, sizeof(Routing_Table));
    parent[start_vertex].connected = start_vertex;
    parent[start_vertex].link = -1;
   
    memset(s, 0, (g_hosts+g_routers)*sizeof(int));
    memset(f, 0, (g_hosts+g_routers) *sizeof(int));

    /* Start of Dijkstra's algorithm. */


    /* The start vertex is part of the solutions set. */
    s[start_vertex] = 1;
    d[start_vertex] = 0;

    /* j is used as the size of the frontier set. */
    j = 0;

    /* Put out set of the starting vertex into the frontier and update the
     * distances to vertices in the out set.  k is the index for the out set.
     */
    
    for ( i = 0 ; i < g_routers_info[start_vertex]; i++){
      w = g_routers_links[start_vertex][i].connected;
      d[w] =  (g_node_list[g_node_list[start_vertex].link_list[i].node_id].level + 1) * 4.0;
      front[j] = w;
      p[w] = j;
      f[w] = 1;
      j++;
      parent[w].connected = g_routers_links[start_vertex][i].connected; // might not work
      parent[w].link = i;
    }


    /* At this point we are assuming that all vertices are reachable from
     * the starting vertex and N > 1 so that j > 0.
     */

    while(j > 0) {

        /* Find the vertex in frontier that has minimum distance. */
        v = front[0];
        for(q = 1; q < j; q++) {
            if(d[front[q]] < d[v]) v = front[q];
        }

        /* Move this vertex from the frontier to the solution set.  It's old
         * position in the frontier array becomes occupied by the element
         * previously at the end of the frontier.
         */
        j--;
        front[p[v]] = front[j];
        p[front[j]] = p[v];
        s[v] = 1;
        f[v] = 0;

        /* Values in p[v] and front[j] no longer used, so are now meaningless
         * at this point.
         */

        /* Update distances to vertices, w, in the out set of v.
         */
	if(v < g_routers) {
	  for ( q = 0 ; q < g_routers_info[v] ; q++){
	    w = g_routers_links[v][q].connected;
	    // all w should be q

	    /* Only update if w is not already in the solution set.
             */ 
            if(s[w] == 0) {
	      
                /* If w is in the frontier the new distance to w is the minimum
                 * of its current distance and the distance to w via v.
                 */
                dist = d[v] + (g_node_list[g_node_list[v].link_list[q].node_id].level + 1) * 4.0;
                if(f[w] == 1) {
		  if(dist < d[w]){ 
		    d[w] = dist;  
		    parent[w].connected = parent[v].connected;
		    parent[w].link = parent[v].link;
		  }           
		}     
                else {
		  parent[w].connected = parent[v].connected;
		  parent[w].link = parent[v].link;
		  d[w] = dist;
		  front[j] = w;
		  p[w] = j;
		  f[w] = 1;
		  j++;
                }
            } /* if */
	  
	  } /* while */
 
	} /* while */
    }
    /* End of Dijkstra's algorithm. */
       
    return parent;
}


Routing_Table *
dijkstra_dml(int start_vertex)
{
    /* Index variables. */
    int i, j, q, v, w;

    /* d[] - distance to vertices.
     * front[] - stores the index to vertices that are in the frontier.
     * p[] - position of vertices in front[] array.
     * f[] - boolean frontier set array.
     * s[] - boolean solution set array.
     * 
     * timing - timing results structure for returning the time taken.
     * dist - used in distance computations.
     * vertices, n - Graph details: vertices array, size.
     * out_n - Current vertex's OUT set size.
     */
    Routing_Table *parent;
    double  dist;
    size_t int_size;
    
    
    parent = calloc(g_routers + g_hosts, sizeof(Routing_Table));
    parent[start_vertex].connected = start_vertex;
    parent[start_vertex].link = -1;
    
    /* Start of Dijkstra's algorithm. */

    memset(s, 0, (g_hosts+g_routers)*sizeof(int));
    memset(f, 0, (g_hosts+g_routers)*sizeof(int));

    /* The start vertex is part of the solutions set. */
    s[start_vertex] = 1;
    d[start_vertex] = 0;

    /* j is used as the size of the frontier set. */
    j = 0;

    /* Put out set of the starting vertex into the frontier and update the
     * distances to vertices in the out set.  k is the index for the out set.
     */
    
    for ( i = 0 ; i < g_routers_info[start_vertex]; i++){
      w = g_routers_links[start_vertex][i].connected;
      d[w] =  1 ;
      front[j] = w;
      p[w] = j;
      f[w] = 1;
      j++;
      parent[w].connected = g_routers_links[start_vertex][i].connected; // might not work
      parent[w].link = i;
    }


    /* At this point we are assuming that all vertices are reachable from
     * the starting vertex and N > 1 so that j > 0.
     */

    while(j > 0) {

        /* Find the vertex in frontier that has minimum distance. */
        v = front[0];
        for(q = 1; q < j; q++) {
            if(d[front[q]] < d[v]) v = front[q];
        }

        /* Move this vertex from the frontier to the solution set.  It's old
         * position in the frontier array becomes occupied by the element
         * previously at the end of the frontier.
         */
        j--;
        front[p[v]] = front[j];
        p[front[j]] = p[v];
        s[v] = 1;
        f[v] = 0;

        /* Values in p[v] and front[j] no longer used, so are now meaningless
         * at this point.
         */

        /* Update distances to vertices, w, in the out set of v.
         */
	if(v < g_routers) {
	  for ( q = 0 ; q < g_routers_info[v] ; q++){
	    w = g_routers_links[v][q].connected;
	    // all w should be q

	    /* Only update if w is not already in the solution set.
             */ 
            if(s[w] == 0) {
	      
                /* If w is in the frontier the new distance to w is the minimum
                 * of its current distance and the distance to w via v.
                 */
                dist = d[v] + 1;
                if(f[w] == 1) {
		  if(dist < d[w]){ 
		    d[w] = dist;  
		    parent[w].connected = parent[v].connected;
		    parent[w].link = parent[v].link;
		  }           
		}     
                else {
		  parent[w].connected = parent[v].connected;
		  parent[w].link = parent[v].link;
		  d[w] = dist;
		  front[j] = w;
		  p[w] = j;
		  f[w] = 1;
		  j++;
                }
            } /* if */
	  
	  } /* while */
 
	} /* while */
    }
    /* End of Dijkstra's algorithm. */
    
    return parent;
}
