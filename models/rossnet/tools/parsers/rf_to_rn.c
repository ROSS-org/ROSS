/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdarg.h>

/***************************************************************************/
/* Defines *****************************************************************/
/***************************************************************************/

#define g_hosts 0

#define MAX_NODES 32000
#define MAX_LINKS_PER_NODE 512

#define MAX_LINE_WIDTH 2048
#define MAX_BUFFER_SIZE  80

#define MAX_LEVELS 16

/***************************************************************************/
/* Type Decs ***************************************************************/
/***************************************************************************/

struct rocket_fuel_link
{
	int             node_id;
	int             bandwidth;
        double          delay;
};

struct rocket_fuel_node
{
	int		id;
        int             status;
	int             used;
	int             is_bb;
	int             level;
	int             num_in_level;
	int             num_links;
	int             host_links;
	struct rocket_fuel_link link_list[MAX_LINKS_PER_NODE];
};

/* measured in Mb/sec */
int             bandwidth_classes[MAX_LEVELS] =
  { 155000000, 155000000, 45000000 , 45000000 , 
    45000000, 1500000, 1500000, 1500000, 
    1500000, 500000, 500000, 500000, 
    500000, 100000, 100000, 100000};

//	{ 9920, 2480, 620, 155, 45, 45, 45, 45 };

float              delay_classes[MAX_LEVELS] = 
  { .0015, .0015, .0015, .0015, .0015, .0015, .0015, .0015, 
    .0015, .0015, .0015, .0015, .0015, .0015, .0015, .0015 };

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

void            print_node_list();
void            parse_rocketfuel_line(char *buffer);
void            color_topology(int levels);
char           *get_rocketfuel_item(char *item, char *buffer);

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

struct rocket_fuel_node *g_node_list;
struct rocket_fuel_node *g_host_list;

unsigned        g_node_count = 0;
unsigned        g_host_count = 0;

FILE           *g_rocketfuel_file = NULL;
FILE           *xml = NULL;
unsigned        g_max_links = 0;
signed          g_max_router = -1;
int             g_level_count[MAX_LEVELS];

int             g_cost = 10;

int	        n_nodes;
int             skip;

unsigned int g_total_nodes;
unsigned int g_total_routers;
unsigned int g_total_hosts;
unsigned int g_total_core;
unsigned int g_total_hosts;

/***************************************************************************/
/* Function Bodys **********************************************************/
/***************************************************************************/

/* Start: partition_topology ***********************************************/

void
color_topology(int levels)
{
	int             i, j, k;
	int             max_levels=0;
	int             pq[g_node_count];
	int             index = 0;
	int             current;
	int             pq_sz;
	int             start_bb = 0;


	/*
	 * first pass establish level 0, super core 
	 */

	g_level_count[0] = 0;
	for (i = 0; i < g_node_count; i++)
	{
	        g_node_list[i].status = -1;
		
                if (g_node_list[i].is_bb)
		{
		        start_bb = i;
			g_node_list[i].level = 0;
			g_level_count[0]++;

			for(j = 0 ; j < g_node_list[i].num_links ; j++) 
			{
			      g_node_list[i].link_list[j].bandwidth = bandwidth_classes[0];
			      g_node_list[i].link_list[j].delay = delay_classes[0];

			}
		}
		else 
		  {
		    g_node_list[i].level = -1;
		    g_node_list[i].used = 0;
		  }
	}

	for (i = 1; i < levels; i++)
	{
		g_level_count[i] = 0;

		for (j = 0; j < g_node_count; j++)
		{
			if (g_node_list[j].level < 0)
			{
				for (k = 0; k < g_node_list[j].num_links; k++)
				{
					if (g_node_list[g_node_list[j].link_list[k].node_id].
						level == i - 1)
					{
					        g_node_list[j].used = 1;
					        g_node_list[j].level = i;
						g_node_list[j].num_in_level = g_level_count[i];
						g_level_count[i]++;

						break;
					}
				}
			}
		}
		if (g_level_count[i] == 0)
		{
			printf("MAX levels is %d \n", i);
			max_levels = i;
			break;
		}
	}
	
	if (max_levels == levels - 1)
	{
		printf
			("WARNING: max levels of topology may not have been reached!! \n");
		printf
			("         re-run again with higher level input to color function\n");
		exit(-1);
	}

	for (j = 0; j < g_node_count; j++)
	{
	       i = g_node_list[j].level;  
	       
	       for (k = 0; k < g_node_list[j].num_links; k++)
	       {
		     if (g_node_list[g_node_list[j].link_list[k].node_id].
			 level < i )
		     {
			      g_node_list[j].link_list[k].bandwidth = bandwidth_classes[g_node_list[g_node_list[j].link_list[k].node_id].level];
			      g_node_list[j].link_list[k].delay = delay_classes[g_node_list[g_node_list[j].link_list[k].node_id].level];
		     }
		     else 
		     {
			       g_node_list[j].link_list[k].bandwidth = bandwidth_classes[i];
			       g_node_list[j].link_list[k].delay = delay_classes[i];
		     }
	       }
	}
	


        current = start_bb;
	index = 0;
	pq_sz = 1;
	
	while(pq_sz) 
	  {

	    g_node_list[current].status = 2;
	   
	    for(i=0; i < g_node_list[current].num_links ; i++) 
	      {
		  if(g_node_list[g_node_list[current].link_list[i].node_id].status == -1) 
		  {
		       pq[index + pq_sz] = g_node_list[current].link_list[i].node_id;
		       pq_sz++;
		       g_node_list[g_node_list[current].link_list[i].node_id].status = 1; 
		  }
	      }
	    index++;
	    current = pq[index];
	    pq_sz--;
	    
	  }
	
	printf("Count of node connect to backbone %d\n", index); 

	for (i = 0; i < levels; i++)
		printf("Level %d has %d Nodes \n", i, g_level_count[i]);

	printf("Nodes not reached from BB include...\n");
	printf("   note: nodes with No links have only external AS links\n");
	printf("         which are pruned \n");

}

void
print_node_list()
{
	int             i, j;

	for (i = 0; i < g_node_count; i++)
	{
		if (g_node_list[i].used)
		{
			printf("Node %d is level %d has the following links \n", i,
				   g_node_list[i].level);

			for (j = 0; j < g_node_list[i].num_links; j++)
				printf("   Link %d: %d \n", j,
					   g_node_list[i].link_list[j].node_id);

			if (g_node_list[i].num_links > g_max_links)
				g_max_links = g_node_list[i].num_links;
		}
	}

	printf("max links is %d\n", g_max_links);
}

void
read_rocketfuel_file(char **argv)
{
	char            buffer[MAX_LINE_WIDTH];
	n_nodes = 0;

	if (NULL == (g_rocketfuel_file = fopen(argv[2], "r")))
	{
		printf("Failed to open input file %s for reason: %s \n",
			   argv[2], strerror(errno));
		exit(1);
	}

	if (NULL == (xml = fopen(argv[1], "w")))
	{
		printf("Failed to open output file %s for reason: %s \n",
			   argv[1], strerror(errno));
		exit(1);
	}

	g_node_list =
		(struct rocket_fuel_node *)calloc(sizeof(struct rocket_fuel_node),

										  MAX_NODES);

	while (NULL != fgets(buffer, MAX_LINE_WIDTH, g_rocketfuel_file))
	{
		parse_rocketfuel_line(buffer);

		if (++n_nodes == MAX_NODES)
		{
			printf("Error: too many network nodes!\n");
			exit(1);
		}
	}

	printf("File %s completely read, found %d nodes. \n", argv[2], n_nodes);

	fclose(g_rocketfuel_file);
}

void
parse_rocketfuel_line(char *buffer)
{
	int             i, j;
	char            item[MAX_LINE_WIDTH];
	char           *position = buffer;
	int             node = 0;

	/*
	 * factor out lines that are comments 
	 */
	if (buffer[0] == '#')
		return;

	/*
	 * get uid 
	 */
	position = get_rocketfuel_item(item, position);
	node = atoi(item);

	/*
	 * nodes < 0 are external to this AS 
	 */
	if (node < 0)
		return;

	/*
	 *nodes are not quite sequential but they are
	 *monotonically increasing
	 *so, we always record the last node found.
	 */
	g_node_count = node+1;
	g_node_list[node].used = 1;

	/*
	 * consume location 
	 */
	position = get_rocketfuel_item(item, position);

	/*
	 * next check to see if next character is a * +, and or bb 
	 */
	position = get_rocketfuel_item(item, position);
	if (item[0] == '+')
	{

		/*
		 * consume the bb, if exists 
		 */
		position = get_rocketfuel_item(item, position);
		if (item[0] == 'b')
		{
			g_node_list[node].is_bb = 1;

			/*
			 * get the number of links 
			 */
			position = get_rocketfuel_item(item, position);
		}
	}

	for (i = 1; i < strlen(item); i++)
	{
		item[i - 1] = item[i];
		if (item[i] == ')')
		{
			item[i - 1] = '\n';
			continue;
		}
	}
	g_node_list[node].num_links = atoi(item);
	/*
	 * consume -> or &# 
	 */
	position = get_rocketfuel_item(item, position);
	if (item[0] == '&')
	{
		/*
		 * consume the -> 
		 */
		position = get_rocketfuel_item(item, position);
	}

	for (j = 0; j < g_node_list[node].num_links; j++)
	{
		position = get_rocketfuel_item(item, position);

		for (i = 1; i < strlen(item); i++)
		{
			item[i - 1] = item[i];
			if (item[i] == '>' || item[i] == '}')
			{
				item[i - 1] = (char)0;
				continue;
			}
		}

		if (atoi(item) >= 0)
			g_node_list[node].link_list[j].node_id = atoi(item);
		else
			break;
	}
	g_node_list[node].num_links = j;
}

char           *
get_rocketfuel_item(char *item, char *buffer)
{
	int             i;
	int             n;
	char           *position = buffer;
	int             j = 0;

	n = strlen(position);

	/*
	 * consume initial tab or space characters 
	 */
	for (i = 0; position[i] == '\t' || position[i] == ' '; i++);

	/*
	 * copy all non-space chars into item 
	 */
	for (; i < n; i++)
	{
		if (position[i] == '\t' || position[i] == ' ')
			break;
		else
		{
			item[j] = position[i];
			j++;
		}
	}

	/*
	 * null terminate the item string 
	 */
	item[j] = '\0';

	/*
	 * return start of next item in buffer 
	 */
	return (&position[i]);
}

void
gen_xml()
{
        int             i, j, k;
	int             id;
	int		temp;

	int		node_id = 0;

	int             found;

	int             next_host = 0;
	int             next_host_id = 0;

	struct rocket_fuel_node *n;
	struct rocket_fuel_node *nbr;

	skip = 0;
	temp = 0;

	/*
	 * Make sure all of the links are bi-directional
	 */
	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used)
		{
			skip++;
			continue;
		}

		for (j = 0; j < n->num_links; j++)
		{
			id = n->link_list[j].node_id;
			nbr = &g_node_list[id];
			found = 0;

			for (k = 0; k < nbr->num_links; k++)
			{
				if (nbr->link_list[k].node_id == i)
				{
					found = 1;
					break;
				}
			}

			if (found == 0)
			{
				nbr->link_list[nbr->num_links++].node_id = i;

				if (!nbr->used) 
				{
				        printf("this could be an error 437\n");
					nbr->used = 1;
				}
			}
		}
	}

	color_topology(MAX_LEVELS);

	for(i = 0; i < g_node_count; i++)
	{
		if(g_node_list[i].used)
			temp++;
	}

	g_total_routers = temp;

	printf("\nCount of node used %d\nIf two numbers aren't equal error?\n", g_total_routers);

	/*
	 * Setup the TCP hosts
	 */
	for (i = 0; i < g_node_count; i++)
		if (g_node_list[i].used && g_node_list[i].num_links == 1)
			g_host_count++;

	if (g_host_count % 2 != 0)
		g_host_count -= 1;

	g_host_count *= g_hosts;

	g_host_list =
		(struct rocket_fuel_node *)calloc(sizeof(struct rocket_fuel_node) *
							  (g_host_count + g_hosts), 1);


	next_host_id = g_node_count;

	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used || n->num_links != 1)
			continue;

		// create the links from the router to the hosts
		for (j = 0; j < g_hosts; j++)
		{
			// printf("link from router %d to host %d\n",i, next_host_id +
			// j);
			n->link_list[n->num_links + j].node_id = next_host_id + j;
			n->link_list[n->num_links + j].bandwidth = bandwidth_classes[MAX_LEVELS - 1];
			n->link_list[n->num_links + j].delay = delay_classes[MAX_LEVELS - 1];
		}

		next_host_id += g_hosts;
		n->num_links += g_hosts;
		n->host_links += g_hosts;

		// now setup the hosts and the links from the host to the router
		for (j = 0; j < g_hosts; j++)
		{
/*
			printf("link from host %d to router %d \n", 
						g_node_count + next_host, i);
*/
			g_host_list[next_host++].link_list[0].node_id = i;
			g_host_list[next_host++].link_list[0].bandwidth = bandwidth_classes[MAX_LEVELS - 1];
			g_host_list[next_host++].link_list[0].delay = delay_classes[MAX_LEVELS - 1];
		}
	}

	node_id = 0;
	temp = 0;

	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used)
			continue;

		if((n->num_links - n->host_links) != 1)
		{
			n->id = node_id++;
			temp++;
/*
			if(i != n->id)
				printf("%d becomes %d \n", i, n->id);
*/
		}
	}

	g_total_core = temp;
	temp = 0;

	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used)
			continue;

		if((n->num_links - n->host_links) ==  1)
		{
			temp++;
			n->id = node_id++;

/*
			if(i != n->id)
				printf("%d becomes %d \n", i, n->id);
*/
		}
	}

	for(i = 0; i < g_host_count; i++)
	{
		n = &g_host_list[i];

		n->id = node_id++;

/*
		if((i + g_node_count) != n->id)
			printf("%d becomes %d \n", i + g_node_count, n->id);
*/
	
	}

	g_total_hosts = g_host_count;
	g_total_nodes = node_id;

	for(i = 0; i < g_total_routers; i++)
		if(g_node_list[i].num_links > g_max_links)
		{
			g_max_links = g_node_list[i].num_links;
			g_max_router = g_node_list[i].id;
		}

	fprintf(xml, "<!--\n");
	fprintf(xml, "\t\t%32s\t%11d\n", "Total Hosts:", g_total_hosts);
	fprintf(xml, "\t\t%32s\t%11d\n", "Total Routers:", g_total_routers);
	fprintf(xml, "\t\t\t%32s%11d\n", "Core:", g_total_core);
	fprintf(xml, "\t\t\t%32s%11d\n", "PoP:", g_total_routers - g_total_core);
	fprintf(xml, "\n\n");
	fprintf(xml, "\t\t%32s\t%11d\n", "Total Network Nodes:", g_total_nodes);
	fprintf(xml, "\t\t%32s\t%11d\n", "Total Connections:", g_total_hosts / 2);
	fprintf(xml, "\n\n");
	fprintf(xml, "\t\t%32s\t%11d\n", "Most Connected Router:", g_max_router);
	fprintf(xml, "\t\t%32s\t%11d\n", "Most Connected Links:", g_max_links);
	fprintf(xml, "-->\n");

	fprintf(xml, "<rossnet>\n");
	fprintf(xml, "\t<as id=\'0\' frequency=\'1\'>\n");
	fprintf(xml, "\t\t<area id=\'0\'>\n");
	fprintf(xml, "\t\t\t<subnet id=\'0\'>\n");

	/*
	 * Put in all the routers with more than 1 link
	 */

	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used || (n->num_links - n->host_links) == 1)
			continue;

		fprintf(xml, "\t\t\t\t<node id=\'%d\' links=\'%d\' type=\'c_router\'>\n",
				n->id, n->num_links);

		for (j = 0; j < n->num_links; j++)
		{
			if(n->link_list[j].node_id > g_node_count)
			{
				fprintf(xml,
					"\t\t\t\t\t<link src=\'%d\' addr=\'%d\' bandwidth=\'%d\' delay=\'%f\' "
					"status=\'up\'/>\n",
					n->id, 
					g_host_list[n->link_list[j].node_id - g_node_count].id, 
					n->link_list[j].bandwidth, 
					n->link_list[j].delay );
			} else
			{
				fprintf(xml,
					"\t\t\t\t\t<link src=\'%d\' addr=\'%d\' bandwidth=\'%d\' delay=\'%f\' "
					"status=\'up\'/>\n",
					n->id, 
					g_node_list[n->link_list[j].node_id].id, 
					n->link_list[j].bandwidth, 
					n->link_list[j].delay );
			}
		}

		fprintf(xml, "\n\t\t\t\t\t<stream port=\'23\'>\n");
		fprintf(xml, "\t\t\t\t\t\t<layer name=\'ospf\' level=\'network\'>\n");

		for (j = 0; j < n->num_links; j++)
			fprintf(xml, "\t\t\t\t\t\t\t<interface src=\'%d\' addr=\'%d\' cost=\'%d\'/>\n",
					n->id, g_node_list[n->link_list[j].node_id].id, g_cost);

		fprintf(xml, "\t\t\t\t\t\t</layer>\n");
		fprintf(xml, "\t\t\t\t\t</stream>\n");
		fprintf(xml, "\t\t\t\t</node>\n");

		node_id++;
	}


	/*
	 * Put in all the edge routers
	 */
	for (i = 0; i < g_node_count; i++)
	{
		n = &g_node_list[i];

		if (!n->used || (n->num_links - n->host_links) != 1)
			continue;

		fprintf(xml, "\t\t\t\t<node id=\'%d\' links=\'%d\' type=\'c_router\'>\n",
				n->id, n->num_links);

		for (j = 0; j < n->num_links; j++)
		{
			if(n->link_list[j].node_id >= g_node_count)
			{
				fprintf(xml,
					"\t\t\t\t\t<link src=\'%d\' addr=\'%d\' bandwidth=\'%d\' delay=\'%f\' "
					"status=\'up\'/>\n",
					n->id, 
					g_host_list[n->link_list[j].node_id - g_node_count].id, 
					n->link_list[j].bandwidth, 
					n->link_list[j].delay );
			} else
			{
				fprintf(xml,
					"\t\t\t\t\t<link src=\'%d\' addr=\'%d\' bandwidth=\'%d\' delay=\'%f\' "
					"status=\'up\'/>\n",
					n->id, 
					g_node_list[n->link_list[j].node_id].id,
					n->link_list[j].bandwidth, 
					n->link_list[j].delay );
			}
		}

		fprintf(xml, "\n\t\t\t\t\t<stream port=\'23\'>\n");
		fprintf(xml, "\t\t\t\t\t\t<layer name=\'ospf\' level=\'network\'>\n");

		for (j = 0; j < n->num_links; j++)
		{
			if(n->link_list[j].node_id >= g_node_count)
			{
				fprintf(xml, "\t\t\t\t\t\t\t<interface src=\'%d\' addr=\'%d\' cost=\'%d\'/>\n",
					n->id, g_host_list[n->link_list[j].node_id - g_node_count].id, g_cost);
			} else
			{
				fprintf(xml,
					"\t\t\t\t\t\t\t<interface src=\'%d\' addr=\'%d\' cost=\'%d\'/>\n",
					n->id, 
					g_node_list[n->link_list[j].node_id].id, 
					g_cost);
			}
		}

		fprintf(xml, "\t\t\t\t\t</layer>\n");
		fprintf(xml, "\t\t\t\t\t</stream>\n");
		fprintf(xml, "\t\t\t\t</node>\n");
	}

	g_node_count++;
	for (i = 0; i < g_host_count; i++)
	{
		n = &g_host_list[i];

		fprintf(xml, "\t\t\t\t<node id=\'%d\' links=\'1\' type=\'c_host\'>\n",
				n->id);

		fprintf(xml, "\t\t\t\t\t<link src=\'%d\' addr=\'%d\' bandwidth=\'%d\' delay=\'%f\' "
				"status=\'up\'/>\n",
				n->id, g_node_list[n->link_list[0].node_id].id,
			        n->link_list[0].bandwidth, 
			        n->link_list[0].delay );
			
		fprintf(xml, "\n\t\t\t\t\t<stream port=\'1025\'>\n");
		if (i)
			fprintf(xml, "\t\t\t\t\t\t<layer name=\'tcp\' level=\'transport\'/>\n");
		else
		{
			fprintf(xml, "\t\t\t\t\t\t<layer name=\'tcp\' level=\'transport\'>\n");
			fprintf(xml, "\t\t\t\t\t\t\t<mss>512</mss>\n");
			fprintf(xml, "\t\t\t\t\t\t\t<recv_wnd>32</recv_wnd>\n");
			fprintf(xml, "\t\t\t\t\t\t</layer>\n");
		}

		fprintf(xml, "\t\t\t\t\t</stream>\n");
		fprintf(xml, "\t\t\t\t</node>\n");
	}

	fprintf(xml, "\t\t\t</subnet>\n");
	fprintf(xml, "\t\t</area>\n");
	fprintf(xml, "\t</as>\n");

	for(i = 0, j = g_host_count / 2; i < g_host_count / 2; i+=2, j+=2)
	{
		n = &g_host_list[i];

		fprintf(xml, "\t<connect src=\'%d\' dst=\'%d\'/>\n", g_host_list[i].id, g_host_list[j].id);
		fprintf(xml, "\t<connect src=\'%d\' dst=\'%d\'/>\n", g_host_list[j+1].id, g_host_list[i+1].id);
	}

	fprintf(xml, "</rossnet>\n");

	fclose(xml);
}

int
main(int argc, char **argv)
{
	if (3 > argc)
	{
		printf("usage: convert output.xml input.al\n");
		exit(1);
	}

	read_rocketfuel_file(argv);

	gen_xml();

	// print_node_list();

	return 1;
}
