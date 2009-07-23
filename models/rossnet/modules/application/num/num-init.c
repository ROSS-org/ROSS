#include<num.h>

void
init_default_profiles()
{
	num_profile	*p;

	g_num_nprofiles = 5;
	g_num_profiles = tw_calloc(TW_LOC, "", sizeof(num_profile), 5);

	/*
	 * Profile 0 - Home non-worker
	 *         1 - Non-financial worker
	 *         2 - Financial worker
	 *         3 - Child
	 *         4 - Firm A worker
	 */

	p = &g_num_profiles[0];
	p->bitrate = 14;
	p = &g_num_profiles[1];
	p->bitrate = 76;
	p = &g_num_profiles[2];
	p->bitrate = 140;
	p = &g_num_profiles[3];
	p->bitrate = 35;
	p = &g_num_profiles[4];
	p->bitrate = 140;
}


	// format is one line.  First number is number of profiles.  Each following
	// number is desired bitrate for this type worker.
void
init_profiles_from_file()
{
	FILE		*f = NULL;

	num_profile	*p;

	unsigned int	 i;

	printf("\n\tNetwork User Profiles: \n\n");

	if(NULL == (f = fopen(g_num_profiles_fn, "r")))
		tw_error(TW_LOC, "Unable to open network user profile file");

	fscanf(f, "%u", &g_num_nprofiles);

	if (!g_num_nprofiles)
		tw_error(TW_LOC, "No profiles in %s", g_num_profiles_fn);

	g_num_profiles = tw_calloc(TW_LOC, "", sizeof(num_profile), g_num_nprofiles);

	for (i = 0; i < g_num_nprofiles; i++)
	{
		p = &g_num_profiles[i];

		if (EOF == fscanf(f, "%u", &p->bitrate))
			tw_error(TW_LOC, "Malformed NUM profile!");

		printf("\tType %-45d %11d bps\n", i, p->bitrate);
	}

	fclose(f);
}

/*
 * scenario initialization
 * must initialize ic stages before initializing agents.
 */
void
num_init_scenario()
{
	int		 i;

	if (0 == strcmp(g_num_profiles_fn, "default"))
		init_default_profiles();
	else
		init_profiles_from_file();

	g_num_profile_cnts = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_num_nprofiles);
	g_num_net_pop = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_num_nprofiles);

	// if g_num_nreport_days > 0, then g_num_mod will be = 0
	if(g_num_mod >= 0)
	{
		// log number of network users who achieve each level by type
		g_num_level = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_num_nprofiles);

		for(i = 0; i < g_num_nprofiles; i++)
			g_num_level[i] = tw_calloc(TW_LOC, "",  sizeof(unsigned int), NUM_LEVELS);

		// open log file for writing
		g_num_log_f = fopen(g_num_log_fn, "w");

		if (!g_num_log_f)
			tw_error(TW_LOC, "Could not open NUM log file: %s", g_num_log_fn);
	}
}
