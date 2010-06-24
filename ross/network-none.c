#include <ross.h>

// for tw_net_onnode
tw_node mynode = 0;
static int npe = 1;
static char g_tw_net_config[256] = ".ross_network";

static const tw_optdef net_options[] = {
	TWOPT_GROUP("ROSS-SMP Kernel"),
	TWOPT_END()
};

void
tw_net_barrier(tw_pe * pe)
{
}

unsigned
tw_nnodes(void)
{
	return 1;
}

tw_statistics *
tw_net_statistics(tw_pe * me, tw_statistics * s)
{
	return s;
}

const tw_optdef *
tw_net_init(int *argc, char ***argv)
{
	FILE	*f;

	int rv = 0;

	g_tw_masternode = 0;
	g_tw_mynode = 0;

	f = fopen(g_tw_net_config, "r");

	if(!f)
		tw_error(TW_LOC, "Unable to open: %s\n", g_tw_net_config);

	while(':' != fgetc(f))
		;

	rv = fscanf(f, "%d", &npe);

	if(ferror(f))
		perror("Unable to configure ROSS");

	return net_options;
}

void
tw_net_start(void)
{
	tw_peid pi;

	g_tw_nlp *= g_tw_npe;

	tw_pe_create(npe);
	for (pi = 0; pi < g_tw_npe; pi++)
		tw_pe_init(pi, pi);
}

void
tw_net_abort(void)
{
	exit(1);
}

void
tw_net_stop(void)
{
}

static void no_network(void) NORETURN;
static void no_network(void)
{
	tw_error(TW_LOC, "Compiled without network support.");
}

void tw_net_read(tw_pe *me)
{
	no_network();
}

void
tw_net_send(tw_event *evt)
{
	no_network();
}

void
tw_net_cancel(tw_event *evt)
{
	no_network();
}

void
tw_send_stats(tw_statistics *s)
{
	no_network();
}

void
tw_recv_stats(tw_statistics *s)
{
	no_network();
}

void
tw_net_send_lvt(tw_pe * pe, tw_stime ts)
{
	no_network();
}

void
tw_net_gvt_compute(tw_pe * pe, tw_stime * ts)
{
	no_network();
}
