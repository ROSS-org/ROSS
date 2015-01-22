#include <ross.h>

static void dummy_pe_f (tw_pe *pe)
{
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

tw_pe *
tw_pe_next(tw_pe * last)
{
	if (!last)
		return g_tw_pe[0];
	if (!last->pe_next)
		return NULL;
	return *last->pe_next;
}

void
tw_pe_create(tw_peid id)
{
	g_tw_npe = id;
	g_tw_pe = (tw_pe **) tw_calloc(TW_LOC, "PE Array", sizeof(*g_tw_pe), id);
}

/*
 * tw_pe_init: initialize individual PE structs
 *
 * id	-- local compute node g_tw_pe array index
 * gid	-- global (across all compute nodes) PE id
 */
void
tw_pe_init(tw_peid id, tw_peid gid)
{
        tw_pe *pe = (tw_pe *) tw_calloc(TW_LOC, "Local PE", sizeof(*pe), 1);
	tw_petype no_type;
	
	memset(&no_type, 0, sizeof(no_type));

	pe->id = gid;
	pe->node = g_tw_mynode;
	tw_pe_settype(pe, &no_type);

	pe->trans_msg_ts = DBL_MAX;
	pe->gvt_status = 0;

	if(id == g_tw_npe-1)
		pe->pe_next = NULL;
	else
		pe->pe_next = &g_tw_pe[id+1];

	if (g_tw_pe[id])
		tw_error(TW_LOC, "PE %u already initialized", pe->id);

	g_tw_pe[id] = pe;

	g_tw_pe[id]->rng = tw_rand_init(31, 41);
}

void
tw_pe_fossil_collect(tw_pe * me)
{
	tw_kp	*kp;

	int	 i;

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
