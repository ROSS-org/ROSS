#include <ip.h>

#define IP_RT_DEBUG 0
#define IP_RT_BINARY 0

int routing_init_ft_write(rn_machine * m);

rn_link	*
ip_route(rn_machine * me, rn_message * msg)
{
	int	 i;
	int	 sz;

	sz = g_ip_rt[msg->src][0];

	for(i = 1; i < sz; i++)
		if(g_ip_rt[msg->src][i] == me->id)
			return rn_getlink(me, g_ip_rt[msg->src][++i]);

	return NULL;
}

int
routing_read()
{
	int	 src;
	int	 dst;
	int	 cnt;

	int	 next;
	int	 i;

	char	 dir[1024];

	sprintf(dir, "tools/%s/ip.rt", g_rn_tools_dir);
	g_ip_routing_file = fopen(dir, "r");

	if(!g_ip_routing_file)
		return 0;

#if IP_RT_DEBUG
	printf("\nReading IPv4 routing table file: %s\n", dir);
#endif

	g_ip_rt = tw_calloc(TW_LOC, "", sizeof(int *), g_rn_nmachines);

	next = 0;
	while(EOF != fscanf(g_ip_routing_file, "%d %d %d", &src, &dst, &cnt))
	{
		if(0 == cnt)
			continue;

		if(g_ip_rt[src])
			tw_error(TW_LOC, "Already allocated FT for %d (%d) \n", src, dst);

		g_ip_rt[src] = tw_calloc(TW_LOC, "", sizeof(int), cnt+1);
		g_ip_rt[src][0] = cnt;

		for(i = 1; i <= cnt; i++)
			fscanf(g_ip_routing_file, "%d", &g_ip_rt[src][i]);

		next++;
	}

	fclose(g_ip_routing_file);

#if IP_RT_DEBUG
	printf("\n\t%-50s %11d \n", "Forwarding tables", next);
#endif

	return 1;
}

void
routing_write()
{
	rn_machine	*m;

	float		 good;
	float		 bad;

	int		 i;

	char		 dir[255];

	sprintf(dir, "tools/%s/ip.rt", g_rn_tools_dir);
	g_ip_routing_file = fopen(dir, "w");

	if(!g_ip_routing_file)
		tw_error(TW_LOC, "Unable to create routing table file: %s", dir);

	if(tw_ismaster())
		printf("Creating routing table file: %s\n", dir);

	bad = good = 0;
	for(i = 0, m = g_rn_machines; i < g_rn_nmachines; m = rn_getmachine(++i))
	{
		switch(routing_init_ft_write(m))
		{
			case -1:
				break;
			case 0:
				bad++;
				break;
			default:
				good++;
		}
	}

	fclose(g_ip_routing_file);

	printf("\t%-50s %11.0f \n", "Good Connections", good);
	printf("\t%-50s %11.0f \n", "Bad Connections", bad);
	printf("\t%-50s %11.4f \n", "Percent", bad / good);

	routing_read();
}

/*
 * Create global structures needed to read/write forwarding tables
 * Note: still sequential at this point, called from ip_main
 */
void
ip_routing_init()
{
	if(!routing_read())
		routing_write();
}

void
routing_init_ft_read(ip_state * state, tw_lp * lp)
{
	rn_machine	*m = rn_getmachine(lp->gid);

#if IP_RT_BINARY
	read(fileno(g_ip_routing_file), m->ft, m->subnet->area->g_ospf_nlsa * sizeof(short));
#else
	fpos_t		 pos;

	tw_lpid		 id;

	int		 i;

	if(m->type == c_host)
		return;

	fgetpos(g_ip_routing_file, &pos);
	fscanf(g_ip_routing_file, "%lld", &id);

	if(id != lp->gid)
	{
		fsetpos(g_ip_routing_file, &pos);
		return;

		tw_error(TW_LOC, "No forwarding table for node: %ld", lp->gid);
	}

	for(i = 0; i < m->subnet->area->g_ospf_nlsa; i++)
		fscanf(g_ip_routing_file, "%d", &m->ft[i]);
#endif
}

	/*
	 * frontier[] - stores the index to vertices that are in the frontier. 
	 * solution[] - boolean solution set array. 
	 *
	 * d[] - distance to vertices. 
	 * p[] - position of vertices in frontier[] array. 
	 * f[] - boolean frontier set array. 
	 */
int	*frontier;
int	*solution;
int	*previous;

unsigned int	*distance;

int
routing_init_ft_write(rn_machine * m)
{
	rn_machine	*node;
	rn_link		*l;

	int              i, q, u, v;
	int		 size;
	int		 dist;
	int		 last;

	if(-1 == m->conn)
		return -1;

	if(!frontier)
	{
		frontier = tw_calloc(TW_LOC, "", sizeof(int), g_rn_nmachines);
		solution = tw_calloc(TW_LOC, "", sizeof(int), g_rn_nmachines);
		previous = tw_calloc(TW_LOC, "", sizeof(int), g_rn_nmachines);
		distance = tw_calloc(TW_LOC, "", sizeof(int), g_rn_nmachines);
	}

#if VERIFY_ROUTING || 1
	if(m->id % 1000 == 0)
		printf("\n%lld: Building path routing table \n", m->id);
#endif

	for (i = 0; i < g_rn_nmachines; i++)
	{
		previous[i] = -1;
		frontier[i] = -1;
		solution[i] = 0;
		distance[i] = -1;
	}

	// The start vertex is part of the solutions set. 
	solution[m->id] = 1;
	frontier[0] = m->id;

	size = 1;

	// At this point we are assuming that all vertices are reachable from
	// the starting vertex and N > 1 so that j > 0.
	while(size)
	{
		// Find the vertex in frontier that has minimum distance. 
		for(q = 0, u = frontier[q], last = q; q < size; q++)
		{
			if(distance[frontier[q]] < distance[u])
			{
				u = frontier[q];
				last = q;
			}
		}

		if(--size)
			frontier[last] = frontier[size];

		dist = distance[u] + 1;

		if(u == m->conn)
			break;

		solution[u] = 1;
		node = rn_getmachine(u);

		for(q = 0, l = node->link; q < node->nlinks; l = &node->link[++q])
		{
			if(l->wire->type == c_host && l->addr != m->conn)
				continue;

			v = l->addr;

			if(solution[v])
				continue;

			if(dist > g_rn_ttl)
				break;

			if(dist < distance[v])
			{
				previous[v] = u;
				distance[v] = dist;

				frontier[size++] = v;
			}
		}
	}

	fprintf(g_ip_routing_file, "%lld %lld %d ", m->id, m->conn, dist-1);

	//if(distance[m->conn] <= g_rn_ttl)
	{
		v = 0;
		i = dist-2;
		u = previous[m->conn];
		//printf("\n%lld %lld: ", m->id, m->conn);
		while(-1 != previous[u])
		{
			//printf("\t%d %d \n", i, u);
			solution[i--] = u;
			u = previous[u];
			v++;
		}
	}

	for(i = 0; i < v; i++)
		fprintf(g_ip_routing_file, "%d ", solution[i]);
	fprintf(g_ip_routing_file, "\n");

	return dist;
}

#if 0
/*
 * Construct the forwarding table for this simple ip router
 * Note: parallel at this point, called from ip_init
 *	 (not going to be able to read/write in this context)
 */
void
ip_routing_init_ft(ip_state * state, tw_lp * lp)
{
	char	 dir[255];

	if(g_ip_write)
	{
		if(!lp->pe->debugfd)
		{
			sprintf(dir, "%s/ip-%d-%d.rt", 
				g_rn_logs_dir, tw_net_mynode(), lp->pe->id);

			lp->pe->debugfd = fopen(dir, "w");

			if(!lp->pe->debugfd)
				tw_error(TW_LOC, 
					 "Unable to create routing table file: %s", dir);

			g_ip_write = 1;
			printf("Creating routing table file: %s\n", dir);
		}

		routing_init_ft_write(state, lp);
	} else
		routing_init_ft_read(state, lp);
}
#endif
