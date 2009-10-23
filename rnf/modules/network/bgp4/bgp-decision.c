#include <bgp.h>

/**
 * \file bgp-decision.c
 * This file contains the BGP decision process.
 */

/**
 * \def VERIFY_BGP_DECISION
 * Turns on debug print statements in this file only.  This is a shortcut
 * to setting this in CFLAGS, which requires rebuilding the entire model.
 */
#define VERIFY_BGP_DECISION 0

/**
 * This function models the BGP decision process.
 *
 * Resolve ties in the following order:
 * <ol>
 * <li>lowest local_pref (not implemented outside of XML)
 * <li>shortest AS PATH
 * <li>lowest origin, one of:
 *	<ul>
 *	<li>iBGP (1)
 *	<li>eBGP (2)
 *	<li>Incomplete (3)
 *	</ul>
 * <li>lowest MED (if next_hops equal)
 * <li>Hot Potato Routing (lowest hop count)
 * <li>lowest next hop
 * <li>DEFAULT: Keep existing route
 * </ol>
 */
int
bgp_decision(bgp_state * state, bgp_route * new, bgp_route * exist, tw_lp * lp)
{
	rn_as		*new_as = rn_getas(rn_getmachine(new->src));
	rn_as		*exist_as = rn_getas(rn_getmachine(exist->src));

	// LOCAL PREF : lower is higher priority (better)
	// was next_hop .. next_hop is AS id, not node id
	if(g_bgp_state->b.local_pref && 
		g_bgp_as[new_as->id].local_pref < g_bgp_as[exist_as->id].local_pref)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: local_pref (%d > %d) \n",
			g_bgp_as[new_as->id].local_pref, 
			g_bgp_as[exist_as->id].local_pref);
#endif
		g_bgp_stats.s_ndec_local_pref++;
		return TW_TRUE;
	} else if(g_bgp_state->b.local_pref && 
		  g_bgp_as[exist_as->id].local_pref < g_bgp_as[new_as->id].local_pref)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: local_pref (%d < %d) \n",
			g_bgp_as[new_as->id].local_pref, 
			g_bgp_as[exist_as->id].local_pref);
#endif
		g_bgp_stats.s_ndec_local_pref++;
		return TW_FALSE;
	}

	// shortest AS path
	if(g_bgp_state->b.as_path && new->as_path.size < exist->as_path.size)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: as path (%d < %d)\n",
			new->as_path.size, exist->as_path.size);
#endif
		g_bgp_stats.s_ndec_aspath++;
		return TW_TRUE;
	} else if(g_bgp_state->b.as_path && new->as_path.size > exist->as_path.size)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: as path (%d > %d)\n",
			new->as_path.size, exist->as_path.size);
#endif
		g_bgp_stats.s_ndec_aspath++;
		return TW_FALSE;
	}

	// ORIGIN code pref: 1 IGP 2 EGP 3 Incomplete
	if(g_bgp_state->b.origin && new->origin > exist->origin)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: origin (%d > %d)\n",
			new->origin, exist->origin);
#endif
		g_bgp_stats.s_ndec_origin++;
		return TW_TRUE;
	} else if(g_bgp_state->b.origin && new->origin < exist->origin)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: origin (%d < %d)\n",
			new->origin, exist->origin);
#endif
		g_bgp_stats.s_ndec_origin++;
		return TW_FALSE;
	}

	//bgp_route_print(state, new);
	//bgp_route_print(state, exist);

	// lowest med preferred, next_hop must be equal
	if(g_bgp_state->b.med && new->next_hop == exist->next_hop)
	{
		if(new->med < exist->med)
		{
#if VERIFY_BGP_DECISION
			fprintf(state->log, "\tdecision: MED (%d < %d)\n", 
				new->med, exist->med);
#endif
			g_bgp_stats.s_ndec_med++;
			return TW_TRUE;
		} else if(new->med > exist->med)
		{
#if VERIFY_BGP_DECISION
			fprintf(state->log, "\tdecision: MED (%d > %d)\n", 
				new->med, exist->med);
#endif
			g_bgp_stats.s_ndec_med++;
			return TW_FALSE;
		}
	}

	if(g_bgp_state->b.hot_potato)
	{
		bgp_nbr *exist_n = bgp_getnbr(state, exist->src);
		bgp_nbr *new_n = bgp_getnbr(state, new->src);

		if(exist_n->hop_count < new_n->hop_count)
		{
			g_bgp_stats.s_ndec_hot_potato++;
			return TW_FALSE;
		} else if(new_n->hop_count < exist_n->hop_count)
		{
/*
			fprintf(state->log, "\tdecision: hot-potato (%d < %d) \n",
				new_n->hop_count, exist_n->hop_count);
*/
			g_bgp_stats.s_ndec_hot_potato++;
			return TW_TRUE;
		}
	}

	// Always take lowest next_hop
	if(g_bgp_state->b.next_hop && new->next_hop < exist->next_hop)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: next hop (%d < %d)\n",
			new->next_hop, exist->next_hop);
#endif
		g_bgp_stats.s_ndec_next_hop++;
		return TW_TRUE;
	}

	if(g_bgp_state->b.existing)
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: default, keep existing route! \n");
#endif
		g_bgp_stats.s_ndec_existing++;
		return TW_FALSE;
	} else
	{
#if VERIFY_BGP_DECISION
		fprintf(state->log, "\tdecision: default, install new route! \n");
#endif
		g_bgp_stats.s_ndec_existing++;
		return TW_TRUE;
	}

	g_bgp_stats.s_ndec_default++;
	return TW_FALSE;
}
