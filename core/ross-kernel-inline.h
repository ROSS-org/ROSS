#ifndef INC_ross_kernel_inline_h
#define INC_ross_kernel_inline_h
#include "instrumentation/st-instrumentation.h"

#define ROSS_MAX(a,b) ((a) > (b) ? (a) : (b))
#define ROSS_MIN(a,b) ((a) < (b) ? (a) : (b))

static inline tw_lp *
     tw_getlocal_lp(tw_lpid gid)
{
  tw_lpid id = gid;

  // finding analysis LPs doesn't depend on model's choice of mapping
  if (g_st_use_analysis_lps && gid >= g_st_total_model_lps)
  {
      return g_tw_lp[(gid - g_st_total_model_lps) % g_tw_nkp + g_tw_nlp];
  }

  switch (g_tw_mapping) {
  case CUSTOM:
      return( g_tw_custom_lp_global_to_local_map( gid ) );
  case ROUND_ROBIN:
      id = gid / tw_nnodes();
      break;
  case LINEAR:
      id = gid - g_tw_lp_offset;
      break;
  }

#ifdef ROSS_runtime_checks
      if (id >= g_tw_nlp)
          tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);
      if (gid != g_tw_lp[id]->gid)
          tw_error(TW_LOC, "Inconsistent LP Mapping");
#endif /* ROSS_runtime_checks */

      return g_tw_lp[id];
}

static inline tw_lp *
     tw_getlp(tw_lpid id)
{
#ifdef ROSS_runtime_checks
  if (id >= g_tw_nlp + g_st_analysis_nlp)
    tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);
  if (id != g_tw_lp[id]->id)
    tw_error(TW_LOC, "Inconsistent LP Mapping");
#endif /* ROSS_runtime_checks */
  return g_tw_lp[id];
}

static inline tw_kp *
     tw_getkp(tw_kpid id)
{
#ifdef ROSS_runtime_checks
  if (id >= g_tw_nkp)
    tw_error(TW_LOC, "ID %u exceeded MAX KPs", id);
  if( g_tw_kp[id] == NULL )
    tw_error(TW_LOC, "Local KP %u found NULL \n", id );
  if (id != g_tw_kp[id]->id)
    tw_error(TW_LOC, "Inconsistent KP Mapping");
#endif /* ROSS_runtime_checks */

  return g_tw_kp[id];
}

static inline int
     tw_ismaster(void)
{
  return (g_tw_mynode == g_tw_masternode);
}

static inline void *
     tw_getstate(tw_lp * lp)
{
  return lp->cur_state;
}

#ifdef USE_RAND_TIEBREAKER
static inline tw_stime
     tw_now(tw_lp const * lp)
{
  return (lp->kp->last_sig.recv_ts);
}

static inline tw_event_sig
     tw_now_sig(tw_lp const *lp)
{
  return (lp->kp->last_sig);
}
#else
static inline tw_stime
     tw_now(tw_lp const * lp)
{
  return (lp->kp->last_time);
}
#endif

#endif
