#ifndef INC_ross_kernel_inline_h
#define INC_ross_kernel_inline_h

#define	ROSS_MAX(a,b)	((a) > (b) ? (a) : (b))
#define	ROSS_MIN(a,b)	((a) < (b) ? (a) : (b))

static inline tw_lp * 
     tw_getlocal_lp(tw_lpid gid)
{
  long id;

  if( g_tw_custom_lp_global_to_local_map )
    return( g_tw_custom_lp_global_to_local_map( gid ) );
  
  id = gid - g_tw_lp_offset;

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
  if (id >= g_tw_nlp)
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

static inline tw_pe * 
     tw_getpe(tw_peid id)
{
#ifdef ROSS_runtime_checks  
  if (id >= g_tw_npe)
    tw_error(TW_LOC, "ID %d exceeded MAX PEs", id);
#endif /* ROSS_runtime_checks */
  
  return g_tw_pe[id];
}

#ifdef ROSS_MEMORY
static inline tw_memoryq * 
     tw_kp_getqueue(tw_kp * kp, tw_fd fd)
{
  return &kp->pmemory_q[fd];
}

static inline tw_memoryq * 
     tw_pe_getqueue(tw_pe * pe, tw_fd fd)
{
  return &pe->memory_q[fd];
}
#endif

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

static inline tw_stime 
     tw_now(tw_lp * lp)
{
  return (lp->kp->last_time);
}

#endif
