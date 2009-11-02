#include<pm.h>

void
init_nodes()
{
	FILE	*f = NULL;

	double	 x, y;

	int	 id;
	int	 cnt;
	int	 res = 10.0;

	char	 fn[255];

	strcpy(fn, g_rm_spatial_scenario_fn);
	strcat(fn, "pos.txt");

	f = fopen(fn, "r");

	if(!f)
		tw_error(TW_LOC, "Unable to open pos.txt in %s!", g_rm_spatial_scenario_fn);

	if(!g_pm_nnodes)
	{
		while(EOF != fscanf(f, "%*g %*d %*g %*g %*g"))
			g_pm_nnodes++;
	}

	fseek(f, 0, SEEK_SET);

	g_pm_position_x = tw_calloc(TW_LOC, "PM node X", sizeof(double), g_pm_nnodes);
	g_pm_position_y = tw_calloc(TW_LOC, "PM node Y", sizeof(double), g_pm_nnodes);
	g_pm_ids = tw_calloc(TW_LOC, "PM ids", sizeof(int), g_pm_nnodes);

	// 1081.000000 1 242700.000000 3566822.000000 3.000000

	cnt = 0;
	while(EOF != fscanf(f, "%*g %d %lf %lf %*g\n", &id, &x, &y))
	{
		x -= 190000.0;
		y -= 3524000.0;

		g_pm_ids[cnt] = id;
		g_pm_position_x[cnt] = lrint(x / res) * res;
		g_pm_position_y[cnt] = lrint(y / res) * res;

		if(++cnt == g_pm_nnodes)
			break;
	}

	if(cnt != g_pm_nnodes)
		tw_printf(TW_LOC, "Resizing number of radios: %d -> %d",
			g_pm_nnodes, cnt);

	g_pm_nnodes = cnt;
	//printf("PM # radios: %d\n", g_pm_nnodes);

	fclose(f);
}

void
init_topology()
{
}

void
pm_init_scenario()
{
	if(NULL == g_rm_spatial_scenario_fn)
		tw_error(TW_LOC, "No defined scenario dir!");

	init_nodes();
}
