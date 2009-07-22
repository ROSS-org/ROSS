#ifndef INC_ross_kernel_inline_h
#define INC_ross_kernel_inline_h

#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

INLINE(tw_lp *)
tw_getlocal_lp(tw_lpid gid)
{
	int index = gid - g_tw_lp_offset;

	if (index >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", index);
	return g_tw_lp[index];
}

INLINE(tw_lp *)
tw_getlp(tw_lpid index)
{
	if (index >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", index);
	return g_tw_lp[index];
}

INLINE(tw_pe *)
tw_getpe(tw_peid index)
{
	if (index >= g_tw_npe)
		tw_error(TW_LOC, "ID %d exceeded MAX PEs", index);
	return g_tw_pe[index];
}

#endif
