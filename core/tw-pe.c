#include <ross.h>

static void dummy_pe_f (tw_pe *pe)
{
    (void) pe;
}

void
tw_pe_settype(tw_pe * pe, const tw_petype * type)
{
	if (!pe)
		tw_error(TW_LOC, "Undefined PE!");

#define copy_pef(f, d) \
		pe->type.f = type->f ? type->f : d

	copy_pef(pre_lp_init, dummy_pe_f);
	copy_pef(post_lp_init, dummy_pe_f);
	copy_pef(gvt, dummy_pe_f);
	copy_pef(final, dummy_pe_f);

#undef copy_pef
}

// TODO can prob remove this function
//tw_pe *
//tw_pe_next(tw_pe * last)
//{
//	if (!last)
//		return g_tw_pe;
//	if (!last->pe_next)
//		return NULL;
//	return *last->pe_next;
//}

/**
 * @brief Allocate memory for the PE struct
 */
//void tw_pe_create(void)
//{
//	g_tw_pe = (tw_pe *) tw_calloc(TW_LOC, "PE Struct", sizeof(g_tw_pe), 1);
//}

/*
 * tw_pe_init: initialize individual PE structs
 *
 * gid	-- global (across all compute nodes) PE id
 */
// TODO prob need to edit
void
tw_pe_init(tw_peid gid)
{
    if (g_tw_pe)
		tw_error(TW_LOC, "PE %u already initialized", g_tw_pe->id);

    g_tw_pe = tw_calloc(TW_LOC, "PE Struct", sizeof(*g_tw_pe), 1);
	tw_petype no_type;

	memset(&no_type, 0, sizeof(no_type));

	g_tw_pe->id = gid;
	g_tw_pe->node = g_tw_mynode;
	tw_pe_settype(g_tw_pe, &no_type);

	g_tw_pe->trans_msg_ts = DBL_MAX;
	g_tw_pe->gvt_status = 0;

    // TODO is the PE RNG ever actually used?
	g_tw_pe->rng = tw_rand_init(31, 41);

}

void
tw_pe_fossil_collect(tw_pe * me)
{
	tw_kp	*kp;

	unsigned int	 i;

	g_tw_fossil_attempts++;

	for(i = 0; i < g_tw_nkp; i++)
	{
		kp = tw_getkp(i);
		tw_eventq_fossil_collect(&kp->pevent_q, me);
	}

}

