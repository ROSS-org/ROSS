#include <ross.h>
#include <signal.h>

static unsigned int once = 1;

void
tw_sigsegv(int sig)
{
	if (sig == SIGBUS)
		tw_error(TW_LOC, "Caught SIGBUS; terminating.");
	else if (sig == SIGSEGV)
		tw_error(TW_LOC, "Caught SIGSEGV; terminating.");
	else if (sig != SIGINT)
		tw_error(TW_LOC, "Caught unknown signal %d; terminating.", sig);

	/* nothing to report, just exit */
	if(0 == g_tw_sim_started)
		tw_net_abort();

	if (once)
	{
		if (g_tw_pe)
			tw_stats(g_tw_pe);
	}

	once = 0;

	tw_printf(TW_LOC, "Caught SIGINT; terminating.");
	tw_net_abort();
}

void
tw_sigterm(int sig)
{
	if (sig != SIGTERM)
		tw_error(TW_LOC, "Caught unknown signal %d; terminating.", sig);
	tw_net_abort();
}
