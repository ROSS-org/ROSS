#ifndef INC_ross_kernel_inline_h
#define INC_ross_kernel_inline_h

#define TW_TRUE	1
#define TW_FALSE 0

#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

INLINE(tw_lp *)
tw_getlocal_lp(tw_lpid gid)
{
	int id = gid - g_tw_lp_offset;

	if (id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);
	if (gid != g_tw_lp[id]->gid)
		tw_error(TW_LOC, "Inconsistent LP Mapping");

	return g_tw_lp[id];
}

INLINE(tw_lp *)
tw_getlp(tw_lpid id)
{
	if (id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);
	if (id != g_tw_lp[id]->id)
		tw_error(TW_LOC, "Inconsistent LP Mapping");

	return g_tw_lp[id];
}

INLINE(tw_kp *)
tw_getkp(tw_kpid id)
{
	if (id >= g_tw_nkp)
		tw_error(TW_LOC, "ID %d exceeded MAX KPs", id);
	if (id != g_tw_kp[id]->id)
		tw_error(TW_LOC, "Inconsistent KP Mapping");

	return g_tw_kp[id];
}

INLINE(tw_pe *)
tw_getpe(tw_peid id)
{
	if (id >= g_tw_npe)
		tw_error(TW_LOC, "ID %d exceeded MAX PEs", id);

	return g_tw_pe[id];
}

INLINE(tw_memoryq *)
tw_kp_getqueue(tw_kp * kp, tw_fd fd)
{
	return &kp->pmemory_q[fd];
}

INLINE(tw_memoryq *)
tw_pe_getqueue(tw_pe * pe, tw_fd fd)
{
	return &pe->memory_q[fd];
}

INLINE(int)
tw_ismaster(void)
{
	return tw_node_eq(&g_tw_mynode, &g_tw_masternode);
}

INLINE(void *)
tw_getstate(tw_lp * lp)
{
	return lp->cur_state;
}

INLINE(tw_stime)
tw_now(tw_lp * lp)
{
	return (lp->kp->last_time);
}

#endif
