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

#ifdef ROSS_MEMORY
		tw_kp_fossil_memory(kp);
#endif
	}

}

#ifdef ROSS_MEMORY
static int next_mem_q = 0;

tw_fd
tw_pe_memory_init(tw_pe * pe, size_t n_mem, size_t d_sz, tw_stime mult)
{
	tw_memoryq	*q;

	int             fd;

	if(!pe->memory_q)
		pe->memory_q = tw_calloc(TW_LOC, "KP memory queues",
					sizeof(tw_memoryq), g_tw_memory_nqueues);

	fd = next_mem_q++;
	pe->stats.s_mem_buffers_used = 0;

	q = &pe->memory_q[fd];

	q->size = 0;
	q->start_size = n_mem;
	q->d_size = d_sz;
	q->grow = mult;

	tw_memory_allocate(q);

	return fd;
}
#endif
