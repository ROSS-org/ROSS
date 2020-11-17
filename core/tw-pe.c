#include <ross.h>

static void dummy_pe_f (tw_pe *pe)
{
    (void) pe;
}

void
tw_pe_settype(const tw_petype * type)
{
	if (!g_tw_pe)
		tw_error(TW_LOC, "Undefined PE!");

#define copy_pef(f, d) \
		g_tw_pe->type.f = type->f ? type->f : d

	copy_pef(pre_lp_init, dummy_pe_f);
	copy_pef(post_lp_init, dummy_pe_f);
	copy_pef(gvt, dummy_pe_f);
	copy_pef(final, dummy_pe_f);

#undef copy_pef
}

/*
 * tw_pe_init: initialize individual PE structs
 *
 */
void
tw_pe_init(void)
{
    if (g_tw_pe)
		tw_error(TW_LOC, "PE %u already initialized", g_tw_mynode);

    g_tw_pe = (tw_pe*)tw_calloc(TW_LOC, "PE Struct", sizeof(*g_tw_pe), 1);
	tw_petype no_type;

	memset(&no_type, 0, sizeof(no_type));

	g_tw_pe->id = g_tw_mynode;
	tw_pe_settype(&no_type);

#ifdef USE_RAND_TIEBREAKER
	g_tw_pe->trans_msg_sig = (tw_event_sig){TW_STIME_MAX,TW_STIME_MAX};
#else
	g_tw_pe->trans_msg_ts = TW_STIME_MAX;
#endif
	g_tw_pe->gvt_status = 0;

    // TODO is the PE RNG ever actually used?
	g_tw_pe->rng = tw_rand_init(31, 41);

}

void
tw_pe_fossil_collect(void)
{
	tw_kp	*kp;

	unsigned int	 i;

	g_tw_fossil_attempts++;

	for(i = 0; i < g_tw_nkp; i++)
	{
		kp = tw_getkp(i);
		tw_eventq_fossil_collect(&kp->pevent_q, g_tw_pe);
	}

}
