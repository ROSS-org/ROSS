#include <rm.h>

/*
 * Grid: the spatial grid represent the space modeled can be created in 3D
 * by created a 3D cube of LPs where the spacing between LPs in a given
 * dimension is given by the user, and stored in the array: g_rm_spatial_d.  
 * 
 * The grid is constructed by reading in a file of Z-values given in an XY-plane.
 * The analogy is that you can think of the g_rm_spatial_terrain_fn file as 
 * a blanket overlaying the terrain.  Then each successive 2D layer of the grid
 * cube is this blanket, but with an offset in the Z-value as specified in 
 * g_rm_spatial_d[2].
 *
 * Constructing the grid in this way means that 2 LPs in the LP array may not
 * be neighbors, because they may contain Z-values > g_rm_spatial_d[2].  Each
 * Cell LP has pointers to the correct neighbors that allow for the proper traversal
 * of particles.  The pointers to neighbor cells allow us to not recompute the
 * surrounding cells for each particle that passes through a given cell.
 * 
 * The grid also *always* defines a bottom-plane, not for cell LPs, but rather 
 * containing the initial z-values for the bottom-most 'blanket'.  These can then
 * be used as offsets in performing location->cell and cell->location lookups.
 * (g_rm_z_values).
 */

void
rm_initialize_terrain(double * grid, double * spacing, int ndim)
{
	int	 i;

	g_rm_spatial_dim = ndim;
	g_rm_spatial_d = tw_calloc(TW_LOC, "spatial d", sizeof(double), g_rm_spatial_dim);
	g_rm_spatial_grid = tw_calloc(TW_LOC, "spatial grid", sizeof(int), g_rm_spatial_dim);

	for(i = 0; i < g_rm_spatial_dim; i++)
	{
		g_rm_spatial_grid[i] = grid[i];
		g_rm_spatial_d[i] = spacing[i];
	}

	g_rm_z_max = g_rm_spatial_grid[2] * g_rm_spatial_d[2];
}

/*
 * rm_grid_terrain(): Add terrain Z-values to the grid
 */
void
rm_grid_terrain()
{
	FILE	*f = NULL;

	int npe = tw_nnodes() * g_tw_npe;
	int nrows_per_pe = ceil(g_rm_spatial_grid[0] / npe);
	int start_row = (nrows_per_pe * g_tw_mynode) - 1;
	int end_row = start_row + nrows_per_pe + 2;

	int 	 cnt = 0;
	int	 xi;
	int	 x;
	int	 y;
	long	 z;

	if(npe > g_rm_spatial_grid[0])
		tw_error(TW_LOC, "Too many CPUs for this terrain!");

	if(0 == g_tw_mynode)
		printf("Scenario: %s\n", g_rm_spatial_terrain_fn);

	f = fopen(g_rm_spatial_terrain_fn, "r");

	if(!f)
		tw_error(TW_LOC, "Unable to open terrain file!");

	if(!g_tw_mynode)
	{
		start_row = 0;
	} else if(g_tw_mynode == tw_nnodes() - 1)
	{
		end_row = g_rm_spatial_grid[0];
	}

	nrows_per_pe = end_row - start_row;

	if(g_rm_spatial_dim != 3)
		tw_error(TW_LOC, "Hard-coded to read in 3D terrain data (DTED)");

	// make this scalable
	g_rm_z_values = tw_calloc(TW_LOC, "z values ptr", sizeof(g_rm_z_values), 
					nrows_per_pe);

	if(!g_rm_z_values)
		tw_error(TW_LOC, "Out of memory!");

	for(x = 0; x < nrows_per_pe; x++)
		g_rm_z_values[x] = tw_calloc(TW_LOC, "z values", sizeof(int), g_rm_spatial_grid[1]);

	//tw_printf(TW_LOC, "%d: npe %d, nrows %d, srow %d, erow %d,  grid[1] %d\n", g_tw_mynode, npe, nrows_per_pe, start_row, end_row, g_rm_spatial_grid[1]);

	//printf("%d: sr %d, er %d, grid0 %d, grid1 %d \n", g_tw_mynode, start_row, end_row, g_rm_spatial_grid[0], g_rm_spatial_grid[1]);

	// seek to correct location in file
	for(x = 0; x < start_row; x++)
	{
		for(y = 0; y < g_rm_spatial_grid[1]; y++)
		{
			cnt++;
			if(EOF == fscanf(f, "%*d"))
				tw_error(TW_LOC, "%d: Malformed terrain data! (%d %d) cnt %d", g_tw_mynode, x, y, cnt);
		}
	}

	for( ; x < end_row; x++)
	{
		xi = x % nrows_per_pe;

		for(y = g_rm_spatial_grid[1]-1; y >= 0; y--)
		{
			cnt++;
			if(EOF == fscanf(f, "%d", &g_rm_z_values[xi][y]))

				tw_error(TW_LOC, "%d: Malformed terrain data! %d (%d, %d, %d) %d", 
					g_tw_mynode, cnt, x, xi, y, nrows_per_pe);

			z = lrint((double) (g_rm_z_values[xi][y] / g_rm_spatial_d[2]));
			g_rm_z_values[xi][y] = z * g_rm_spatial_d[2];
		}

		// extra value on end of line
		if(0 == strcmp("scenario", g_rm_spatial_terrain_fn))
		{
			if(EOF == fscanf(f, "%*d"))
				tw_error(TW_LOC, "Malformed terrain data!");
		}
	}
}

/*
 * rm_grid_init(): Initialize the grid data structure
 */
tw_lpid
rm_grid_init()
{
	tw_lpid	 nlp;
	int	 i;
	int	 j;

	for(i = 0, nlp = 1; i < g_rm_spatial_dim; i++)
	{
		nlp *= g_rm_spatial_grid[i];

		for(j = i - 1; j >= 0; j--)
		{
			g_rm_spatial_grid_i[i] ? 
				g_rm_spatial_grid_i[i] *= g_rm_spatial_grid[j] :
				(g_rm_spatial_grid_i[i] = g_rm_spatial_grid[j]);
		}

		g_rm_spatial_offset_ts[i*2] = g_rm_spatial_d[i] / g_rm_wave_velocity;
		g_rm_spatial_offset_ts[(i*2)+1] = g_rm_spatial_d[i] / g_rm_wave_velocity;

//tw_printf(TW_LOC, "i %dD = %12.12lf offset ts\n", i, g_rm_spatial_offset_ts[i*2]);

		if(0.0 > g_rm_spatial_offset_ts[i*2] ||
		   0.0 > g_rm_spatial_offset_ts[(i*2)+1])
			tw_error(TW_LOC, "Wave offset ts == 0.0!");

		g_rm_scatter_ts = min(g_rm_scatter_ts, g_rm_spatial_offset_ts[i*2]);
	}

	g_rm_scatter_ts /= 10.0;

	return nlp;
}
