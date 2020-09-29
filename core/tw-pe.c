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

/**
 * initialize individual PE structs
 *
 * must be called after tw_nnodes / MPI world size is set.
 *
 */
void
tw_pe_init(void)
{
    if (g_tw_pe) tw_error(TW_LOC, "PE %u already initialized", g_tw_mynode);

    g_tw_pe = (tw_pe*)tw_calloc(TW_LOC, "PE Struct", sizeof(*g_tw_pe), 1);

    g_tw_pe->id = g_tw_mynode;

    tw_petype no_type;
    memset(&no_type, 0, sizeof(no_type));
    tw_pe_settype(&no_type);

    g_tw_pe->trans_msg_ts = DBL_MAX;
    g_tw_pe->gvt_status = 0;

    // TODO is the PE RNG ever actually used?
    g_tw_pe->rng = tw_rand_init(31, 41);

    //If we're in (some variation of) optimistic mode, we need this hash
    if (g_tw_synchronization_protocol == OPTIMISTIC ||
        g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
        g_tw_synchronization_protocol == OPTIMISTIC_REALTIME) {
        g_tw_pe->hash_t = tw_hash_create();
    } else {
        g_tw_pe->hash_t = NULL;
    }

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
